/**********************************************************************/
/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (c) 2013, Acuity Brands, Inc.

Author: Philip Nye <philip.nye@engarts.com>

This file forms part of Acacian a full featured implementation of 
ANSI E1.17 Architecture for Control Networks (ACN)

#tabs=3
*/
/**********************************************************************/
/*
file: ddltree.c

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
#include "printtree.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL
/**********************************************************************/
const char *ltags[] = {
	"en-GB",
	"en-US",
	NULL
};

int
main(int argc, char *argv[])
{
	struct rootdev_s *rootdev;
	int i;

	for (i = 1; i < argc; ++i) {
		rootdev = parseroot(argv[i]);
		if (rootdev == NULL) {
			printf("No valid root device in %s\n", argv[i]);
		} else {
			setlang(ltags);
			printf("\n"
				"Start DDL device tree.\n"
				"=====================\n"
				"\n");
			printtree(stdout, rootdev->ddlroot);
	
			printf("\n"
				"End DDL device tree.\n"
				"===================\n"
				"\n");
			freerootdev(rootdev);
		}	
	}

	return 0;
}
