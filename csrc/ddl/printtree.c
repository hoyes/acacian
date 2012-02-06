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
		printf(" loc: %u, size %u\n", prop->v.net.addr, prop->v.net.size);
		fprintf(stdout, "%s- flags: %s\n", prefix, flagnames(prop->v.net.flags & ~pflg_valid));
		printf("%s- type/encoding: %s\n", prefix, etypes[prop->v.net.etype]);
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
printmap(struct proptab_s *map)
{
	struct prop_s *pp;
	struct propfind_s *pf;
	uint32_t i;

	for ( ; map != NULL; map = map->nxt) {
		if (map->inc == 1)
			printf("Property Map: single properties and arrays with increment 1\n");
		else
			printf("Property Map: arrays with increment %d\n", map->inc);
		for (pf = map->props, i = 0; i++ < map->nprops; ++pf) {
			pp = pf->prop;
			if (pf->count == 1)
				printf("%8u  size %-5u addr %u\n", i, pp->v.net.size, pf->lo);
			else 
				printf("%8u  size %-5u addr %u - %u\n", i, pp->v.net.size, pf->lo, pf->lo + (pf->count - 1) * map->inc);
		}
	}
}
