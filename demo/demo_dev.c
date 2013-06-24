/**********************************************************************/
/*
Copyright (c) 2013, Philip Nye
All rights reserved.

	$Id$

#tabs=3t
*/
/**********************************************************************/

#include "acn.c"


#include <stdio.h>
#include <expat.h>
/*
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <slp.h>

*/
#include "acncommon.h"
#include "acnlog.h"
#include "acnmem.h"
#include "uuid.h"
#include "ddl/parse.h"
//#include "propmap.h"
#include "ddl/behaviors.h"
#include "ddl/printtree.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL


/**********************************************************************/
/*
topic: Command line options
*/
const char shortopts[] = "c:p:";
const struct option longopta[] = {
	{"cid", required_argument, NULL, 'c'},
	{"port", required_argument, NULL, 'p'},
	{NULL,0,NULL,0}
};
const uint16_t default_port = 56789;

/**********************************************************************/
uint8_t dcid[UUID_SIZE];

/**********************************************************************/
static SLPHandle slph = NULL;

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

#define SVC_URLLEN (sizeof("service:acn.esta:///uuuuuuuu-uuuu-uuuu-uuuu-uuuuuuuuuuuu") + 1)
#define ATT_CIDLEN (sizeof("(cid=uuuuuuuu-uuuu-uuuu-uuuu-uuuuuuuuuuuu)"))
#define ATT_FCTNLEN (sizeof(",(fctn=)") + ACN_FCTN_SIZE)
#define ATT_UACNLEN (sizeof(",(uacn=)") + ACN_UACN_SIZE)
#define ATT_ACNSVCLEN (sizeof(",(acn-services=esta.dmp)"))

/* space needed for an address and port string - port takes max of 5 digits */
#if defined(ACNCFG_NET_IPV6)
#define MAX_IPADSTR (sizeof("[aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa]"))
#elif defined(ACNCFG_NET_IPV4)
#define MAX_IPADSTR (sizeof("aaa.aaa.aaa.aaa"))
#endif

#define CSLOCLEN (sizeof("esta.sdt/:ppppp;esta.dmp/") + MAX_IPADSTR ifDMP_C(+ 1) ifDMP_D(+ UUID_STR_SIZE + 1))

#define MAX_CSLLEN (sizeof(",(csl-esta.dmp=)") + ACNCFG_MAX_IPADS * (CSLOCLEN + 1))
/**********************************************************************/
char *
makesvc(
	ifMC(struct Lcomponent_s *Lcomp,)
	port_t port,
	ifDMP_D(char *dcidstr,)
	char *interfaces[]
) {
#if !defined(ACNCFG_MULTI_COMPONENT)
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif
	int rslt;
	char *buffer;
	char *bp;
	char **ipstrs;
	char **ipstrp;
#if defined(ACNCFG_DMP_DEVICE)
	char dmploc[UUID_STR_SIZE + 3];  /* space for "cd:uuidstr" */
#else
	const static char dmploc[] = "c";
#endif

	buffer = mallocx(SVC_URLLEN + ATT_CIDLEN + ATT_FCTNLEN + ATT_UACNLEN + ATT_ACNSVCLEN + MAX_CSLLEN);
	/*
	Get suitable addresses
	*/
	ipstrs = netx_getmyipstr(interfaces, GIPF_DEFAULT, GIPF_DEFAULT, ACNCFG_MAX_IPADS);
	if (!ipstrs) {
		return -1;
	}

	port = NTOHS(port);  /* byteswap once if necessary */

	/* make our DMP declaration */
#if defined(ACNCFG_DMP_DEVICE)
	if (str2uuid(dcidstr, NULL) < 0) {
		acnlogmark(lgERR, "illegal DCID \"%s\"", dcidstr);
		return -1;
	}
	bp = dmploc;
#if defined(ACNCFG_DMP_CONTROLLER)
	*bp++ = 'c';
#endif
	*bp++ = 'd';
	*bp++ = ':';
	strcpy(bp, dcidstr);
#endif

	/* generate the service URL */
	rslt = snprintf(buffer, sizeof(buffer), "service:acn.esta:///%s", Lcomp->uuidstr);
	assert(rslt == SVC_URLLEN - 1);
	bp = buffer + SVC_URLLEN;

	/* now attributes */
	/* cid, fctn, uacn and start of csl-dmp */
	bp += snprintf(bp, 
					"(cid=%s),(fctn=%s),(uacn=%s),(acn-services=esta.dmp),(csl-esta.dmp=",
					Lcomp->uuidstr, Lcomp->fctn, Lcomp->uacn);

	/* now csl-dmp values - one per ip address */
	for (ipstrp = ipstrs; *ipstrp; ++ipstrp) {
		bp += sprintf(bp, "esta.sdt/%s:%hu;esta.dmp/%s," *ipstrp, port, dmploc);
		free(*ipstrp);
	}
	free(ipstrs);
	*(bp - 1) = ')';  /* overwrite trailing comma */
	buffer = realloc(buffer, bp - buffer + 1);
	return buffer;
}
/**********************************************************************/
int
slp_start_sa(
	ifMC(struct Lcomponent_s *Lcomp,)
	port_t port,
	char *dcidstr,
	bool iscontroller,
	char *interfaces[]
)
{
#if !defined(ACNCFG_MULTI_COMPONENT)
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif
	int rslt;
	char buffer[MAX_MTU];
	char *bp;
	const char *bufend = buffer + sizeof(buffer);
	netx_addr_t ipads[ACNCFG_MAX_IPADS];
	int nipads;
	int i;
	char *attp;
	char ipadstr[MAX_ADPSTR + 1];
	char dmploc[UUID_STR_SIZE + 3];  /* space for "cd:uuidstr" */

	/* first open SLP */
	if (slph == NULL && (rslt = SLPOpen(NULL, true, &slph)) < 0) {
		acnlogmark(lgERR, "Cannot open SLP: %s\n", slperrs[-rslt]);
		return -1;
	}

	/*
	Get suitable addresses
	*/
	nipads = netx_getmyip(interfaces, GIPF_DEFAULT, GIPF_DEFAULT, (void *)ipads, sizeof(ipads));
	if (nipads <= 0) return -1;


	port = NTOHS(port);  /* byteswap once if necessary */

	/* make our DMP declaration */
	if (!iscontroller && !dcidstr) {
			acnlogmark(lgERR, "DMP component must be controller or device (or both)");
			return -1;
	}
	bp = dmploc;
	if (iscontroller) *bp++ = 'c';
	if (dcidstr) {
		if (str2uuid(dcidstr, NULL) < 0) {
			acnlogmark(lgERR, "illegal DCID \"%s\"", dcidstr);
			return -1;
		}
		bp += sprintf(bp, "d:%s", dcidstr);
	}

	/* first generate the service URL */
	rslt = snprintf(buffer, sizeof(buffer), "service:acn.esta:///%s", Lcomp->uuidstr);
	assert(rslt == SVC_URLLEN - 1);
	bp = buffer + SVC_URLLEN;

	/* now attributes */
	/* cid, fctn, uacn and start of csl-dmp */
	bp += snprintf(bp, 
					"(cid=%s),(fctn=%s),(uacn=%s),(acn-services=esta.dmp),(csl-esta.dmp=",
					Lcomp->uuidstr, Lcomp->fctn, Lcomp->uacn);
	/* now csl-dmp values - one per ip address */
	for (i = 0; i < nipads; ++i) {
		if ((bufend - bp) < MAX_CSLLEN) {
			acnlogerror(lgERR);
			return -1;
		}
		if (i != 0) *bp++ = ',';
		bp = stpcpy(bp, "esta.sdt/");
		if (!inet_ntop(netx_TYPE(ipads + i), &netx_SINADDR(ipads + i), bp, bufend - bp)) {
			acnlogerror(lgERR);
			return -1;
		}
		while (*++bp) /* empty */;  /* find the end of string */
		bp += sprintf(bp, ":%uh;esta.dmp/", port);
		if (iscontroller) *bp++ = 'c';
		if (dcidstr) {
			bp += sprintf(bp, "d:%s", dcidstr);
		}
	}
	if (bp >= bufend - 1) {
		acnlogerror(lgERR);
		return -1;
	}
	*bp++ = ')'
	*bp = 0;
}

/**********************************************************************/
void
run_device(const char *uuidstr, uint16_t port)
{
	netx_addr_t listenaddr;
	char *service_url;
	char *service_atts;

	/* initialize our component */
	if (!initstr_Lcomponent(uuidstr, dev_fctn, dev_uacn)) return;

	/* set up for ephemeral port and any interface */
	netx_INIT_ADDR_ANY(&listenaddr, port);

	/* start up ACN */
	if (!dmp_start(&listenaddr, unsigned int flags)) return;

	/* now we can advertise ourselves */
	
}

/**********************************************************************/
/*
func: main

Parse command line options and start device
*/
int
main(int argc, char *argv[])
{
	int opt;
	const char *uuidstr = NULL;
	uint16_t port = netx_PORT_EPHEM;

	while ((opt = getopt_long(argc, argv, "u:", longopta, NULL)) != -1) {
		switch (opt) {
		case 'u':
			uuidstr = optarg;
			break;
		case 'p': {
			const char *ep = NULL;
			unsigned long u;

			u = strtoul(optarg, &ep, 0);
			if (ep && *ep == 0 && u <= 0xffff) {
				port = u;
			} else {
				fprintf(stderr, "Bad port specification \"%s\" ignored\n", optarg);
				port = netx_PORT_EPHEM;
			}
			break;
		}
		case '?':
		default:
			exit(EXIT_FAILURE);
		}
	}
	if (uuidstr == NULL) {
		fprintf(stderr, "No CID specified\n");
		exit(EXIT_FAILURE);
	} else if (str2uuid(uuidstr, NULL) != 0) {
		fprintf(stderr, "Cannot parse CID \"%s\"\n", uuidstr);
		exit(EXIT_FAILURE);
	}

	slp_start();
	run_device(uuidstr, port);

	return 0;
}
