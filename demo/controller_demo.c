/**********************************************************************/
/*
Copyright (c) 2013, Philip Nye
All rights reserved.

  $Id$

#tabs=3
*/
/**********************************************************************/
#include <stdio.h>
#include <ctype.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


/*
#include <expat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

*/
#include "acn.h"
#include "demo_utils.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL
/**********************************************************************/
/*
Fix some values
*/

const char fctn[] = "Acacian controller demo";
const char uacn_dflt[] = "Unnamed demo controller";

#define MAXINTERFACES 8
const char **interfaces;
/**********************************************************************/
struct Lcomponent_s localComponent = {
	.fctn = fctn,
	.uacn = uacn,
	.dmp = {
		.rxvec = {
			[DMP_reserved0]             = NULL,
			[DMP_GET_PROPERTY]          = NULL,
			[DMP_SET_PROPERTY]          = NULL,
			[DMP_GET_PROPERTY_REPLY]    = NULL,
			[DMP_EVENT]                 = NULL,
			[DMP_reserved5]             = NULL,
			[DMP_reserved6]             = NULL,
			[DMP_SUBSCRIBE]             = NULL,
			[DMP_UNSUBSCRIBE]           = NULL,
			[DMP_GET_PROPERTY_FAIL]     = NULL,
			[DMP_SET_PROPERTY_FAIL]     = NULL,
			[DMP_reserved11]            = NULL,
			[DMP_SUBSCRIBE_ACCEPT]      = NULL,
			[DMP_SUBSCRIBE_REJECT]      = NULL,
			[DMP_reserved14]            = NULL,
			[DMP_reserved15]            = NULL,
			[DMP_reserved16]            = NULL,
			[DMP_SYNC_EVENT]            = NULL,
		},
		.flags = 0,
	}
};

/**********************************************************************/
void cd_sdtev(int event, void *object, void *info)
{

	LOG_FSTART();
	switch (event) {
	case EV_RCONNECT:  /* object = Lchan, info = memb */
	case EV_LCONNECT:  /* object = Lchan, info = memb */
	case EV_REMDISCONNECT:  /* object = Lchan, info = memb */
	case EV_LOCDISCONNECT:  /* object = Lchan, info = memb */
	case EV_DISCOVER:  /* object = Rcomp, info = discover data in packet */
	case EV_JOINSUCCESS:  /* object = Lchan, info = memb */
	case EV_JOINFAIL:  /* object = Lchan, info = memb->rem.Rcomp */
	case EV_LOCCLOSE:  /* object = , info =  */
	case EV_LOCLEAVE:  /* object = Lchan, info = memb */
	case EV_LOSTSEQ:  /* object = Lchan, info = memb */
	case EV_MAKTIMEOUT:  /* object = Lchan, info = memb */
	case EV_NAKTIMEOUT:  /* object = Lchan, info = memb */
	case EV_REMLEAVE:  /* object = , info =  */
	default:
		break;
	}
	LOG_FEND();
}

/**********************************************************************/

/**********************************************************************/
static bool
wordmatch(char **str, const char *word, int minlen)
{
	char *bp;

	bp = *str;
	for (bp = *str; *word && *bp == *word; ++bp, ++word, --minlen);

	if ((*bp == 0 || *bp == ' ') && (*word == 0 || minlen <= 0)) {
		*str = bp;
		return true;
	}
	return false;
}
/**********************************************************************/
void
ddltree(char **bpp)
{
	char *bp;
	uint8_t cid[UUID_SIZE];
	struct Rcomponent_s *Rcomp;

	fprintf(stdout, "DDL tree\n");
	bp = *bpp;
	while (bp && isspace(*bp)) ++bp;
	if (str2uuid(bp, cid) != 0) {
		fprintf(stdout, "Can't parse UUID \"%s\"\n", bp);
		*bpp = NULL;
	} else {
		Rcomp = findRcomp(cid);
		if (Rcomp == NULL) {
			fprintf(stdout, "Not found\n");
		} else {
			fprintf(stdout, "Found\n");
		}
	}
}
/**********************************************************************/
void
dodiscover()
{
	char *ctyp;
	struct Rcomponent_s *Rcomp;
	int i;
	char uuidstr[UUID_STR_SIZE];

	discover();
	if (Rcomponents.first == NULL) {
		fprintf(stdout, "Nothing discovered\n");
	} else {
		i = 0;
	
		FOR_EACH_UUID(&Rcomponents, Rcomp, struct Rcomponent_s, uuid[0]) {

			switch (Rcomp->slp.flags & (slp_ctl | slp_dev)) {
			case slp_ctl:
				ctyp = "controller";
				break;
			case slp_dev:
				ctyp = "device";
				break;
			case slp_ctl | slp_dev:
				ctyp = "controller+device";
				break;
			default:
				ctyp = "";
				break;
			}
			fprintf(stdout, "%3i:  %s %s \"%s\" at %s:%-5d  [%.8s]\n",
				i,
				Rcomp->slp.fctn,
				ctyp,
				Rcomp->slp.uacn,
				inet_ntoa(netx_SINADDR(&Rcomp->sdt.adhocAddr)),
				ntohs(netx_PORT(&Rcomp->sdt.adhocAddr)),
				uuid2str(Rcomp->uuid, uuidstr)
			);
			++i;

		} NEXT_UUID()
	}
}
/**********************************************************************/
void
term_event(uint32_t evf, void *evptr)
{
	char buf[256];
	int len;
	char *bp;

	len = read(STDIN_FILENO, buf, sizeof(buf) - 1);  /* waiting input */
	if (len < 0) {
		acnlogmark(lgWARN, "read stdin error %d %s", len, strerror(errno));
		return;
	}

	if (buf[len - 1] == '\n') --len;
	buf[len] = 0;	 /* terminate */

	bp = buf;

	if (wordmatch(&bp, "discover", 2)) dodiscover();
	else if (wordmatch(&bp, "describe", 2) || wordmatch(&bp, "ddl", 2))
		ddltree(&bp);
	else if (wordmatch(&bp, "quit", 1)) runstate = rs_quit;
	else {
		fprintf(stdout, "Bad command \"%s\"\n", bp);
	}
}

poll_fn * term_event_ref = &term_event;

/**********************************************************************/
static void
run_controller(const char *uuidstr, uint16_t port)
{
	netx_addr_t listenaddr;

	LOG_FSTART();
	/* initialize our component */
	if (initstr_Lcomponent(uuidstr) != 0) {
		acnlogmark(lgERR, "Init local component failed");
		return;
	}

	/* read uacn if its been set */
	uacn_init(uuidstr);

	/* set up for ephemeral port and any interface */
	netx_INIT_ADDR_ANY(&listenaddr, port);

	/* start up ACN */
	/* prepare DMP */
	if (dmp_register() < 0) {
		acnlogerror(lgERR);
	} else if (sdt_register(&cd_sdtev) < 0) {
		acnlogerror(lgERR);
	} else {
		if (sdt_setListener(NULL, &listenaddr) < 0) {
			acnlogerror(lgERR);
		} else {
			if (sdt_addClient(&dmp_sdtRx, NULL) < 0) {
				acnlogerror(lgERR);
			} else {
				/* now we can advertise ourselves */
				acnlogmark(lgDBUG, "starting SLP");
				slp_register(interfaces);
			
				evl_register(STDIN_FILENO, &term_event_ref, EPOLLIN);

				evl_wait();
			
				evl_register(STDIN_FILENO, NULL, 0);

				slp_deregister();
			}
			sdt_clrListener();
		}
		sdt_deregister();
	}
	uacn_close();
	LOG_FEND();
}

/**********************************************************************/
/*
topic: Command line options
*/
const char shortopts[] = "c:p:i:";
const struct option longopta[] = {
	{"cid", required_argument, NULL, 'c'},
	{"port", required_argument, NULL, 'p'},
	{"interface", required_argument, NULL, 'i'},
	{NULL,0,NULL,0}
};
/**********************************************************************/
/*
func: main

Parse command line options and start controller
*/
int
main(int argc, char *argv[])
{
	int opt;
	const char *uuidstr = NULL;
	long int port = netx_PORT_EPHEM;
	const char *interfacesb[MAXINTERFACES + 1];
	int ifc = 0;
	
	LOG_FSTART();

	while ((opt = getopt_long(argc, argv, shortopts, longopta, NULL)) != -1) {
		switch (opt) {
		case 'c':
			uuidstr = optarg;
			break;
		case 'p': {
			char *ep = NULL;
			unsigned long u;

			u = strtoul(optarg, &ep, 0);
			if (ep && *ep == 0 && u <= 0xffff) {
				port = htons(u);
			} else {
				acnlogmark(lgERR, "Bad port specification \"%s\" ignored\n", optarg);
				port = netx_PORT_EPHEM;
			}
			break;
		case 'i':
			if (ifc < MAXINTERFACES) interfacesb[ifc++] = optarg;
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
	//snprintf(serialno, sizeof(serialno), "%.8s:%.8s", DCID_STR, uuidstr);

	interfacesb[ifc] = NULL;  /* terminate */
	interfaces = interfacesb;
	run_controller(uuidstr, port);

	LOG_FEND();
	return 0;
}
