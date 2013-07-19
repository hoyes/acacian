/**********************************************************************/
/*
Copyright (c) 2013, Philip Nye
All rights reserved.

	$Id$

#tabs=3t
*/
/**********************************************************************/

#include "acn.h"


#include <stdio.h>
#include <expat.h>
#include <unistd.h>
#include <getopt.h>
/*
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

*/
//#include "ddl/parse.h"
//#include "propmap.h"
//#include "ddl/behaviors.h"
//#include "ddl/printtree.h"
#include "demo_utils.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_APP


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
const uint16_t default_port = 56789;

/**********************************************************************/
/*
Fix some values
*/

static const char DCID[UUID_STR_SIZE] = "dbc670da-e228-11e2-b379-0017316c497d";
static const char FCTN[] = "EAACN demonstration device";
static char UACN[ACN_UACN_SIZE] = "EAACN demo device (default name)";

/**********************************************************************/
static void
run_device(const char *uuidstr, uint16_t port, const char *interfaces[])
{
	netx_addr_t listenaddr;
	char *service_url;
	char *service_atts;
	uint8_t dcid[UUID_SIZE];

	/* get DCID in binary and text */
	str2uuid(DCID, dcid);

	/* initialize our component */
	if (initstr_Lcomponent(uuidstr, FCTN, UACN) != 0) {
		acnlogmark(lgERR, "Init local component failed");
		return;
	}

	/* set up for ephemeral port and any interface */
	netx_INIT_ADDR_ANY(&listenaddr, port);

	/* start up ACN */
	if (!dmp_register(&listenaddr)) return;

	/* now we can advertise ourselves */
	acnlogmark(lgDBUG, "starting SLP");
	slp_start_sa(netx_PORT(&listenaddr), DCID, interfaces);
	while (1) {
		sleep(1);
	}
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
	const char *interfaces[8];
	int ifc;

	ifc = 0;
	memset(interfaces, 0, sizeof(interfaces));

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
			interfaces[ifc++] = optarg;
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

	run_device(uuidstr, port, interfaces);

	return 0;
}
