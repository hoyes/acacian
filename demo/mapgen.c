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

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL
/**********************************************************************/

const char header[] =
"/**********************************************************************/\n"
"/*\n"
"\tAutomatically generated from DDL source\n"
"\n"
"\tDevice UUID = %s\n"
"\n"
"/**********************************************************************/\n"
"\n"
;

const char maphead[] =
"struct addrmap_s addrmap = {\n"
"\t.h = {.size = 0, .count = %u},\n"
"\t.map = {\n"
;

const char propent[] = 
"\t\t{.adlo = %u, .adhi = %u, .ntests = %i},\n"
;

const char mapfoot[] = 
"\t}\n"
"}\n\n"
;


void
printtests(struct addrmap_s *map)
{
}

void
printmap(struct addrmap_s *map)
{
	struct prop_s *pp;
	struct netprop_s *np;
	struct addrfind_s *af;
	union proportest_u *nxt;
	int i, j;
	int d;

	printf(maphead, map->h.count);
	for (i = 0, af = map->map; i < map->h.count; ++i, ++af) {
		j = af->ntests;
		nxt = &af->p;
		printf(propent, af->adlo, af->adhi, af->ntests);
		do {
			if (j <= 1) pp = nxt->prop;
			else {
				pp = nxt->test->prop;
				nxt = &nxt->test->nxt;
			}
			np = pp->v.net;
			if (pp->id) /* printf(" ID=\"%12.12s\" ", pp->id) */;
			//printf(" size=%u dims=%u", np->dmp.size, np->dmp.ndims);
			for (d = 0; d < np->dmp.ndims; ++d)
				printf(" [i=%d, n=%u]", np->dim[d].i, np->dim[d].r + 1);
			//putchar('\n');
			if (j > 1) fputs("              ", stdout);
		} while (--j > 0);
	}
	printf(footer);
}

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

	printf(header, argv[1]));
	
	printtests(rootprop->addrmap);
	printmap(rootprop->addrmap);

	freerootprop(rootprop);
	//freemap(map);

	return 0;
}
