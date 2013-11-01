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
#define Ctl(x) (x + 1 - 'A')
#define ESC Ctl('[')

void
term_event(uint32_t evf, void *evptr)
{
	char buf[256];
	int i;
	int n;
	int c;
	static unsigned int escp = 0;
	int unsigned escn;
	//static int csav;

	n = read(STDIN_FILENO, buf, sizeof(buf));  /* read character or control sequence */
	if (n < 0) {
		acnlogmark(lgWARN, "read stdin error %d %s", n, strerror(errno));
		return;
	}
	for (i = 0; i < n; ++i) {
		c = buf[i] & 0xff;
		if (isgraph(c)) {
			acnlogmark(lgDBUG, "char '%c'", c);
		} else {
			acnlogmark(lgDBUG, "char 0x%02x", c);
		}
		escn = 0;  /* reset escape state by default */
		switch (escp) {
		case 0:	/* normal case */
			switch (c) {
			case Ctl('C'): case Ctl('D'): case Ctl('Q'):
			case 'q': case 'Q':
				runstate = rs_quit;
				break;
			}
			break;
		}
		escp = escn;
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
				slp_register();
			
				evl_register(STDIN_FILENO, &term_event_ref, EPOLLIN);

				discover();

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
	//snprintf(serialno, sizeof(serialno), "%.8s:%.8s", DCID_STR, uuidstr);

	run_controller(uuidstr, port);

	LOG_FEND();
	return 0;
}
