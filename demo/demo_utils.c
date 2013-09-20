/**********************************************************************/
/*
Copyright (c) 2013, Philip Nye
All rights reserved.

	$Id$

#tabs=3t
*/
/**********************************************************************/

#include "acn.h"
#include <slp.h>

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_APP

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

#define SVC_URLLEN (sizeof("service:acn.esta:///uuuuuuuu-uuuu-uuuu-uuuu-uuuuuuuuuuuu"))
                          //service:acn.esta:///16f5ce4a-e237-11e2-a4c1-0017316c497d
#define ATT_CIDLEN (sizeof("(cid=uuuuuuuu-uuuu-uuuu-uuuu-uuuuuuuuuuuu)"))
#define ATT_FCTNLEN (sizeof(",(fctn=)") + ACN_FCTN_SIZE)
#define ATT_UACNLEN (sizeof(",(uacn=)") + ACN_UACN_SIZE)
#define ATT_ACNSVCLEN (sizeof(",(acn-services=esta.dmp)"))

/* space needed for an address and port string - port takes max of 5 digits */
#if ACNCFG_NET_IPV6
#define MAX_IPADSTR (sizeof("[aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa]"))
#elif ACNCFG_NET_IPV4
#define MAX_IPADSTR (sizeof("aaa.aaa.aaa.aaa"))
#endif

#define CSLOCLEN (sizeof("esta.sdt/:ppppp;esta.dmp/") + MAX_IPADSTR ifDMP_C(+ 1) ifDMP_D(+ UUID_STR_SIZE + 1))

#define MAX_CSLLEN (sizeof(",(csl-esta.dmp=)") + ACNCFG_MAX_IPADS * (CSLOCLEN + 1))
#define MAX_ATTLEN (ATT_CIDLEN + ATT_FCTNLEN + ATT_UACNLEN + ATT_ACNSVCLEN + MAX_CSLLEN)
/**********************************************************************/
static SLPHandle slph = NULL;

/**********************************************************************/
char *
makesvc(
	ifMC(struct Lcomponent_s *Lcomp,)
	port_t port,
	ifDMP_D(const char *dcidstr,)
	const char *interfaces[]
) {
#if !ACNCFG_MULTI_COMPONENT
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif
	int rslt;
	char *bp;
	char **ipstrs;
	char **ipstrp;
	char *escuacn, *escfctn;
	char dmploc[UUID_STR_SIZE + 3];  /* space for "cd:uuidstr" */
	char *buffer;

	LOG_FSTART();
	/*
	Get suitable addresses
	*/
	ipstrs = netx_getmyipstr(interfaces, GIPF_DEFAULT, GIPF_DEFAULT, ACNCFG_MAX_IPADS);
	if (!ipstrs) {
		acnlogmark(lgERR, "No IP addresses found");
		return NULL;
	}

#if acntestlog(lgDBUG)
	for (ipstrp = ipstrs, rslt = 0; *ipstrp; ++ipstrp) ++rslt;
	acnlogmark(lgDBUG, "Found %d IP addresses", rslt);
#endif

	buffer = mallocx(SVC_URLLEN + MAX_ATTLEN + 1);
	port = ntohs(port);  /* byteswap once if necessary */

	/* make our DMP declaration */
	bp = dmploc;
#if ACNCFG_DMP_CONTROLLER
	*bp++ = 'c';
#endif
#if ACNCFG_DMP_DEVICE
#if ACNCFG_STRICT_CHECKS
	if (str2uuid(dcidstr, NULL) < 0) {
		acnlogmark(lgERR, "illegal DCID \"%s\"", dcidstr);
		return NULL;
	}
#endif
	*bp++ = 'd';
	*bp++ = ':';
	strcpy(bp, dcidstr);
#endif

	/* generate the service URL */
	rslt = snprintf(buffer, SVC_URLLEN + 1, "service:acn.esta:///%s", Lcomp->uuidstr);
	acnlogmark(lgDBUG, "Service URL length %d (expected %lu): %s", rslt, SVC_URLLEN - 1, buffer);
	
	assert(rslt == SVC_URLLEN - 1);
	bp = buffer + SVC_URLLEN;

	SLPEscape(Lcomp->fctn, &escfctn, false);
	SLPEscape(Lcomp->uacn, &escuacn, false);

	/* now attributes */
	/* cid, fctn, uacn and start of csl-dmp */
	bp += snprintf(bp, MAX_ATTLEN + 1,
					"(cid=%s),(fctn=%s),(uacn=%s),(acn-services=esta.dmp),(csl-esta.dmp=",
					Lcomp->uuidstr, escfctn, escuacn);
	SLPFree(escfctn);
	SLPFree(escuacn);

	/* now csl-dmp values - one per ip address */
	for (ipstrp = ipstrs; *ipstrp; ++ipstrp) {
		bp += sprintf(bp, "esta.sdt/%s:%hu;esta.dmp/%s,", *ipstrp, port, dmploc);
		free(*ipstrp);
	}
	free(ipstrs);
	*(bp - 1) = ')';  /* overwrite trailing comma */

	buffer = realloc(buffer, bp - buffer + 1);
	if (!buffer) {
		acnlogmark(lgERR, "realloc fail");
	}
	LOG_FEND();
	return buffer;
}

/**********************************************************************/
void
slp_reg_report(
	SLPHandle slph,
	SLPError errcode,
	void* cookie
)
{
	acnlogmark(lgDBUG, "SLP register callback: %s\n", slperrs[-errcode]);
}
/**********************************************************************/
int
slp_start_sa(
	ifMC(struct Lcomponent_s *Lcomp,)
	port_t port,
	ifDMP_D(const char *dcidstr,)
	const char *interfaces[]
)
{
#if !ACNCFG_MULTI_COMPONENT
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif
	char *svc;
	int rslt;

	LOG_FSTART();
	/* first open SLP */
	if (slph == NULL && (rslt = SLPOpen(NULL, false, &slph)) < 0) {
		acnlogmark(lgERR, "Cannot open SLP: %s\n", slperrs[-rslt]);
		return -1;
	}

	svc = makesvc(ifMC(Lcomp,) port, ifDMP_D(dcidstr,) interfaces);
	if (svc == NULL) {
		acnlogmark(lgERR, "makesvc fail\n", slperrs[-rslt]);
		return -1;
	} else {
		acnlogmark(lgDBUG,
						"Service URL: %s\n"
						" Attributes: %s\n",
						svc,
						svc + SVC_URLLEN
					);
	}
	rslt = SLPReg(slph, svc, SLP_LIFETIME_MAXIMUM, "acn.esta", svc + SVC_URLLEN, true, &slp_reg_report, NULL);
	acnlogmark(lgDBUG, "SLP registered: %s\n", slperrs[-rslt]);
	LOG_FEND();
	return 0;
}
