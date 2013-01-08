/**********************************************************************/
/*
Copyright (c) 2013, Philip Nye
All rights reserved.

  $Id$

#tabs=3
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

int
main(int argc, char *argv[])
{
	rootprop_t *rootprop;

	switch (argc) {
	case 2:
		if (quickuuidOKstr(argv[1])) break;
		/* fall through */
	case 0:
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
