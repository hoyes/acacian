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
#include "acncommon.h"
#include "acnlog.h"
#include "acnmem.h"
#include "propmap.h"
#include "uuid.h"
#include "ddl/parse.h"
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
	struct dcxt_s dcxt;

	memset(&dcxt, 0, sizeof(dcxt));
	dcxt.curdev = dcxt.lastdev = &dcxt.rootprop;
	dcxt.rootprop.vtype = VT_include;
	/* dcxt.elestack[0] = ELx_ROOT; // ELx_ROOT is zero so no need to initialize */

	switch (argc) {
	case 2:
		str2uuid(argv[1], dcxt.rootprop.v.dev.uuid);
		break;
	case 0:
	default:
		acnlogmark(lgERR, "Usage: %s <root-DCID>", argv[0]);
		return EXIT_FAILURE;
	}
	register_base_bvs();

	for ( ; dcxt.curdev != NULL; dcxt.curdev = dcxt.curdev->v.dev.nxtdev) {
		parsedevx(&dcxt);
	}

	acnlogmark(lgDBUG, "Found %d net properties", dcxt.nprops);
	printtree(&dcxt.rootprop, 0);
	freedev(&dcxt);

	return 0;
}
