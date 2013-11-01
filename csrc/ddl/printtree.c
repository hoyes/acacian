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
#include "acn.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL
/**********************************************************************/
static const char prefix[] = "                                                ";
#define PFLEN() (_pdepth * 3)

void
printtree(struct prop_s *prop)
{
	struct prop_s *pp;
		int i;
	char buf[pflg_NAMELEN + pflg_COUNT];

FOR_EACH_PROP(prop) {
	printf("%.*s%s property", PFLEN(), prefix, ptypes[prop->vtype]);
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
		printf(" loc: %u, size %u\n", prop->v.net.dmp->addr, prop->v.net.dmp->size);
		fprintf(stdout, "%.*s - flags: %s\n", PFLEN(), prefix, flagnames(prop->v.net.dmp->flags, pflgnames, buf, " %s"));
		printf("%.*s - type/encoding: %s\n", PFLEN(), prefix, etypes[prop->v.net.dmp->etype]);
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
				printf("%.*s%4d = %u\n", PFLEN(), prefix, i, prop->v.imm.t.Aui[i]);
			break;
		case VT_imm_sint:
			if (prop->v.imm.count <= 1)
				printf(" = %d\n", prop->v.imm.t.si);
			else for (i = 0; i < prop->v.imm.count; ++i)
				printf("%.*s%4d = %d\n", PFLEN(), prefix, i, prop->v.imm.t.Asi[i]);
			break;
		case VT_imm_float:
			if (prop->v.imm.count <= 1)
				printf(" = %g\n", prop->v.imm.t.f);
			else for (i = 0; i < prop->v.imm.count; ++i)
				printf("%.*s%4d = %g\n", PFLEN(), prefix, i, prop->v.imm.t.Af[i]);
			break;
		case VT_imm_string:
			if (prop->v.imm.count <= 1)
				printf(" = \"%s\"\n", prop->v.imm.t.str);
			else for (i = 0; i < prop->v.imm.count; ++i)
				printf("%.*s%4d = \"%s\"\n", PFLEN(), prefix, i, prop->v.imm.t.Astr[i]);
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
					printf("%.*s%4d =", PFLEN(), prefix, i);
					for (j = 0; j < prop->v.imm.t.Aobj[i].size; ++j) {
						printf(" %02x", prop->v.imm.t.Aobj[i].data[j]);
					}
				}
			} break;
		default:
			break;
		}

		if (prop->v.imm.count > 1) printf("%.*s}\n", PFLEN(), prefix);		
		break;		
	}
} NEXT_PROP(prop)
	/*
	if (prop->children) {
		pfp = stpcpy(pfp, pfstr);
		for (pp = prop->children; pp != NULL; pp = pp->siblings)
			printtree(pp);
		*(pfp -= pflen) = 0;
	}
	*/
}
