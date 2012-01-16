/**********************************************************************/
/*

	Copyright (C) 2012, Engineering Arts. All rights reserved.

	Author: Philip Nye

	$Id$

#tabs=3
*/
/**********************************************************************/

#include <expat.h>
#include "acncommon.h"
#include "acnlog.h"
#include "acnmem.h"
#include "propmap.h"
#include "uuid.h"
#include "ddl/parse.h"
#include "ddl/behaviors.h"
#include "ddl/bvactions.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL

/**********************************************************************/
/*
For each behavior action, define a BVA_behaviorset_behaviorname macro
for each behavior which calls that action - these macros then 
create the appropriate entries in the known_bvs table.
*/
/**********************************************************************/
/* typedef void bvaction(struct prop_s *prop, const bv_t *bv); */

void
null_bvaction(struct prop_s *prop, const bv_t *bv)
{
	acnlogmark(lgDBUG, "     behavior %s: no action", bv->name);
}

/**********************************************************************/
/*
Many behaviors are defined as abstract - they are used for refinement
but should not be applied directly to properties
*/

void
abstract_bvaction(struct prop_s *prop, const bv_t *bv)
{
	acnlogmark(lgWARN,
			"     Abstract behavior %s used. Pleas use a refinement.",
			bv->name);
}

/**********************************************************************/
/*
Property flag behaviors
*/
/**********************************************************************/
/*
set property flags
Only makes sense on net properties
FIXME: More sophisticated errror checking needed e.g. a constant flag
applied to an immediate property is not really an error - just redundant
*/

void
setbvflg(struct prop_s *prop, enum propflags_e flag)
{
	if (prop->vtype != VT_network) {
		acnlogmark(lgERR,
			"     Attempt to specify access class (0x%04x) on non network property",
			flag);
		return;
	}
	flag |= prop->v.net.flags;

	/* perform some sanity checks */
	if ((flag & pflg_constant) && (flag & (pflg_volatile | pflg_persistent))) {
		acnlogmark(lgERR,
			"     Constant property cannot also be volatile or persistent");
		return;
	}
	prop->v.net.flags = flag;
	acnlogmark(lgDBUG,
		"     prop flags: %s", flagnames(flag));
}

/**********************************************************************/
/*
behavior: persistent
behaviorsets: acnbase, acnbase-r2
*/
void
persistent_bvaction(struct prop_s *prop, const bv_t *bv)
{
	setbvflg(prop, pflg_persistent);
}

/**********************************************************************/
/*
behavior: constant
behaviorsets: acnbase, acnbase-r2
*/

void
constant_bvaction(struct prop_s *prop, const bv_t *bv)
{
	setbvflg(prop, pflg_constant);
}

/**********************************************************************/
/*
behavior: volatile
behaviorsets: acnbase, acnbase-r2
*/

void
volatile_bvaction(struct prop_s *prop, const bv_t *bv)
{
	setbvflg(prop, pflg_volatile);
}


/**********************************************************************/
/*
Property encoding behaviors
*/
/**********************************************************************/
/*
set encoding type
only make sense for network properties
*/

void
setptype(struct prop_s *prop, enum proptype_e type)
{
/*
	switch (prop->vtype) {
	case VT_NULL:
	case VT_implied:
		acnlogmark(lgWARN,
			"Type/encoding on NULL or implied property");
		return;
	case VT_network:
		if (prop->v.net.etype) {
			acnlogmark(lgWARN,
				"Redefinition of property type/encoding");
		}
		prop->v.net.etype = type;
	case VT_imm_unknown:

	default:
		acnlogmark(lgERR,
			"unknown property vtype");
		return;
	}
*/

	if (prop->vtype != VT_network) {
		acnlogmark(lgERR,
			"     Type/encoding behavior on non-network property");
		return;
	}
	if (prop->v.net.etype) {
		acnlogmark(lgWARN,
			"     Redefinition of property type/encoding");
	}
	prop->v.net.etype = type;
}

/**********************************************************************/
/*
behavior: type.boolean
behaviorsets: acnbase, acnbase-r2
*/

void
et_boolean_bva(struct prop_s *prop, const bv_t *bv)
{
	setptype(prop, etype_boolean);
}


/**********************************************************************/
/*
behavior: type.signed.integer, type.sint
behaviorsets: acnbase, acnbase-r2
*/

void
et_sint_bva(struct prop_s *prop, const bv_t *bv)
{
	if (prop->vtype == VT_network && !(prop->v.net.flags & pflg_vsize)) {
		switch (prop->v.net.size) {
		case 1:
		case 2:
		case 4:
		case 8:
			setptype(prop, etype_sint);
			return;
		default:
			break;
		}
	}
	acnlogmark(lgERR,
		"     signed integer: bad or variable size or not a network property");
}


/**********************************************************************/
/*
behavior: type.unsigned.integer, type.uint
behaviorsets: acnbase, acnbase-r2
*/

void
et_uint_bva(struct prop_s *prop, const bv_t *bv)
{
#if defined(BEHAVIOR_AFTER_PROPREF)
	if (prop->vtype == VT_network && !(prop->v.net.flags & pflg_vsize)) {
		switch (prop->v.net.size) {
		case 1:
		case 2:
		case 4:
		case 8:
			setptype(prop, etype_uint);
			return;
		default:
			break;
		}
	}
#else
	if (prop->vtype == VT_network)
		setptype(prop, etype_uint);
	else
#endif
	acnlogmark(lgERR,
		"     unsigned integer: bad or variable size or not a network property");
}


/**********************************************************************/
/*
behavior: type.float
behaviorsets: acnbase, acnbase-r2
*/

void
et_float_bva(struct prop_s *prop, const bv_t *bv)
{
	if (prop->vtype == VT_network && !(prop->v.net.flags & pflg_vsize)) {
		switch (prop->v.net.size) {
		case 4:
		case 8:
			setptype(prop, etype_float);
			return;
		default:
			break;
		}
	}
	acnlogmark(lgERR,
		"     floating point: bad or variable size or not a network property");
}


/**********************************************************************/
/*
behavior: type.char.UTF-8
behaviorsets: acnbase, acnbase-r2
*/

void
et_UTF8_bva(struct prop_s *prop, const bv_t *bv)
{
	setptype(prop, etype_UTF8);
}


/**********************************************************************/
/*
behavior: type.char.UTF-16
behaviorsets: acnbase, acnbase-r2
*/

void
et_UTF16_bva(struct prop_s *prop, const bv_t *bv)
{
	setptype(prop, etype_UTF16);
}


/**********************************************************************/
/*
behavior: type.char.UTF-32
behaviorsets: acnbase, acnbase-r2
*/

void
et_UTF32_bva(struct prop_s *prop, const bv_t *bv)
{
	setptype(prop, etype_UTF32);
}


/**********************************************************************/
/*
behavior: type.string
behaviorsets: acnbase, acnbase-r2
*/

void
et_string_bva(struct prop_s *prop, const bv_t *bv)
{
	if (prop->vtype == VT_network && (prop->v.net.flags & pflg_vsize)) {
		setptype(prop, etype_string);
	} else {
		acnlogmark(lgERR,
			"     String: not variable size or not a network property");
	}
}


/**********************************************************************/
/*
behavior: type.enumeration, type.enum
behaviorsets: acnbase, acnbase-r2
*/

void
et_enum_bva(struct prop_s *prop, const bv_t *bv)
{
	if (prop->vtype == VT_network && !(prop->v.net.flags & pflg_vsize)) {
		switch (prop->v.net.size) {
		case 1:
		case 2:
		case 4:
		case 8:
			setptype(prop, etype_enum);
			return;
		default:
			break;
		}
	}
	acnlogmark(lgERR,
		"enumeration: bad or variable size or not a network property");
}


/**********************************************************************/
/*
behavior: type.fixBinob
behaviorsets: acnbase, acnbase-r2
*/

void
et_opaque_fixsize_bva(struct prop_s *prop, const bv_t *bv)
{
	if (prop->vtype == VT_network && !(prop->v.net.flags & pflg_vsize)) {
		setptype(prop, etype_opaque);
	} else {
		acnlogmark(lgERR,
			"     fixBinob: variable size or not a network property");
	}
}


/**********************************************************************/
/*
behavior: type.varBinob
behaviorsets: acnbase, acnbase-r2
*/

void
et_opaque_varsize_bva(struct prop_s *prop, const bv_t *bv)
{
	if (prop->vtype == VT_network && (prop->v.net.flags & pflg_vsize)) {
		setptype(prop, etype_opaque);
	} else {
		acnlogmark(lgERR,
			"     varBinob: fixed size or not a network property");
	}
}


/**********************************************************************/
/*
behavior: UUID
behaviorsets: acnbase, acnbase-r2
*/

void
et_uuid_bva(struct prop_s *prop, const bv_t *bv)
{
	if (prop->vtype == VT_network) {
		if (!(prop->v.net.flags & pflg_vsize) && prop->v.net.size == 16)
		{
			setptype(prop, etype_uuid);
		} else {
			acnlogmark(lgERR,
				"     UUID: wrong or variable size");
		}
	}
}


/**********************************************************************/
/*
behavior: type.bitmap
behaviorsets: acnbase-r2
*/

void
et_bitmap_bva(struct prop_s *prop, const bv_t *bv)
{
	setptype(prop, etype_bitmap);
}


/**********************************************************************/

void
deviceref_bvaction(struct prop_s *prop, const bv_t *bv)
{
	/* add_proptask(prop, &do_deviceref, bv); */
}
/**********************************************************************/
