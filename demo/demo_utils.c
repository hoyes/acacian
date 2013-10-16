/**********************************************************************/
/*
Copyright (c) 2013, Philip Nye
All rights reserved.

	$Id$

#tabs=3t
*/
/**********************************************************************/

#include <sys/types.h>
//#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <slp.h>

#include "acn.h"
#include "demo_utils.h"

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

/* space needed for an address string - port takes max of 5 digits */
#if ACNCFG_NET_IPV6
#define MAX_IPADSTR (sizeof("[aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa]"))
#elif ACNCFG_NET_IPV4
#define MAX_IPADSTR (sizeof("aaa.aaa.aaa.aaa"))
#endif
#define MAX_PORTSTR 5

#define SVC_URLLEN (sizeof("service:acn.esta:///uuuuuuuu-uuuu-uuuu-uuuu-uuuuuuuuuuuu"))
                          //service:acn.esta:///16f5ce4a-e237-11e2-a4c1-0017316c497d
#define ATT_CIDLEN (sizeof("(cid=uuuuuuuu-uuuu-uuuu-uuuu-uuuuuuuuuuuu),"))
#define ATT_FCTNLEN (sizeof("(fctn=),") + ACN_FCTN_SIZE)
#define ATT_UACNLEN (sizeof("(uacn=),") + ACN_UACN_SIZE)
#define ATT_ACNSVCLEN (sizeof("(acn-services=esta.dmp),"))
#define ATT_DDLLEN (sizeof("(device-description=$:tftp:///$.ddl),") + MAX_IPADSTR)

#define CSLOCLEN (sizeof("esta.sdt/:;esta.dmp/") + MAX_IPADSTR + MAX_PORTSTR ifDMP_C(+ 1) ifDMP_D(+ UUID_STR_SIZE + 1))
#define MAX_CSLLEN (sizeof(",(csl-esta.dmp=)") + ACNCFG_MAX_IPADS * (CSLOCLEN + 1))

#define MAX_ATTLEN (ATT_CIDLEN + ATT_FCTNLEN + ATT_UACNLEN + ATT_ACNSVCLEN + ATT_DDLLEN + MAX_CSLLEN + 1)
/**********************************************************************/
static SLPHandle slph = NULL;
uint16_t adhocport = 0;

const char *interfaces[MAXINTERFACES] = {NULL};
int ifc = 0;

/**********************************************************************/
/*
UACN stuff
*/
const char cfgdir[] = "/.device_demo/";
const char uacnsuffix[] = ".uacn";
char uacn[ACN_UACN_SIZE + 1];  /* allow for trailing newline */
extern const char uacn_dflt[];
char *uacncfg;  /* path and name of UACN config file */

/**********************************************************************/
char *
make_svc(ifMC(struct Lcomponent_s *Lcomp)) {
#if !ACNCFG_MULTI_COMPONENT
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif
	char *svcurl;
	int rslt;

	LOG_FSTART();
	svcurl = mallocx(SVC_URLLEN);
	rslt = stpcpy(stpcpy(svcurl, "service:acn.esta:///"), Lcomp->uuidstr) - svcurl;
	acnlogmark(lgDBUG, "Service URL length %d (expected %lu): %s", rslt, SVC_URLLEN - 1, svcurl);
	assert(rslt == SVC_URLLEN - 1);
	LOG_FEND();
	return svcurl;
}
/**********************************************************************/

char *
make_atts(
	ifMC(struct Lcomponent_s *Lcomp)
) {
#if !ACNCFG_MULTI_COMPONENT
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif
	char *atts;
	int i, nipads;
	char *bp, *cp;
	char *ipstrs[ACNCFG_MAX_IPADS];
	char dmploc[UUID_STR_SIZE + 3];  /* space for "cd:uuidstr" */
	port_t port;

	/*
	prepare for csl-esta.dmp values

	*/
	/* make our DMP declaration */
	bp = dmploc;
#if ACNCFG_DMP_CONTROLLER
	*bp++ = 'c';
#endif
#if ACNCFG_DMP_DEVICE
	*bp++ = 'd'; *bp++ = ':';
	uuid2str(Lcomp->dmp.amap->any.dcid, bp);
#endif

	/* Get suitable addresses */
	nipads = netx_getmyipstr(interfaces, GIPF_DEFAULT, GIPF_DEFAULT, ipstrs, ACNCFG_MAX_IPADS);
	if (nipads <= 0) {
		acnlogmark(lgERR, "No IP addresses found");
		return NULL;
	}
	acnlogmark(lgDBUG, "Found %d IP addresses", nipads);

	port = ntohs(adhocport);

	bp = atts = mallocx(MAX_ATTLEN);

	/*
	atts: cid, acn-services and start csl-esta.dmp
	*/
	bp += sprintf(bp, "(cid=%s),(acn-services=esta.dmp),(csl-esta.dmp=",
						Lcomp->uuidstr);
	for (i = 0; i < nipads; ++i) {
		bp += sprintf(bp, "esta.sdt/%s:%hu;esta.dmp/%s,", ipstrs[i], port, dmploc);
	}
	*(bp - 1) = ')';  /* overwrite trailing comma */

#if 0
	bp = stpcpy(bp, ",(device-description=");  /* overwrite trailing comma */
	for (i = 0; i < nipads; ++i) {
		bp += sprintf(bp, "$:tftp://%s/$.ddl,", ipstrs[i]);
	}
	*(bp - 1) = ')';  /* overwrite trailing comma */
#else
	bp += sprintf(bp, ",(device-description=$:tftp://%s/$.ddl)", ipstrs[0]);
#endif

	for (i = 0; i < nipads; ++i) free(ipstrs[i]);

	/* fctn */
	SLPEscape(Lcomp->fctn, &cp, false);
	bp += sprintf(bp, ",(fctn=%s)", cp);
	SLPFree(cp);

	SLPEscape(Lcomp->uacn, &cp, false);
	bp += sprintf(bp, ",(uacn=%s)", cp);
	SLPFree(cp);

	LOG_FEND();
	return atts;
}

/**********************************************************************/
void
slp_reg_report(
	SLPHandle slph,
	SLPError errcode,
	void* cookie
)
{
	acnlogmark(lgDBUG, "SLP %s callback: %s\n", (char *)cookie, slperrs[-errcode]);
}
/**********************************************************************/
int
slp_register(
	ifMC(struct Lcomponent_s *Lcomp)
)
{
	char *svcurl;
	char *atts;
	int rslt;

	svcurl = make_svc(ifMC(struct Lcomponent_s *Lcomp));

	if (!(atts = make_atts(ifMC(Lcomp)))) {
		free(svcurl);
		return -1;
	}

	acnlogmark(lgDBUG,
					"Service URL: %s\n"
					" Attributes: %s\n", svcurl, atts);

	rslt = SLPReg(slph, svcurl, SLP_LIFETIME_MAXIMUM, "acn.esta", atts, 
						true, &slp_reg_report, "register");
	acnlogmark(lgDBUG, "SLP registered: %s\n", slperrs[-rslt]);
	free(atts);
	free(svcurl);
	return 0;
}

/**********************************************************************/
void
slp_deregister(void)
{
	char *svcurl;
	int rslt;

	LOG_FSTART();
	svcurl = make_svc(ifMC(struct Lcomponent_s *Lcomp));

	rslt = SLPDereg(slph, svcurl, &slp_reg_report, "deregister");
	acnlogmark(lgDBUG, "SLP deregistered: %s\n", slperrs[-rslt]);
	free(svcurl);
	LOG_FEND();
}

/**********************************************************************/
int
slp_startSA(
	ifMC(struct Lcomponent_s *Lcomp,)
	port_t port
)
{
	int rslt;

	LOG_FSTART();
	/* first open SLP */
	if (slph == NULL && (rslt = SLPOpen(NULL, false, &slph)) < 0) {
		acnlogmark(lgERR, "Cannot open SLP: %s\n", slperrs[-rslt]);
		return -1;
	}

	adhocport = port;
	rslt = slp_register(ifMC(Lcomp));
	LOG_FEND();
	return rslt;
}
/**********************************************************************/
void
slp_stopSA(void)
{
	LOG_FSTART();

	slp_deregister();
	SLPClose(slph);
	LOG_FEND();
}
/**********************************************************************/
char *
gethomedir(void)
{
	char *hp;
	struct passwd *pwd;
	
	if ((hp = getenv("HOME")) != NULL)
		return hp;

	if ((pwd = getpwuid(geteuid())) != NULL)
		return pwd->pw_dir;

	return NULL;
}

/**********************************************************************/
void
uacn_init(const char *cidstr)
{
	char *cp;
	int fd;
	int len;

	cp = gethomedir();
	if (cp == NULL) {
		acnlogmark(lgERR, "Cannot find HOME directory");
		exit(EXIT_FAILURE);
	}
	uacncfg = mallocx(strlen(cp) + strlen(cfgdir) + UUID_STR_SIZE + strlen(uacnsuffix));
	sprintf(uacncfg, "%s%s%s%s", cp, cfgdir, cidstr, uacnsuffix);
	len = 0;
	if ((fd = open(uacncfg, O_RDONLY)) >= 0) {
		if ((len = read(fd, uacn, ACN_UACN_SIZE - 1)) > 0) {
			if (uacn[len - 1] == '\n') --len;
			uacn[len] = 0;
		}
		close(fd);
	}
	if (len <= 0) {
		strcpy(uacn, uacn_dflt);
	}
}
/**********************************************************************/
void
uacn_change(const uint8_t *dp, int size)
{
	int fd;
	int rslt;

	memcpy(uacn, dp, size);
	uacn[size++] = '\n';
	uacn[size] = 0;
	fd = open(uacncfg, O_WRONLY);
	if (fd >= 0) {
		if (write(fd, uacn, size) < size) {
			acnlogerror(lgERR);
		}
		close(fd);
	}
	/* readvertise */
	if ((rslt = slp_register(ifMC(Lcomp))) < 0) {
		acnlogmark(lgERR, "Re-registering UACN: %s\n", slperrs[-rslt]);
	}
}
/**********************************************************************/
void
uacn_close()
{
	free(uacncfg);
}
