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
static const char prefix[] = ".  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  ";
#define PFLEN() (_pdepth * 3)
void
printtree(struct ddlprop_s *prop)
{
	int i;
	char buf[pflg_NAMELEN + pflg_COUNT];

FOR_EACH_PROP(prop) {
	const char *propid;
	char array[32];

	propid = prop->id;
	if (propid == NULL) propid = "";
	array[0] = 0;
	if (prop->array > 1) {
#if ACNCFG_DDLACCESS_DMP
		int32_t inc;

		if (prop->vtype == VT_network) {
			inc = prop->inc;
		} else {
			inc = prop->childinc;
		}
		if (inc > 1) sprintf(array, "[%u:%i]", prop->array, inc);
		else
#endif
		sprintf(array, "[%u]", prop->array);
	}

	switch (prop->vtype) {
	case VT_NULL:
		printf("%.*sContainer: %s%s\n", PFLEN(), prefix, propid, array);
		break;
	case VT_implied:
		printf("%.*sImplied: %s%s\n", PFLEN(), prefix, propid, array);
		break;
	case VT_device:
		if (prop->parent == NULL) {
			printf("%.*sRoot device: %s%s\n", PFLEN(), prefix, propid, array);
		} else {
			printf("%.*sSubdevice: %s%s\n", PFLEN(), prefix, propid, array);
		}
		break;
	case VT_include:
		fputc('\n', stdout);
		break;
	case VT_imm_unknown:
		printf("%.*sValue (unknown type): %s%s\n", PFLEN(), prefix, propid, array);
		break;
	case VT_network:
		printf("%.*sProperty: %s%s (%s)", PFLEN(), prefix, propid, array, etypes[prop->v.net.dmp->etype]);
		printf(" loc: %u, size %u\n", prop->v.net.dmp->addr, prop->v.net.dmp->size);
		fprintf(stdout, "%.*s+- flags: %s\n", PFLEN(), prefix, flagnames(prop->v.net.dmp->flags, pflgnames, buf, " %s"));
		break;
	default:
		printf("Error: unknown property type!\n");
		break;

	case VT_imm_uint:
	case VT_imm_sint:
	case VT_imm_float:
	case VT_imm_string:
	case VT_imm_object:

		printf("%.*sValue: %s%s (%s)", PFLEN(), prefix, propid, array, 
			ptypes[prop->vtype] + sizeof("immediate"));
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
}
