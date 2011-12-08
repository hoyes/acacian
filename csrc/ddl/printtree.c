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
#include "propmap.h"
#include "uuid.h"
#include "ddl/parse.h"
#include "ddl/behaviors.h"
#include "ddl/resolve.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL
/**********************************************************************/
const ddlchar_t *ptypes[] = {
	[VT_NULL] = "NULL",
	[VT_imm_unknown] = "immediate (unknown)",
	[VT_implied] = "implied",
	[VT_network] = "network",
	[VT_include] = "include",
	[VT_imm_uint] = "immediate (uint)",
	[VT_imm_sint] = "immediate (sint)",
	[VT_imm_float] = "immediate (float)",
	[VT_imm_string] = "immediate (string)",
	[VT_imm_object] = "immediate (object)",
};

const ddlchar_t *etypes[] = {
    [etype_none]      = "none",
    [etype_boolean]   = "boolean",
    [etype_sint]      = "sint",
    [etype_uint]      = "uint",
    [etype_float]     = "float",
    [etype_UTF8]      = "UTF8",
    [etype_UTF16]     = "UTF16",
    [etype_UTF32]     = "UTF32",
    [etype_string]    = "string",
    [etype_enum]      = "enum",
    [etype_opaque]    = "opaque",
    [etype_bitmap]    = "bitmap",
};

void
printtree(struct prop_s *prop, int lvl)
{
	struct prop_s *pp;
	enum propflags_e flags;
	

	{
		int i;
		for (i = lvl; i--;) fputs("   ", stdout);
	}
	/*
	if (lvl) printf("%-*d", lvl * 3, lvl);
	*/
	printf("%s property", ptypes[prop->vtype]);
	if (prop->id) printf(" ID: %s", prop->id);
	switch (prop->vtype) {
	case VT_imm_object:
		{
			int i;
			printf(" =");
			for (i = 0; i < prop->v.imm.t.obj.size; ++i) {
				printf(" %02x", prop->v.imm.t.obj.data[i]);
			}
		}
		/* fall through */
	case VT_NULL:
	case VT_implied:
	case VT_include:
		fputc('\n', stdout);
		break;
	case VT_imm_unknown:
		printf(" value: Unknown\n");
		break;
	case VT_network:
		printf(" loc: %u, size %u\n", prop->v.net.hd.addr, prop->v.net.size);
		{
			int i;
			for (i = lvl; i--;) fputs("   ", stdout);
		}
		fputs("- flags:", stdout);
		flags = prop->v.net.flags;
		if ((flags & pflg_valid))      fputs(" valid", stdout);
		if ((flags & pflg_read))       fputs(" read", stdout);
		if ((flags & pflg_write))      fputs(" write", stdout);
		if ((flags & pflg_event))      fputs(" event", stdout);
		if ((flags & pflg_vsize))      fputs(" vsize", stdout);
		if ((flags & pflg_abs))        fputs(" abs", stdout);
	    if ((flags & pflg_persistent)) fputs(" persistent", stdout);
	    if ((flags & pflg_constant))   fputs(" constant", stdout);
	    if ((flags & pflg_volatile))   fputs(" volatile", stdout);
	    fputs("\n", stdout);
		{
			int i;
			for (i = lvl; i--;) fputs("   ", stdout);
		}
		printf("- type/encoding: %s\n", etypes[prop->v.net.etype]);
		break;
	case VT_imm_uint:
		printf(" = %u\n", prop->v.imm.t.ui);
		break;
	case VT_imm_sint:
		printf(" = %d\n", prop->v.imm.t.si);
		break;
	case VT_imm_float:
		printf(" = %g\n", prop->v.imm.t.f);
		break;
	case VT_imm_string:
		printf(" = \"%s\"\n", prop->v.imm.t.str);
		break;
	default:
		printf("unknown type!\n");
		break;
	}
	
	for (pp = prop->children; pp != NULL; pp = pp->siblings)
		printtree(pp, lvl + 1);
}
