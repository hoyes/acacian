/**********************************************************************/
/*

	Copyright (C) 2011, Engineering Arts. All rights reserved.

	Author: Philip Nye

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
#include "acn.h"
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
		if (str2uuid(argv[1], NULL) == 0) break;
		/* fall through */
	case 0:
	default:
		acnlogmark(lgERR, "Usage: %s <root-DCID>", argv[0]);
		return EXIT_FAILURE;
	}
	init_behaviors();

	rootprop = parsedevice(argv[1]);

	printtree(&rootprop->prop);

	freerootprop(rootprop);
	//freemap(map);

	return 0;
}
