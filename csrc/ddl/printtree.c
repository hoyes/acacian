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
file: printtree.c

Utility to display the property tree generated by the DDL parser.
*/

#include <stdio.h>
#include <expat.h>
/*
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "acnmem.h"
*/
#include "acn.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL

/**********************************************************************/
const ddlchar_t *pheadings[] = {
	[VT_NULL] = "Container",
	[VT_imm_unknown] = "Value (unknown type)",
	[VT_implied] = "Implied",
	[VT_network] = "DMP Property",
	[VT_include] = "error",
	[VT_device] = "Subdevice",
	[VT_alias] = "Pointer",
	[VT_imm_uint] = "Value",
	[VT_imm_sint] = "Value",
	[VT_imm_float] = "Value",
	[VT_imm_string] = "Value",
	[VT_imm_object] = "Value (object)",
};

/**********************************************************************/
static const char prefix[] = ".  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  ";
#define PFLEN(n) ((_pdepth + (n)) * 3)
void
printtree(FILE *ofile, struct ddlprop_s *prop)
{
	int i;
	char buf[pflg_NAMELEN + pflg_COUNT];
	const char *pname;
	const char *lbl;
	const char *hd;

FOR_EACH_PROP(prop) {
	char array[32];

	pname = propxname(prop);
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
	lbl = lblookup(&prop->label);
	if (prop->vtype == VT_device && prop->parent == NULL)
		hd = "Root device";
	else
		hd = pheadings[prop->vtype];

	fprintf(ofile, "%.*s%s: %s%s", PFLEN(0), prefix, hd, pname, array);
	if (prop->vtype == VT_alias) {
		if (prop->v.alias == NULL) {
			fprintf(ofile, " (broken reference)");
		} else {
			fprintf(ofile, " to %s", propxpath(prop->v.alias));
		}
	} else if (prop->vtype >= VT_imm_FIRST && prop->v.imm.count <= 1) {
		switch (prop->vtype) {
		case VT_imm_uint:
			fprintf(ofile, " = %u", prop->v.imm.t.ui);
			break;
		case VT_imm_sint:
			fprintf(ofile, " = %d",  prop->v.imm.t.si);
			break;
		case VT_imm_float:
			fprintf(ofile, " = %g",  prop->v.imm.t.f);
			break;
		case VT_imm_string:
			fprintf(ofile, " = \"%s\"", prop->v.imm.t.str);
			break;
		case VT_imm_object: {
			int j;
			fprintf(ofile, " =");
			for (j = 0; j < prop->v.imm.t.obj.size; ++j) {
				fprintf(ofile, " %02x", prop->v.imm.t.obj.data[j]);
			}
			} break;
		default:
			break;
		}
	}
	fputs("\n", ofile);
	if (lbl)
		fprintf(ofile, "%.*s* '%s'\n", PFLEN(0), prefix, lbl);

	switch (prop->vtype) {
	case VT_NULL:
	case VT_implied:
		break;
	case VT_device:
		fprintf(ofile, "%.*s*  DCID = %s\n", PFLEN(0), prefix, 
					uuid2str(prop->v.dev.dcid, buf));
		break;
	case VT_alias:		
		break;
	case VT_network:
		fprintf(ofile, "%.*s*  %s, addr %u, size %u (%s )\n", PFLEN(0), prefix, 
				etypes[prop->v.net.dmp->etype],
				prop->v.net.dmp->addr, prop->v.net.dmp->size,
				flagnames(prop->v.net.dmp->flags, pflgnames, buf, " %s"));
		break;
	default:
		fprintf(ofile, "Error: unknown property type!\n");
		break;

	case VT_imm_uint:
	case VT_imm_sint:
	case VT_imm_float:
	case VT_imm_string:
	case VT_imm_object:
		if (prop->v.imm.count > 1) {
			//fprintf(ofile, "%.*s{\n", PFLEN(0), prefix);
			switch (prop->vtype) {
			case VT_imm_uint:
				for (i = 0; i < prop->v.imm.count; ++i)
					fprintf(ofile, "%.*s   [%d] = %u\n", PFLEN(0), prefix, i, prop->v.imm.t.Aui[i]);
				break;
			case VT_imm_sint:
				for (i = 0; i < prop->v.imm.count; ++i)
					fprintf(ofile, "%.*s   [%d] = %d\n", PFLEN(0), prefix, i, prop->v.imm.t.Asi[i]);
				break;
			case VT_imm_float:
				for (i = 0; i < prop->v.imm.count; ++i)
					fprintf(ofile, "%.*s   [%d] = %g\n", PFLEN(0), prefix, i, prop->v.imm.t.Af[i]);
				break;
			case VT_imm_string:
				for (i = 0; i < prop->v.imm.count; ++i)
					fprintf(ofile, "%.*s   [%d] = \"%s\"\n", PFLEN(0), prefix, i, prop->v.imm.t.Astr[i]);
				break;
			case VT_imm_object: {
				int j;
	
				for (i = 0; i < prop->v.imm.count; ++i) {
					fprintf(ofile, "%.*s   [%d] =", PFLEN(0), prefix, i);
					for (j = 0; j < prop->v.imm.t.Aobj[i].size; ++j) {
						fprintf(ofile, " %02x", prop->v.imm.t.Aobj[i].data[j]);
					}
				}
			}	break;
			default:
				break;
			}
			//fprintf(ofile, "%.*s}\n", PFLEN(0), prefix);
			//fputs("\n", ofile);
		}
		break;
	}
	fprintf(ofile, "%.*s\n", PFLEN(1), prefix);
} NEXT_PROP(prop);
}
