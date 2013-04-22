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
const char shortopts[] = "u:p:";
const struct option longopta[] = {
	{"uuid", 1, NULL, 'u'},
	{"port", 1, NULL, 'p'},
	{NULL,0,NULL,0}
};


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
	uint16_t port = 0;

	while ((opt = getopt_long(argc, argv, "u:", longopta, NULL)) != -1) {
		switch (opt) {
		case 'u':
			if (str2uuid(optarg, NULL) == 0) {
				uuidstr = optarg;
			} else {
				fprintf(stderr, "Bad format UUID \"%s\" ignored\n", optarg);
			}
			break;
		case 'p': {
			const char *ep = NULL;
			unsigned long u;

			u = strtoul(optarg, &ep, 0);
			if (ep && *ep == 0 && (u & ~0xffff) == 0) {
				port = u;
			} else {
				fprintf(stderr, "Bad port specification \"%s\" ignored\n", optarg);
			}
			break;
		}
		case '?':
		default:
			exit(EXIT_FAILURE);
		}
	}
	if (uuidstr == NULL) {
		uuidstr = getenv("DEVICE_UUID");
		if (uuidstr && str2uuid(uuidstr, NULL) != 0) {
			fprintf(stderr, "Bad environment DEVICE_UUID=\"%s\" ignored\n", uuidstr);
			uuidstr = NULL;
		}
		if (uuidstr == NULL) {
			int fd;

			uuidstr = mallocx(UUID_STR_SIZE);
			
		}
	}


	switch (argc) {
	case 2:
		if (quickuuidOKstr(argv[1])) break;
	default:
		acnlogmark(lgERR, "Usage: %s <root-DCID>", argv[0]);
		return EXIT_FAILURE;
	}
	init_behaviors();

	rootprop = parsedevice(argv[1]);

	printtree(&rootprop->prop);
	printmap(rootprop->addrmap);

	freerootprop(rootprop);
	//freemap(map);

	return 0;
}
