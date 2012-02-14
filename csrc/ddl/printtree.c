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
#include "acnmem.h"
*/
#include "acncommon.h"
#include "acnlog.h"
#include "uuid.h"
#include "ddl/parse.h"
#include "propmap.h"
#include "ddl/behaviors.h"
#include "ddl/resolve.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL
/**********************************************************************/
#define MAXPREFIX 60
static char prefix[MAXPREFIX];
static char *pfp = prefix;
static const char pfstr[] = "   ";
static const int pflen = sizeof(pfstr) - 1;

void
printtree(struct prop_s *prop)
{
	struct prop_s *pp;
		int i;

	printf("%s%s property", prefix, ptypes[prop->vtype]);
	if (prop->id) printf(" ID: %s", prop->id);

	switch (prop->vtype) {
	case VT_NULL:
	case VT_implied:
	case VT_device:
	case VT_include:
		fputc('\n', stdout);
		break;
	case VT_imm_unknown:
		printf(" value: Unknown\n");
		break;
	case VT_network:
		printf(" loc: %u, size %u\n", prop->v.net->dmp.addr, prop->v.net->dmp.size);
		fprintf(stdout, "%s- flags: %s\n", prefix, flagnames(prop->v.net->dmp.flags));
		printf("%s- type/encoding: %s\n", prefix, etypes[prop->v.net->dmp.etype]);
		break;
	default:
		printf("unknown type!\n");
		break;

	case VT_imm_uint:
	case VT_imm_sint:
	case VT_imm_float:
	case VT_imm_string:
	case VT_imm_object:

		if (prop->v.imm.count > 1)
			printf(" = array[%u] {\n", prop->v.imm.count);

		switch (prop->vtype) {
		case VT_imm_uint:
			if (prop->v.imm.count <= 1)
				printf(" = %u\n", prop->v.imm.t.ui);
			else for (i = 0; i < prop->v.imm.count; ++i)
				printf("%s%4d = %u\n", prefix, i, prop->v.imm.t.Aui[i]);
			break;
		case VT_imm_sint:
			if (prop->v.imm.count <= 1)
				printf(" = %d\n", prop->v.imm.t.si);
			else for (i = 0; i < prop->v.imm.count; ++i)
				printf("%s%4d = %d\n", prefix, i, prop->v.imm.t.Asi[i]);
			break;
		case VT_imm_float:
			if (prop->v.imm.count <= 1)
				printf(" = %g\n", prop->v.imm.t.f);
			else for (i = 0; i < prop->v.imm.count; ++i)
				printf("%s%4d = %g\n", prefix, i, prop->v.imm.t.Af[i]);
			break;
		case VT_imm_string:
			if (prop->v.imm.count <= 1)
				printf(" = \"%s\"\n", prop->v.imm.t.str);
			else for (i = 0; i < prop->v.imm.count; ++i)
				printf("%s%4d = \"%s\"\n", prefix, i, prop->v.imm.t.Astr[i]);
			break;
		case VT_imm_object:
			{
				int j;

				if (prop->v.imm.count <= 1) {
					printf(" =");
					for (j = 0; j < prop->v.imm.t.obj.size; ++j) {
						printf(" %02x", prop->v.imm.t.obj.data[j]);
					}
					fputc('\n', stdout);
				} else for (i = 0; i < prop->v.imm.count; ++i) {
					printf("%s%4d =", prefix, i);
					for (j = 0; j < prop->v.imm.t.Aobj[i].size; ++j) {
						printf(" %02x", prop->v.imm.t.Aobj[i].data[j]);
					}
				}
			} break;
		default:
			break;
		}

		if (prop->v.imm.count > 1) printf("%s}\n", prefix);		
		break;		
	}
	if (prop->children) {
		pfp = stpcpy(pfp, pfstr);
		for (pp = prop->children; pp != NULL; pp = pp->siblings)
			printtree(pp);
		*(pfp -= pflen) = 0;
	}
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

	for (i = 0, af = map->map; i < map->h.count; ++i, ++af) {
		j = af->ntests;
		nxt = &af->p;
		printf("%8u addr %9u ..%9u: %s", i, af->adlo, af->adhi,
				j ? "test props" : "found prop");
		do {
			if (j <= 1) pp = nxt->prop;
			else {
				pp = nxt->test->prop;
				nxt = &nxt->test->nxt;
			}
			np = pp->v.net;
			if (pp->id) printf(" ID=\"%12.12s\" ", pp->id);
			printf(" size=%u dims=%u", np->dmp.size, np->dmp.ndims);
			for (d = 0; d < np->dmp.ndims; ++d)
				printf(" [i=%d, n=%u]", np->dim[d].i, np->dim[d].r + 1);
			putchar('\n');
			if (j > 1) fputs("              ", stdout);
		} while (--j > 0);
	}
}
