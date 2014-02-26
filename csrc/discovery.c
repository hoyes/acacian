/**********************************************************************/
/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (c) 2013, Acuity Brands, Inc.

Author: Philip Nye <philip.nye@engarts.com>

This file forms part of Acacian a full featured implementation of 
ANSI E1.17 Architecture for Control Networks (ACN)

#tabs=3
*/
/**********************************************************************/
/*
file: discovery.c

Utilities for SLP (Service Location Protocol) as specified in epi19
*/
/**********************************************************************/
/*
Logging level for this source file.
If not set it will default to the global CF_LOG_DEFAULT

options are

lgOFF lgEMRG lgALRT lgCRIT lgERR lgWARN lgNTCE lgINFO lgDBUG
*/
//#define LOGLEVEL lgDBUG

/**********************************************************************/
#include <sys/types.h>
//#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <slp.h>
#include "acn.h"
#include "demo_utils.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#if CF_DMPCOMP_Cx
#endif

/**********************************************************************/

const char * const slperrs[] = {
	[- SLP_OK]                     = "OK",
	[- SLP_LANGUAGE_NOT_SUPPORTED] = "language not supported",
	[- SLP_PARSE_ERROR]            = "parse error",
	[- SLP_INVALID_REGISTRATION]   = "invalid registration",
	[- SLP_SCOPE_NOT_SUPPORTED]    = "scope not supported",
	[- SLP_AUTHENTICATION_ABSENT]  = "authentication absent",
	[- SLP_AUTHENTICATION_FAILED]  = "authentication failed",
	[- SLP_INVALID_UPDATE]         = "invalid update",
	[- SLP_REFRESH_REJECTED]       = "refresh rejected",
	[- SLP_NOT_IMPLEMENTED]        = "not implemented",
	[- SLP_BUFFER_OVERFLOW]        = "buffer overflow",
	[- SLP_NETWORK_TIMED_OUT]      = "network timed out",
	[- SLP_NETWORK_INIT_FAILED]    = "network init failed",
	[- SLP_MEMORY_ALLOC_FAILED]    = "memory alloc failed",
	[- SLP_PARAMETER_BAD]          = "parameter bad",
	[- SLP_NETWORK_ERROR]          = "network error",
	[- SLP_INTERNAL_SYSTEM_ERROR]  = "internal system error",
	[- SLP_HANDLE_IN_USE]          = "handle in use",
	[- SLP_TYPE_ERROR]             = "type error",
};

/* space needed for an address string - port takes max of 5 digits */
#if CF_NET_IPV6
#define MAX_IPADSTR (sizeof("[aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa]"))
#elif CF_NET_IPV4
#define MAX_IPADSTR (sizeof("aaa.aaa.aaa.aaa"))
#endif
/* MAX_PORTSTR size of "65535" */
#define MAX_PORTSTR 6

#define SVC_URLLEN (sizeof("service:acn.esta:///uuuuuuuu-uuuu-uuuu-uuuu-uuuuuuuuuuuu"))
                          //service:acn.esta:///16f5ce4a-e237-11e2-a4c1-0017316c497d
#define ATT_CIDLEN (sizeof("(cid=uuuuuuuu-uuuu-uuuu-uuuu-uuuuuuuuuuuu),"))
#define ATT_FCTNLEN (sizeof("(acn-fctn=),") + ACN_FCTN_SIZE)
#define ATT_UACNLEN (sizeof("(acn-uacn=),") + ACN_UACN_SIZE)
#define ATT_ACNSVCLEN (sizeof("(acn-services=esta.dmp),"))
#define ATT_DDLLEN (sizeof("(device-description=$:tftp:///$.ddl),") + MAX_IPADSTR)

#define CSLOCLEN(c, d) (sizeof("esta.sdt/:;esta.dmp/") + MAX_IPADSTR + MAX_PORTSTR + (!!(c)) +  ((d) ? UUID_STR_SIZE + 1 : 0))
#define MAX_CSLLEN(c, d) (sizeof(",(csl-esta.dmp=)") + CF_MAX_IPADS * (CSLOCLEN(c, d) + 1))

#define MAX_ATTLEN(c, d) (\
		ATT_CIDLEN \
		+ ATT_FCTNLEN \
		+ ATT_UACNLEN \
		+ ATT_ACNSVCLEN \
		+ ATT_DDLLEN \
		+ MAX_CSLLEN(c, d) \
		+ 1)

const char slpscope[] = EPI19_DEFAULT_SCOPE /* "," SLP_DEFAULT_SCOPE */;

/**********************************************************************/
/*
SLP handles

Keep one for SA and one for UA. All components are required to be SAs.
Only controllers need to be UAs.

slphSA - Service Agent (SA) handle
slphUA - User Agent (UA) handle
*/

static SLPHandle slphSA = NULL;
#if CF_DMPCOMP_Cx
static SLPHandle slphUA = NULL;
#endif

/**********************************************************************/
/*
constant strings
*/
const char svctype[] = "service:acn.esta:///";
#define OFS_SVC_UUID strlen(svctype);

/**********************************************************************/
/*
make_svc()

Construct an EPI-19 service URL for a local component.
Arguments:
If CF_MULTICOMP then the Lcomponent_s for the component is passed first.
The next argument is a pointer to a character array of at least SVC_URLLEN
bytes to hold the result.

The format of a service URL is
> service:acn.esta:///<cid>
where <cid> is the CID of the component in standard UUID string format.

Returns:
A pointer to a dynamically allocated service URL this must be freed after use.
*/
static inline char*
make_svc(ifMC(struct Lcomponent_s *Lcomp,) char *cp) {
#if !CF_MULTI_COMPONENT
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif
	stpcpy(stpcpy(cp, svctype), Lcomp->uuidstr);
	return cp;
}
/**********************************************************************/
/*
make_atts()

Construct an attribute string for a local component.
*/
static char *
make_atts(ifMC(struct Lcomponent_s *Lcomp)) 
{
#if !CF_MULTI_COMPONENT
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif
	char *atts;
	int i, nipads;
	char *bp;
	char *up, *fp;
	char *ipstrs[CF_MAX_IPADS];
	char dmploc[UUID_STR_SIZE + 3];  /* space for "cd:uuidstr" */
	port_t port;
	const char **interfaces;
#if !CF_LOCALIP_ANY
	netx_addr_t adhoc;
#endif

	LOG_FSTART();
	/*
	prepare for csl-esta.dmp values

	*/
	/* make our DMP declaration */
	bp = dmploc;
#if CF_DMPCOMP_Cx
	*bp++ = 'c';
#endif
#if CF_DMPCOMP_xD
	*bp++ = 'd'; *bp++ = ':';
	uuid2str(Lcomp->dmp.amap->any.dcid, bp);
#endif

#if !CF_LOCALIP_ANY
	if (netxGetMyAddr(Lcomp->sdt.adhoc, &adhoc) < 0) {
		acnlogmark(lgERR, "Can't get adhoc address");
		return NULL;
	}
	if (! netx_ISADDR_ANY(&adhoc)) {
		ipstrs[0] = mallocx(MAX_IPADSTR);
		if ( !inet_ntop(netx_TYPE(&adhoc), netx_ADDRP(&adhoc), 
						ipstrs[0], MAX_IPADSTR)) {
			free(ipstrs[0]);
			acnlogmark(lgERR, "Can't print adhoc address");
			return NULL;
		}
		nipads = 1;
	} else {
#endif
	/* Get suitable addresses */
	interfaces = Lcomp->lifetimer.userp;
	nipads = netx_getmyipstr(interfaces, GIPF_DEFAULT, GIPF_DEFAULT, 
					ipstrs, CF_MAX_IPADS);
	if (nipads <= 0) {
		acnlogmark(lgERR, "No IP addresses found");
		return NULL;
	}
	acnlogmark(lgDBUG, "Found %d IP addresses", nipads);
#if !CF_LOCALIP_ANY
	}
#endif
	port = Lcomp->sdt.adhoc->port;


	/* fctn */
	SLPEscape(Lcomp->fctn, &fp, false);
	SLPEscape(Lcomp->uacn, &up, false);

	atts = mallocx(MAX_ATTLEN(CF_DMPCOMP_Cx, CF_DMPCOMP_xD));

	/*
	atts: cid, acn-services and start csl-esta.dmp
	*/
	bp = atts;
	bp += sprintf(bp,
			"(cid=%s)"
			",(acn-services=esta.dmp)"
			",(acn-fctn=%s)"
			",(acn-uacn=%s)"
			",(csl-esta.dmp=",
						Lcomp->uuidstr,
						fp,
						up
			);
	
	SLPFree(up);
	SLPFree(fp);

	for (i = 0; i < nipads; ++i) {
		bp += sprintf(bp, "esta.sdt/%s:%hu;esta.dmp/%s,", ipstrs[i], ntohs(port), dmploc);
	}
	--bp;  /* overwrite trailing comma */

	bp = stpcpy(bp, "),(device-description=");
	for (i = 0; i < nipads; ++i) {
		bp += sprintf(bp, "$:tftp://%s/$.ddl,", ipstrs[i]);
	}
	*(bp - 1) = ')';  /* overwrite trailing comma */

	for (i = 0; i < nipads; ++i) free(ipstrs[i]);

	LOG_FEND();
	return atts;
}

/**********************************************************************/
/*
registrations

Keep track of the number of separate registrations
*/
static int registrations = 0;

#define REGISTER ((void *)0)
#define REREGISTER ((void *)1)
#define DEREGISTER ((void *)2)

/**********************************************************************/
/*
slp_reg_report

This is the open slp callback for the service registration call. All
we do is log the callback.
*/
static void
slp_reg_report(
	SLPHandle slphSA,
	SLPError errcode,
	void* cookie
)
{
	const char *rtype;
	
	rtype = (cookie == REGISTER) ? "" :
			(cookie == REREGISTER) ? "re-" :
			"de-";

	if (errcode != SLP_OK) {
		acnlogmark(lgWARN, "SLP %sregister error %s", rtype, 
					slperrs[-errcode]);
	} else if (cookie == REGISTER) ++registrations;
	else if (cookie == DEREGISTER) --registrations;
	acnlogmark(lgDBUG, "SLP %sregister: No %i", rtype, registrations);
}
/**********************************************************************/
/*
slp_refresh()

Registration renewal function, called to refresh the registration half
way through its expiry period.
*/
static void
slp_refresh(struct acnTimer_s *timer)
{
#if CF_MULTI_COMPONENT
	struct Lcomponent_s *Lcomp;

	Lcomp = container_of(timer, struct Lcomponent_s, lifetimer);
#endif
	(void) slp_register(ifMC(*Lcomp));
}
/**********************************************************************/
/*
slp_register()

Register (or re-register) a local component for advertisement by SLP
service agent.
All the necessary information is part of the Lcomponent_s which is
passed as the first arg if CF_MULTICOMP is set.
*/
int
slp_register(
	ifMC(struct Lcomponent_s *Lcomp)
)
{
#if !CF_MULTI_COMPONENT
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif
	char svcurl[SVC_URLLEN];
	char *atts;
	int rslt;
	bool fresh;

	LOG_FSTART();
	/* check we're listening */
	if (Lcomp->sdt.adhoc == NULL) {
		acnlogmark(lgERR, "No active adhoc socket");
		return -1;
	}

	/* Open SLP if necessary */
	if (slphSA == NULL && (rslt = SLPOpen(NULL, false, &slphSA)) < 0) {
		acnlogmark(lgERR, "Cannot open SLP: %s", slperrs[-rslt]);
		return -1;
	}

	make_svc(ifMC(struct Lcomponent_s *Lcomp,) svcurl);

	if (!(atts = make_atts(ifMC(Lcomp)))) {
		if (registrations == 0) {
			SLPClose(slphSA);
			slphSA = NULL;
		}
		return -1;
	}

	acnlogmark(lgDBUG, "Service URL: %s\n", svcurl);
	acnlogmark(lgDBUG, " Attributes: %s\n", atts);

	fresh = !(Lcomp->flags & Lc_advert);

	rslt = SLPReg(slphSA, svcurl, Lcomp->lifetime, "acn.esta", atts, 
						true, &slp_reg_report,
						fresh ? REGISTER : REREGISTER);
	acnlogmark(lgDBUG, "SLP registered: %s", slperrs[-rslt]);
	if (rslt == SLP_OK) {
		Lcomp->flags |= Lc_advert;
		schedule_action(&Lcomp->lifetimer, &slp_refresh,
						timerval_s(Lcomp->lifetime / 2));
	}
	free(atts);
	if (registrations == 0) {
		SLPClose(slphSA);
		slphSA = NULL;
		return -1;
	}
	return 0;
}

/**********************************************************************/
/*
slp_deregister()

De-register a local component with SLP service agent.
*/
void
slp_deregister(
	ifMC(struct Lcomponent_s *Lcomp)
)
{
#if !CF_MULTI_COMPONENT
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif
	char svcurl[SVC_URLLEN];
	int rslt;

	LOG_FSTART();
	make_svc(ifMC(struct Lcomponent_s *Lcomp,) svcurl);

	rslt = SLPDereg(slphSA, svcurl, &slp_reg_report, DEREGISTER);
	if (rslt == SLP_OK) Lcomp->flags &= ~Lc_advert;
	acnlogmark(lgDBUG, "SLP deregistered: %s", slperrs[-rslt]);
	if (registrations == 0) {
		SLPClose(slphSA);
		slphSA = NULL;
	}
	LOG_FEND();
}

/**********************************************************************/
/*
//group: discovery functions

These functions are used by DMP Controller components but are 
unnecessary for Device only components
*/
#if CF_DMPCOMP_Cx

#define logf stdout

struct newRcomp_s {
	int count;
	struct Rcomponent_s **a;
};
#define NEWCOMPBLOCKSIZE 16
/**********************************************************************/
/*
discUrl_cb()

Callback function called for each service URL discovered.
*/
static SLPBoolean
discUrl_cb(
    SLPHandle      slph,
    const char*    svcurl,
    unsigned short lifetime,
    SLPError       errcode,
    void*          cookie
)
{
	uint8_t svcCid[UUID_SIZE];
	const char *svcuustr = svcurl + strlen(svctype);
	struct newRcomp_s *newcomps = (struct newRcomp_s *)cookie;
	struct Rcomponent_s *Rcomp = NULL;

	if (errcode != SLP_OK) {
		if (errcode == SLP_LAST_CALL) {
			acnlogmark(lgDBUG, "discovery finished");
		} else {
			acnlogmark(lgWARN, "discover_cb: %s", slperrs[-errcode]);
		}
		return false;
	}
	if (strncmp(svcurl, svctype, strlen(svctype)) != 0) {
		acnlogmark(lgNTCE, "Unexpected service URL: %s", svcurl);
	} else if (str2uuid(svcuustr, svcCid) != 0) {
		acnlogmark(lgNTCE, "Bad service UUID: %s", svcurl);
	} else if (uuidsEq(svcCid, localComponent.uuid)) {
		acnlogmark(lgDBUG, "Discovered self");
	} else {
		if ((Rcomp = findRcomp(svcCid)) == NULL) {
			acnlogmark(lgDBUG, "Discovered new: %s", svcuustr);
			Rcomp = acnNew(struct Rcomponent_s);
			uuidcpy(Rcomp->uuid, svcCid);
			Rcomp->slp.flags = slp_found;
		}
		if (newcomps->count % NEWCOMPBLOCKSIZE == 0) {
			if (newcomps->count == 0)
				newcomps->a = mallocx(NEWCOMPBLOCKSIZE * sizeof(*newcomps->a));
			else newcomps->a = reallocx(newcomps->a,
				(newcomps->count + NEWCOMPBLOCKSIZE) * sizeof(*newcomps->a));
		}
		newcomps->a[newcomps->count++] = Rcomp;
	}
	return true;
}
/**********************************************************************/

   /*
   attr-list = attribute / attribute `,' attr-list
   attribute = `(' attr-tag `=' attr-val-list `)' / attr-tag
   attr-val-list = attr-val / attr-val `,' attr-val-list
   attr-tag = 1*safe-tag
   attr-val = intval / strval / boolval / opaque
   intval = [-]1*DIGIT
   strval = 1*safe-val
   boolval = "true" / "false"
   opaque = "\FF" 1*escape-val
   safe-val = ; Any character except reserved.
   safe-tag = ; Any character except reserved, star and bad-tag.
   reserved = `(' / `)' / `,' / `\' / `!'  / `<' / `=' / `>' / `~' / CTL
   escape-val = `\' HEXDIG HEXDIG
   bad-tag = CR / LF / HTAB / `_'
    star = `*'
   */
#define HX 0x80
#define RSV 0x40
#define BTG 0x20
uint8_t esctb[256] = {
	RSV,					/*  0   NUL */
	RSV,					/*  1   SOH */
	RSV,					/*  2   STX */
	RSV,					/*  3   ETX */
	RSV,					/*  4   EOT */
	RSV,					/*  5   ENQ */
	RSV,					/*  6   ACK */
	RSV,					/*  7   BEL */
	RSV,					/*  8   BS  */
	RSV | BTG,			/*  9   HT  */
	RSV | BTG,			/*  a   LF  */
	RSV,					/*  b   VT  */
	RSV,					/*  c   FF  */
	RSV | BTG,			/*  d   CR  */
	RSV,					/*  e   SO  */
	RSV,					/*  f   SI  */
	RSV,					/* 10   DLE */
	RSV,					/* 11   DC1 */
	RSV,					/* 12   DC2 */
	RSV,					/* 13   DC3 */
	RSV,					/* 14   DC4 */
	RSV,					/* 15   NAK */
	RSV,					/* 16   SYN */
	RSV,					/* 17   ETB */
	RSV,					/* 18   CAN */
	RSV,					/* 19   EM  */
	RSV,					/* 1a   SUB */
	RSV,					/* 1b   ESC */
	RSV,					/* 1c   FS  */
	RSV,					/* 1d   GS  */
	RSV,					/* 1e   RS  */
	RSV,					/* 1f   US  */
	[0x7f] = RSV,
	['('] = RSV,
	[')'] = RSV,
	[','] = RSV,
	['\\'] = RSV,
	['!'] = RSV,
	['<'] = RSV,
	['='] = RSV,
	['>'] = RSV,
	['~'] = RSV,
	['*'] = BTG,
	['_'] = BTG,

	['0'] = HX + 0x0,
	['1'] = HX + 0x1,
	['2'] = HX + 0x2,
	['3'] = HX + 0x3,
	['4'] = HX + 0x4,
	['5'] = HX + 0x5,
	['6'] = HX + 0x6,
	['7'] = HX + 0x7,
	['8'] = HX + 0x8,
	['9'] = HX + 0x9,
	['A'] = HX + 0xa,
	['B'] = HX + 0xb,
	['C'] = HX + 0xc,
	['D'] = HX + 0xd,
	['E'] = HX + 0xe,
	['F'] = HX + 0xf,
	['a'] = HX + 0xa,
	['b'] = HX + 0xb,
	['c'] = HX + 0xc,
	['d'] = HX + 0xd,
	['e'] = HX + 0xe,
	['f'] = HX + 0xf,
};

/**********************************************************************/
/*
parseatts()

Parse an SLP attribute list for a service:acn.esta service.

example attribute string (with newlines inserted between atts):

"(cid=78247984-3627-11e3-a8fe-0017316c497d),
(acn-services=esta.dmp),
(csl-esta.dmp=esta.sdt/192.168.156.3:33554;esta.dmp/d:684867b8-eb9b-11e2-b590-0017316c497d),
(device-description=$:tftp://192.168.156.3/$.ddl),
(acn-fctn=Acacian Demo Device),
(acn-uacn=Testing: comma\2C separated \5C text with \28brackets\29)"
*/

/*
Must be in lexical order of corresponding strings
*/
enum needatt_e {
	na_fctn,
	na_svc,
	na_uacn,
	na_cid,
	na_csl,
	na_ddl,
	na_MAX
};

char *needatts[na_MAX] = {
"acn-fctn",
"acn-services",
"acn-uacn",
"cid",
"csl-esta.dmp",
"device-description",
};

#define NF_OPT 1
#define NF_LST 2

uint8_t naflags[na_MAX] = {
	[na_svc]  = NF_LST,
	[na_cid]  = 0,
	[na_csl]  = NF_LST,
	[na_ddl]  = NF_LST,
	[na_fctn] = 0,
	[na_uacn] = 0,	
};

/*
//macro: NA_OPTMASK
Bits corresponding to all optional attributes
*/
#define NA_OPTMASK 0
#define NA_MASK ((1 << na_MAX) - 1)

struct e19att_s {
	char *attp[na_MAX];
	uint16_t vcount[na_MAX];
	char buf[];
};

/**********************************************************************/
static int
matchatt(const char *name, unsigned len)
{
	int hi, lo, i, diff;

	hi = na_MAX;
	lo = 0;
	do {
		i = (hi + lo) / 2;
		diff = strncasecmp(name, needatts[i], len);
		if (diff == 0) return i;
		else if (diff < 0) hi = i;
		else lo = i + 1;
	} while (hi > lo);
	return -1;
}


/**********************************************************************/
static struct e19att_s *
parseatts(const char *attstr)
{
	struct e19att_s *atts;
	
	const char *sp, *ep;
	char *dp;
	int match;
	unsigned flags, bit;
	unsigned totlen;

	/* this does not do strict checks on format */
	/* first pass */
	flags = 0;
	totlen = 0;
	acnlogmark(lgDBUG, "Attribute string: \"%s\"", attstr);
	for (sp = attstr; (sp = strchr(sp, '('));) {
		++sp;
		ep = strchr(sp, '=');
		match = matchatt(sp, (unsigned)(ep - sp));
		if (match < 0) {
			acnlogmark(lgINFO, "Ignoring %.*s attribute",
									(unsigned)(ep - sp), sp);
			continue;
		}
		bit = 1 << match;
		if (flags & bit) {
			acnlogmark(lgERR, "Duplicate %s attribute", needatts[match]);
			return NULL;
		}
		flags |= bit;
		sp = strchr(ep, ')');
		if (ep == NULL) {
			acnlogmark(lgERR, "Expected ')' after %s=", needatts[match]);
			return NULL;
		}
		totlen += sp - ep;
	}
	/* check we've had all attributes */
	flags |= NA_OPTMASK;   /* force optional att bits on */
	flags ^= NA_MASK;

	if (flags) {
		for (match = 0; flags; ++match, flags >>= 1) {
			if (flags & 1)
				acnlogmark(lgERR, "Missing %s attribute", needatts[match]);
		}
		return NULL;
	}

	/* we should be OK - assign the buffer and fill it */
	atts = mallocx(offsetof(struct e19att_s, buf[0]) + totlen);

	/* second pass */
	dp = atts->buf;
	for (sp = attstr; (sp = strchr(sp, '('));) {
		++sp;
		ep = strchr(sp, '=');
		match = matchatt(sp, (unsigned)(ep - sp));
		if (match < 0) {
			sp = ep + 1;
			continue;
		}
		atts->attp[match] = dp;
		atts->vcount[match] = 1;
		sp = ep + 1;
		while (*sp != ')') {
			unsigned h, b;

			if (*sp == ',') {
				if ( !(naflags[match] & NF_LST)) {
					acnlogmark(lgERR, "Multiple values for %s", needatts[match]);
					free(atts);
					return NULL;
				}
				*dp++ = 0;
				++atts->vcount[match];
				++sp;
			} else if (*sp != '\\') {
				*dp++ = *sp++;
			} else if ( ((h = esctb[(uint8_t)sp[1]]) & HX)
						&& ((b = esctb[(uint8_t)sp[2]]) & HX)
						&& (esctb[b = (h & 0xf) << 4 | (b & 0xf)] & RSV))
			{
				*dp++ = b;
				sp += 3;
			} else {
				acnlogmark(lgERR, "Illegal escape in %s", needatts[match]);
				free(atts);
				return NULL;
			}
		}
		*dp++ = 0;
	}
	return atts;
}
/**********************************************************************/
static const char sdtcsl[] = "esta.sdt/";
#define SDTCSLEN (sizeof(sdtcsl) - 1)
static const char dmpcsl[] = "esta.dmp/";
#define DMPCSLEN (sizeof(dmpcsl) - 1)
enum csldmp_rslt_e {
	CSL_OK,
	CSL_EPROTO,
	CSL_ENETF,
	CSL_EPORT,
	CSL_EDMP,
};
/**********************************************************************/
/*

esta.sdt/192.168.156.3:33554;esta.dmp/d:684867b8-eb9b-11e2-b590-0017316c497d
*/
static int
parsedmpcsl(char *csl, netx_addr_t *skad, uint8_t *dcid)
{
	char *cp, *ep;
	uint16_t port;		
	int flags;
	//const char * INITIALIZED(dcidstr);
#if CF_NET_MULTI
#define SKAD4 ((struct sockaddr_in *)skad)
#define SKAD6 ((struct sockaddr_in6 *)skad)
#else
#define SKAD4 skad
#define SKAD6 skad
#endif

	//acnlogmark(lgDBUG, "pcsl: %s", csl);
	if (strncasecmp(csl, sdtcsl, SDTCSLEN) != 0) return -CSL_EPROTO;

	//acnlogmark(lgDBUG, "pcsl: %s OK", sdtcsl);
	cp = strchr(csl + SDTCSLEN, ':');
	if (cp == NULL
		|| (port = strtoul(cp + 1, &ep, 10)) == 0  /* 0 is legal conversion but invalid port */
		|| *ep != ';'
	) return -CSL_EPORT;

	//acnlogmark(lgDBUG, "pcsl: port %hu OK", port);
	memset(skad, 0, sizeof(netx_addr_t));
#if CF_NET_IPV4
	*cp = 0;
	if (inet_pton(AF_INET, csl + SDTCSLEN, &SKAD4->sin_addr) == 1) {
		netx_TYPE(skad) = AF_INET;
		netx_PORT(skad) = htons(port);
	}
	*cp = ':';  /* restore */
#endif
#if CF_NET_IPV6
	if (csl[SDTCSLEN] == '[' && cp[-1] == ']') {
		cp[-1] = 0;
		if (inet_pton(AF_INET6, csl + SDTCSLEN + 1, &SKAD6->sin6_addr) == 1) {
			netx_TYPE(skad) = AF_INET6;
			netx_PORT(skad) = htons(port);
		}
		cp[-1] = ']';  /* restore */
	}
#endif
	if (netx_TYPE(skad) == 0) return -CSL_ENETF;
	/* Got a good SDT csl - now check DMP */

	//acnlogmark(lgDBUG, "pcsl: addr type %i", netx_TYPE(skad));
	cp = ep + 1;
	if (strncasecmp(cp, dmpcsl, DMPCSLEN) != 0) return -CSL_EPROTO;
	cp += DMPCSLEN;

	//acnlogmark(lgDBUG, "pcsl: %s OK", dmpcsl);

	flags = 0;
	if (*cp == 'c' || *cp == 'C') {
		flags |= slp_ctl;
		++cp;
	}
	if ((*cp == 'd' || *cp =='D') 
		&& *++cp == ':' 
		&& str2uuid(++cp, dcid) == 0)
	{
		flags |= slp_dev;
		cp += UUID_STR_SIZE - 1;
	}
	//acnlogmark(lgDBUG, "pcsl: left <%s>", cp);
	if (*cp) return -CSL_EDMP;
	if (flags == 0) flags = slp_err;
	return flags;
}

/**********************************************************************/
/*
discAtt_cb()

Callback function called for each attribute list received.
Parse the attributes and add DMP components to the remote
component set.

*/
static SLPBoolean
discAtt_cb(
    SLPHandle      slph,
    const char*    atts,
    SLPError       errcode,
    void*          cookie
)
{
	//char uuidstr[UUID_STR_SIZE];
	uint8_t uuid[UUID_SIZE];
	struct Rcomponent_s *Rcomp = (struct Rcomponent_s *)cookie;
	struct e19att_s *eatts;
	int rslt;
	char *cp;
	int i;

	if (errcode != SLP_OK) return false;

	if ((eatts = parseatts(atts)) == NULL) {
		Rcomp->slp.flags = slp_err | slp_found;
		goto done;
	}

#if CF_STRICT_CHECKS
	/* cid attribute is redundant here */
	if (str2uuid(eatts->attp[na_cid], uuid) != 0
		|| !uuidsEq(uuid, Rcomp->uuid))
	{
		acnlogmark(lgERR, "CID attribute error");
		Rcomp->slp.flags = slp_err | slp_found;
		goto done;
	}
#endif
	/* check for DMP service */
	cp = eatts->attp[na_svc];
	i = eatts->vcount[na_svc];
	while (strcasecmp(cp, "esta.dmp") != 0) {
		if (--i == 0) {
			acnlogmark(lgINFO, "Not a DMP component");
			goto done;
		}
		cp = strchr(cp, 0) + 1;
	}

	for (i = eatts->vcount[na_csl], cp = eatts->attp[na_csl];
			i--;
			cp = strchr(cp, 0) + 1)
	{
		netx_addr_t adhoc;

		if ((rslt = parsedmpcsl(cp, &adhoc, uuid)) < 0) {
			acnlogmark(lgINFO, "Parse csl error: %d", -rslt);
		} else {
			/* got a suitable csl */
			Rcomp->slp.flags = rslt | slp_found;
			memcpy(&Rcomp->sdt.adhocAddr, &adhoc, sizeof(netx_addr_t));
			if (Rcomp->slp.fctn == NULL)
				Rcomp->slp.fctn = strdup(eatts->attp[na_fctn]);
			if (Rcomp->slp.uacn) free(Rcomp->slp.uacn);  /* use latest */
			Rcomp->slp.uacn = strdup(eatts->attp[na_uacn]);
			if (rslt & slp_dev) {
				Rcomp->slp.dcid = mallocx(UUID_SIZE);
				uuidcpy(Rcomp->slp.dcid, uuid);
			}
			goto done;
		}
	}
	// Rcomp->slp.flags = slp_err | slp_found;
	acnlogmark(lgINFO, "No useable DMP access method");
done:
	addRcomponent(Rcomp);
	free(eatts);
	return false;
}
/**********************************************************************/
/*
discover()

Call openSLP to discover available acn.esta services.

This first builds a list of services, then queries each to find
their attributes. The callback for returned attributes <discAtt_cb> 
parses the attributes and adds or updates
suitable discovered components in the Remote component set.

In applications where large numbers of remote components are
discovered that are not connected to, it could be more efficient
to maintain seperate sets for discovered components and actively
connected components.
*/
void
discover(void)
{
	int rslt;
	struct newRcomp_s newcomps;

	LOG_FSTART();
	/* first open SLP */
	if (slphUA == NULL && (rslt = SLPOpen(NULL, false, &slphUA)) < 0) {
		acnlogmark(lgERR, "Cannot open SLP: %s", slperrs[-rslt]);
		return;
	}
	newcomps.count = 0;
	rslt = SLPFindSrvs(slphUA, "service:acn.esta", slpscope, "", &discUrl_cb, &newcomps);
	if (rslt < 0) {
		acnlogmark(lgWARN, "SLP discover URL: %s", slperrs[-rslt]);
	}
	acnlogmark(lgDBUG, "Found %d components.", newcomps.count);
	if (newcomps.count) {
		int i;
		char svcurl[SVC_URLLEN];
		char *cidsp;

		cidsp = stpcpy(svcurl, svctype);
		for (i = 0; i < newcomps.count; ++i) {
			struct Rcomponent_s *Rcomp = newcomps.a[i];

			uuid2str(Rcomp->uuid, cidsp);
			rslt = SLPFindAttrs(slphUA, svcurl, slpscope, "", &discAtt_cb, Rcomp);
			if (rslt < 0) {
				acnlogmark(lgERR, "SLP discover att: %s", slperrs[-rslt]);
				free(Rcomp);
			}
		}
		free(newcomps.a);
		newcomps.count = 0;
	}
	LOG_FEND();
	return;
}

#endif  /* CF_DMPCOMP_Cx */
