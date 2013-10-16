/**********************************************************************/
/*
Copyright (c) 2013, Philip Nye
All rights reserved.

  $Id$

#tabs=3
*/
/**********************************************************************/
#include <stdio.h>
#include <expat.h>
/*
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

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
void cd_sdtev(int event, void *object, void *info)
{

	LOG_FSTART();
	switch (event) {
	case EV_RCONNECT:  /* object = Lchan, info = memb */
	case EV_LCONNECT:  /* object = Lchan, info = memb */
	{
		struct Lchannel_s *Lchan = (struct Lchannel_s *)object;

		if (evcxt && evcxt->dest == Lchan) { 
			/* new connection in event channel */
			struct adspec_s flatads = {0, 1, DIM_bargraph__0};
			struct adspec_s dmpads = {DMP_bargraph.addr, INC_bargraph__0, DIM_bargraph__0};
	
			declare_bars(evcxt, &dmpads, &flatads, DMP_SYNC_EVENT);
			dmp_flushpdus(evcxt);
		}
	}	break;
	case EV_REMDISCONNECT:  /* object = Lchan, info = memb */
	case EV_LOCDISCONNECT:  /* object = Lchan, info = memb */
	{
		struct member_s *memb = (struct member_s *)info;
		struct Lchannel_s *Lchan = (struct Lchannel_s *)object;

		if (evcxt && evcxt->dest == Lchan) drop_subscriber(memb);
	}	break;
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
				slp_startSA(netx_PORT(&listenaddr));
			
				termsetup();
				showbars();
			
				evl_wait();
			
				termrestore();
				slp_stopSA();
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
			if (ifc < MAXINTERFACES) interfaces[ifc++] = optarg;
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
	snprintf(serialno, sizeof(serialno), "%.8s:%.8s", DCID_STR, uuidstr);

	run_controller(uuidstr, port);

	LOG_FEND();
	return 0;
}
