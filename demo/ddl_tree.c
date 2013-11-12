/**********************************************************************/
/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (c) 2013, Acuity Brands, Inc.

Author: Philip Nye <philip.nye@engarts.com>

#tabs=3
*/
/**********************************************************************/
/*
about: Acacian

Acacian is a full featured implementation of ANSI E1.17 2012
Architecture for Control Networks (ACN) from Acuity Brands

file: ddl_tree.c

Simple demonstration DDL tree print.
*/


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
	struct rootdev_s *rootdev;

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

	rootdev = parsedevice(argv[1]);

	printtree(rootdev->ddlroot);

	freerootdev(rootdev);
	//freemap(map);

	return 0;
}
