/**********************************************************************/
/*

	Copyright (C) 2012, Engineering Arts. All rights reserved.

	Author: Philip Nye

	$Id$

#tabs=3
*/
/**********************************************************************/

#include <expat.h>
#include <assert.h>
#include "acn.h"

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
/* typedef void bvaction(struct dcxt_s *dcxp, const struct bv_s *bv); */

void
null_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	acnlogmark(lgDBUG, "     behavior %s: no action", bv->name);
}

/**********************************************************************/
/*
Many behaviors are defined as abstract - they are used for refinement
but should not be applied directly to properties
*/

void
abstract_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
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
setbvflg(struct dcxt_s *dcxp, enum netflags_e flag)
{
	struct ddlprop_s *prop = dcxp->m.dev.curprop;
	struct dmpprop_s *np;
	char buf[pflg_NAMELEN + pflg_COUNT];

	if (prop->vtype != VT_network) {
		if (flag != pflg(constant)) {
			acnlogmark(lgERR,
				"     Attempt to specify access class (0x%04x) on non network property",
				flag);
		}
		return;
	}
	np = prop->v.net.dmp;
	flag |= np->flags;

	/* perform some sanity checks */
	if ((flag & pflg(constant)) && (flag & (pflg(volatile) | pflg(persistent)))) {
		acnlogmark(lgERR,
			"     Constant property cannot also be volatile or persistent");
		return;
	}
	np->flags = flag;
	acnlogmark(lgDBUG,
		"     prop flags:%s", flagnames(flag, pflgnames, buf, " %s"));
}

/**********************************************************************/
/*
behavior: persistent
behaviorsets: acnbase, acnbase-r2
*/
void
persistent_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setbvflg(dcxp, pflg(persistent));
}

/**********************************************************************/
/*
behavior: constant
behaviorsets: acnbase, acnbase-r2
*/

void
constant_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setbvflg(dcxp, pflg(constant));
}

/**********************************************************************/
/*
behavior: volatile
behaviorsets: acnbase, acnbase-r2
*/

void
volatile_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setbvflg(dcxp, pflg(volatile));
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
setptype(struct dcxt_s *dcxp, enum proptype_e type)
{
	struct ddlprop_s *prop = dcxp->m.dev.curprop;
	struct dmpprop_s *np;

/*
	switch (prop->vtype) {
	case VT_NULL:
	case VT_implied:
		acnlogmark(lgWARN,
			"Type/encoding on NULL or implied property");
		return;
	case VT_network:
		if (np->etype) {
			acnlogmark(lgWARN,
				"Redefinition of property type/encoding");
		}
		np->etype = type;
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
	np = prop->v.net.dmp;
	if (np->etype) {
		acnlogmark(lgWARN,
			"     Redefinition of property type/encoding");
	}
	np->etype = type;
}

/**********************************************************************/
/*
behavior: type.boolean
behaviorsets: acnbase, acnbase-r2
*/

void
et_boolean_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_boolean);
}


/**********************************************************************/
/*
behavior: type.signed.integer, type.sint
behaviorsets: acnbase, acnbase-r2
*/

void
et_sint_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	struct ddlprop_s *prop = dcxp->m.dev.curprop;
	struct dmpprop_s *np;

	if (prop->vtype == VT_network && !((np = prop->v.net.dmp)->flags & pflg(vsize))) {
		switch (np->size) {
		case 1:
		case 2:
		case 4:
		case 8:
			setptype(dcxp, etype_sint);
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
et_uint_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	struct ddlprop_s *prop = dcxp->m.dev.curprop;
	struct dmpprop_s *np;

	if (prop->vtype == VT_network && !((np = prop->v.net.dmp)->flags & pflg(vsize))) {
		switch (np->size) {
		case 1:
		case 2:
		case 4:
		case 8:
			setptype(dcxp, etype_uint);
			return;
		default:
			break;
		}
	}
	acnlogmark(lgERR,
		"     unsigned integer: bad or variable size or not a network property");
}


/**********************************************************************/
/*
behavior: type.float
behaviorsets: acnbase, acnbase-r2
*/

void
et_float_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	struct ddlprop_s *prop = dcxp->m.dev.curprop;
	struct dmpprop_s *np;

	if (prop->vtype == VT_network && !((np = prop->v.net.dmp)->flags & pflg(vsize))) {
		switch (np->size) {
		case 4:
		case 8:
			setptype(dcxp, etype_float);
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
et_UTF8_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_UTF8);
}


/**********************************************************************/
/*
behavior: type.char.UTF-16
behaviorsets: acnbase, acnbase-r2
*/

void
et_UTF16_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_UTF16);
}


/**********************************************************************/
/*
behavior: type.char.UTF-32
behaviorsets: acnbase, acnbase-r2
*/

void
et_UTF32_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_UTF32);
}


/**********************************************************************/
/*
behavior: type.string
behaviorsets: acnbase, acnbase-r2
*/

void
et_string_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	struct ddlprop_s *prop = dcxp->m.dev.curprop;
	struct dmpprop_s *np;

	assert (prop->vtype < VT_maxtype);
	switch (prop->vtype) {
	case VT_network:
		if (((np = prop->v.net.dmp)->flags & pflg(vsize)))
			setptype(dcxp, etype_string);
		else
			acnlogmark(lgERR, "     String network property not variable size");
		break;
	case VT_imm_string:
	case VT_imm_object:
		/* just ignore string behavior here */
		break;
	case VT_NULL:
	case VT_imm_unknown:
	case VT_implied:
	case VT_include:
	case VT_device:
	case VT_imm_uint:
	case VT_imm_sint:
	case VT_imm_float:
		acnlogmark(lgERR,
			"     String behavior not permitted on %s property", ptypes[prop->vtype]);
		break;
	}
}


/**********************************************************************/
/*
behavior: type.enumeration, type.enum
behaviorsets: acnbase, acnbase-r2
*/

void
et_enum_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	struct ddlprop_s *prop = dcxp->m.dev.curprop;
	struct dmpprop_s *np;

	if (prop->vtype == VT_network && !((np = prop->v.net.dmp)->flags & pflg(vsize))) {
		switch (np->size) {
		case 1:
		case 2:
		case 4:
		case 8:
			setptype(dcxp, etype_enum);
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
et_opaque_fixsize_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	struct ddlprop_s *prop = dcxp->m.dev.curprop;
	struct dmpprop_s *np;

	if (prop->vtype == VT_network && !((np = prop->v.net.dmp)->flags & pflg(vsize))) {
		setptype(dcxp, etype_opaque);
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
et_opaque_varsize_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	struct ddlprop_s *prop = dcxp->m.dev.curprop;
	struct dmpprop_s *np;

	if (prop->vtype == VT_network && ((np = prop->v.net.dmp)->flags & pflg(vsize))) {
		setptype(dcxp, etype_opaque);
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
et_uuid_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	struct ddlprop_s *prop = dcxp->m.dev.curprop;
	struct dmpprop_s *np;

	if (prop->vtype == VT_network) {
		if (!((np = prop->v.net.dmp)->flags & pflg(vsize)) && np->size == 16)
		{
			setptype(dcxp, etype_uuid);
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
et_bitmap_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_bitmap);
}


/**********************************************************************/

void
deviceref_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	/* add_proptask(prop, &do_deviceref, bv); */
}

/**********************************************************************/
/*
behavior: UACN
behaviorsets: acnbase, acnbase-r2
*/
void
persist_string_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	et_string_bva(dcxp, bv);
	persistent_bva(dcxp, bv);
}

/**********************************************************************/
/*
behavior: FCTN
behaviorsets: acnbase, acnbase-r2
*/
void
const_string_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	et_string_bva(dcxp, bv);
	constant_bva(dcxp, bv);
}

/**********************************************************************/
