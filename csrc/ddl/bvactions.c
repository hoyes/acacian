/**********************************************************************/
/*

	Copyright (C) 2011, Engineering Arts. All rights reserved.

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
#include "ddl/keys.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL
/**********************************************************************/
/* typedef void bvaction(struct prop_s *prop, bv_t *bv); */

void
null_bvaction(struct prop_s *prop, bv_t *bv)
{
	acnlogmark(lgDBUG, "behavior %s: no action", bv->key.name);
}

#define BVA_acnbase_NULL 		null_bvaction
#define BVA_acnbase_r2_NULL 	null_bvaction

/**********************************************************************/

void
abstract_bvaction(struct prop_s *prop, bv_t *bv)
{
	ddlchar_t uuidstr[UUID_STR_SIZE];

	acnlogmark(lgWARN,
			"Abstract behavior %s [%s] used. Pleas use a refinement.",
			bv->key.name, uuid2str(bv->key.uuid, uuidstr));
}

#define BVA_sl_simplifiedLighting					abstract_bvaction

#define BVA_acnbase_typingPrimitive					abstract_bvaction
#define BVA_acnbase_reference							abstract_bvaction
#define BVA_acnbase_encoding							abstract_bvaction
#define BVA_acnbase_type_floating_point			abstract_bvaction
#define BVA_acnbase_accessClass						abstract_bvaction
#define BVA_acnbase_atomicLoad						abstract_bvaction
#define BVA_acnbase_algorithm							abstract_bvaction
#define BVA_acnbase_time								abstract_bvaction
#define BVA_acnbase_date								abstract_bvaction
#define BVA_acnbase_propertyRef						abstract_bvaction
#define BVA_acnbase_scale								abstract_bvaction
#define BVA_acnbase_rate								abstract_bvaction
#define BVA_acnbase_direction							abstract_bvaction
#define BVA_acnbase_orientation						abstract_bvaction
#define BVA_acnbase_publishParam						abstract_bvaction
#define BVA_acnbase_connectionDependent			abstract_bvaction
#define BVA_acnbase_pushBindingMechanism			abstract_bvaction
#define BVA_acnbase_pullBindingMechanism			abstract_bvaction
#define BVA_acnbase_DMPbinding						abstract_bvaction
#define BVA_acnbase_preferredValue_abstract		abstract_bvaction
#define BVA_acnbase_cyclicPath						abstract_bvaction
#define BVA_acnbase_streamFilter						abstract_bvaction
#define BVA_acnbase_beamDiverter						abstract_bvaction
#define BVA_acnbase_enumeration						abstract_bvaction
#define BVA_acnbase_boolean							abstract_bvaction

#define BVA_acnbase_r2_abstractPriority			abstract_bvaction
#define BVA_acnbase_r2_typingPrimitive				abstract_bvaction
#define BVA_acnbase_r2_reference						abstract_bvaction
#define BVA_acnbase_r2_encoding						abstract_bvaction
#define BVA_acnbase_r2_type_floating_point		abstract_bvaction
#define BVA_acnbase_r2_accessClass					abstract_bvaction
#define BVA_acnbase_r2_atomicLoad					abstract_bvaction
#define BVA_acnbase_r2_algorithm						abstract_bvaction
#define BVA_acnbase_r2_time							abstract_bvaction
#define BVA_acnbase_r2_date							abstract_bvaction
#define BVA_acnbase_r2_propertyRef					abstract_bvaction
#define BVA_acnbase_r2_scale							abstract_bvaction
#define BVA_acnbase_r2_rate							abstract_bvaction
#define BVA_acnbase_r2_direction						abstract_bvaction
#define BVA_acnbase_r2_orientation					abstract_bvaction
#define BVA_acnbase_r2_publishParam					abstract_bvaction
#define BVA_acnbase_r2_connectionDependent		abstract_bvaction
#define BVA_acnbase_r2_pushBindingMechanism		abstract_bvaction
#define BVA_acnbase_r2_pullBindingMechanism		abstract_bvaction
#define BVA_acnbase_r2_DMPbinding					abstract_bvaction
#define BVA_acnbase_r2_preferredValue_abstract	abstract_bvaction
#define BVA_acnbase_r2_cyclicPath					abstract_bvaction
#define BVA_acnbase_r2_enumeration					abstract_bvaction
#define BVA_acnbase_r2_boolean						abstract_bvaction

/**********************************************************************/
/*
Only implied and net properties carry a flag field. Attempting to 
set some flags on other types is just redundant and can be ignored, 
whilst other flags indicate an error.
*/

/*
Available flags

pflg_valid
pflg_read
pflg_write
pflg_event
pflg_vsize
pflg_abs
pflg_persistent
pflg_constant
pflg_volatile
*/

void
setbvflg(struct prop_s *prop, enum propflags_e flag)
{
	if (prop->vtype != VT_network) {
		acnlogmark(lgERR,
			"Attempt to specify access class on non network property");
		return;
	}
	flag |= prop->v.net.flags;
	if ((flag & pflg_constant) && (flag & (pflg_volatile | pflg_persistent))) {
		acnlogmark(lgERR,
			"Constant property cannot also be volatile or persistent");
		return;
	}
	prop->v.net.flags = flag;
}

/**********************************************************************/
/*
encoding types
etype_none
etype_boolean
etype_sint
etype_uint
etype_float
etype_UTF8
etype_UTF16
etype_UTF32
etype_string
etype_enum
etype_opaque
etype_uuid
etype_bitmap
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
			"Type/encoding behavior on non-network property");
		return;
	}
	if (prop->v.net.etype) {
		acnlogmark(lgWARN,
			"Redefinition of property type/encoding");
	}
	prop->v.net.etype = type;
}

/**********************************************************************/

void
persistent_bvaction(struct prop_s *prop, bv_t *bv)
{
	setbvflg(prop, pflg_persistent);
}

#define BVA_acnbase_persistent		persistent_bvaction
#define BVA_acnbase_r2_persistent	persistent_bvaction

/**********************************************************************/

void
constant_bvaction(struct prop_s *prop, bv_t *bv)
{
	setbvflg(prop, pflg_constant);
}

#define BVA_acnbase_constant		constant_bvaction
#define BVA_acnbase_r2_constant	constant_bvaction

/**********************************************************************/

void
volatile_bvaction(struct prop_s *prop, bv_t *bv)
{
	setbvflg(prop, pflg_volatile);
}

#define BVA_acnbase_volatile		volatile_bvaction
#define BVA_acnbase_r2_volatile	volatile_bvaction

/**********************************************************************/

void
et_boolean_bva(struct prop_s *prop, bv_t *bv)
{
	setptype(prop, etype_boolean);
}

#define BVA_acnbase_type_boolean		et_boolean_bva
#define BVA_acnbase_r2_type_boolean	et_boolean_bva

/**********************************************************************/

void
et_sint_bva(struct prop_s *prop, bv_t *bv)
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
		"signed integer: bad or variable size or not a network property");
}

#define BVA_acnbase_type_signed_integer		et_sint_bva
#define BVA_acnbase_type_sint						et_sint_bva
#define BVA_acnbase_r2_type_signed_integer	et_sint_bva
#define BVA_acnbase_r2_type_sint					et_sint_bva

/**********************************************************************/

void
et_uint_bva(struct prop_s *prop, bv_t *bv)
{
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
	acnlogmark(lgERR,
		"unsigned integer: bad or variable size or not a network property");
}

#define BVA_acnbase_type_unsigned_integer		et_uint_bva
#define BVA_acnbase_type_uint						et_uint_bva
#define BVA_acnbase_r2_type_unsigned_integer	et_uint_bva
#define BVA_acnbase_r2_type_uint					et_uint_bva

/**********************************************************************/

void
et_float_bva(struct prop_s *prop, bv_t *bv)
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
		"floating point: bad or variable size or not a network property");
}

#define BVA_acnbase_r2_type_float	et_float_bva
#define BVA_acnbase_type_float		et_float_bva

/**********************************************************************/

void
et_UTF8_bva(struct prop_s *prop, bv_t *bv)
{
	setptype(prop, etype_UTF8);
}

#define BVA_acnbase_r2_type_char_UTF_8	et_UTF8_bva
#define BVA_acnbase_type_char_UTF_8		et_UTF8_bva

/**********************************************************************/

void
et_UTF16_bva(struct prop_s *prop, bv_t *bv)
{
	setptype(prop, etype_UTF16);
}

#define BVA_acnbase_r2_type_char_UTF_16	et_UTF16_bva
#define BVA_acnbase_type_char_UTF_16		et_UTF16_bva

/**********************************************************************/

void
et_UTF32_bva(struct prop_s *prop, bv_t *bv)
{
	setptype(prop, etype_UTF32);
}

#define BVA_acnbase_r2_type_char_UTF_32	et_UTF32_bva
#define BVA_acnbase_type_char_UTF_32		et_UTF32_bva

/**********************************************************************/

void
et_string_bva(struct prop_s *prop, bv_t *bv)
{
	if (prop->vtype == VT_network && (prop->v.net.flags & pflg_vsize)) {
		setptype(prop, etype_string);
	} else {
		acnlogmark(lgERR,
			"String: not variable size or not a network property");
	}
}

#define BVA_acnbase_r2_type_string		et_string_bva
#define BVA_acnbase_type_string			et_string_bva

/**********************************************************************/

void
et_enum_bva(struct prop_s *prop, bv_t *bv)
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

#define BVA_acnbase_r2_type_enumeration	et_enum_bva
#define BVA_acnbase_type_enumeration		et_enum_bva
#define BVA_acnbase_r2_type_enumeration	et_enum_bva
#define BVA_acnbase_type_enumeration		et_enum_bva

/**********************************************************************/

void
et_opaque_fixsize_bva(struct prop_s *prop, bv_t *bv)
{
	if (prop->vtype == VT_network && !(prop->v.net.flags & pflg_vsize)) {
		setptype(prop, etype_opaque);
	} else {
		acnlogmark(lgERR,
			"fixBinob: variable size or not a network property");
	}
}

#define BVA_acnbase_r2_type_fixBinob		et_opaque_fixsize_bva
#define BVA_acnbase_type_fixBinob			et_opaque_fixsize_bva

/**********************************************************************/

void
et_opaque_varsize_bva(struct prop_s *prop, bv_t *bv)
{
	if (prop->vtype == VT_network && (prop->v.net.flags & pflg_vsize)) {
		setptype(prop, etype_opaque);
	} else {
		acnlogmark(lgERR,
			"varBinob: fixed size or not a network property");
	}
}

#define BVA_acnbase_r2_type_varBinob		et_opaque_varsize_bva
#define BVA_acnbase_type_varBinob			et_opaque_varsize_bva

/**********************************************************************/

void
et_uuid_bva(struct prop_s *prop, bv_t *bv)
{
	if (prop->vtype == VT_network) {
		if (!(prop->v.net.flags & pflg_vsize) && prop->v.net.size == 16)
		{
			setptype(prop, etype_uuid);
		} else {
			acnlogmark(lgERR,
				"UUID: wrong or variable size");
		}
	}
}

#define BVA_acnbase_r2_UUID		et_uuid_bva
#define BVA_acnbase_UUID			et_uuid_bva

/**********************************************************************/

void
et_bitmap_bva(struct prop_s *prop, bv_t *bv)
{
	setptype(prop, etype_bitmap);
}

#define BVA_acnbase_r2_type_bitmap		et_bitmap_bva

/**********************************************************************/

void
deviceref_bvaction(struct prop_s *prop, bv_t *bv)
{
	/* add_proptask(prop, &do_deviceref, bv); */
}
/**********************************************************************/
extern bv_t known_bvs[];
extern const int Nknown_bvs;

void
register_known_bvs(void)
{
	bv_t *bv;

	for (bv = known_bvs; bv < (known_bvs + Nknown_bvs); ++bv) {
		register_bv(bv);
	}
}

#define BVSET(x) setid_ ## x
#define BVENTRY(set, actionfn, namestr) \
	{ \
		.key = { \
			.uuid = (set), \
			.name = (namestr), \
			.namelen = sizeof(namestr) - 1 \
		}, \
		.action = &(actionfn) \
	},

extern const uuid_t setid_acnbase;
extern const uuid_t setid_acnbase_r2;
extern const uuid_t setid_acnbaseExt1;
extern const uuid_t setid_sl;
/* extern const uuid_t setid_originaldmp; */
extern const uuid_t setid_artnet;

bv_t known_bvs[] = {
#include "bvtab-acnbase.c"
#include "bvtab-acnbase-r2.c"
#include "bvtab-acnbaseExt1.c"
#include "bvtab-sl.c"
/* #include "bvtab-originaldmp.c" */
#include "bvtab-artnet.c"
};

const int Nknown_bvs = arraycount(known_bvs);

const uuid_t setid_acnbase     = BVSETID_acnbase;
const uuid_t setid_acnbase_r2  = BVSETID_acnbase_r2;
const uuid_t setid_acnbaseExt1 = BVSETID_acnbaseExt1;
const uuid_t setid_sl          = BVSETID_sl;
/* const uuid_t setid_originaldmp = BVSETID_originaldmp; */
const uuid_t setid_artnet      = BVSETID_artnet;

