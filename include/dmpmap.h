/**********************************************************************/
/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (c) 2013, Acuity Brands, Inc.

Author: Philip Nye <philip.nye@engarts.com>

This file forms part of Acacian a full featured implementation of 
ANSI E1.17 Architecture for Control Networks (ACN)

#tabs=3t
*/
/**********************************************************************/
/*
header: dmpmap.h

DMP address and property mapping
*/

#ifndef __dmpmap_h__
#define __dmpmap_h__ 1

enum netflags_e {
/* basic flags from propref_DMP element */
	pflgb_read,
	pflgb_write,
	pflgb_event,
	pflgb_vsize,
	pflgb_abs,
#if CF_DDL_BEHAVIORS
/* flags derived from behaviors */
	pflgb_constant,
	pflgb_persistent,
	pflgb_volatile,
	pflgb_ordered,
	pflgb_measure,
	pflgb_cyclic,
#endif  /* CF_DDL_BEHAVIORS */
/* flags relating to address map */
	pflgb_packed,
	pflgb_overlap,
	pflgb_MAX
};

#define pflg(x) (1 << pflgb_ ## x)

#define pflg_COUNT pflgb_MAX

extern const char *pflgnames[pflg_COUNT];
/* pflg_NAMELEN is sum of strlen(pflg_NAMES) */
#define pflg_NAMELEN 61

#if CF_DDL_BEHAVIORS
enum proptype_e {   /* encoding type */
	etype_unknown,
	etype_boolean,
	etype_sint,
	etype_uint,
	etype_float,
	etype_UTF8,
	etype_UTF16,
	etype_UTF32,
	etype_string,
	etype_enum,
	etype_opaque,
	etype_UUID,
	etype_DCID,
	etype_CID,
	etype_languagesetID,
	etype_behaviorsetID,
	etype_ISOdate,
	etype_URI,
	etype_bitmap,
	etype_MAX
};
#endif  /* CF_DDL_BEHAVIORS */

#define getflags(pp) ((pp)->flags)
#define getsize(pp)  ((pp)->size)

#if !CF_DMPMAP_NONE
#define PROP_P_ const struct dmpprop_s *prop,
#define PROP prop,
#else
#define PROP_P_
#define PROP_A_
#endif

struct dmptcxt_s;
struct adspec_s;

struct dmpdim_s {
   int32_t inc;  /* increment */
   uint32_t cnt; /* range */
   uint8_t tref; 	/* reference to dim in tree order - 0 references the leaf */
   uint8_t lvl;
};

/*
type: dmpprop_s

Contains information required for DMP access to a property.
*/

struct dmpprop_s {
	struct dmpprop_s *nxt;
	struct ddlprop_s *prop;
	uint32_t flags;
#if CF_DDL_BEHAVIORS
	enum proptype_e etype;
#endif
	unsigned int size;
	uint32_t addr;
	uint32_t span;
#ifdef CF_PROPEXT_TOKS
#if CF_MAPGEN
	const char *extends[CF_NUMEXTENDFIELDS];
#else
#undef _EXTOKEN_
#define _EXTOKEN_(tk, type) type tk ;
   CF_PROPEXT_TOKS
#endif
#endif
	int ndims;
	struct dmpdim_s dim[];
};

#define _DMPPROPSIZE offsetof(struct dmpprop_s, dim)

#define dmppropsize(ndims) (_DMPPROPSIZE + sizeof(struct dmpdim_s) * (ndims))

/*
group: Address search structures

Address search maps may be of different types.

enum: maptype_e

am_none - no map or unspecified map.
am_srch - a map optimized for binary search
am_indx - a direct lookup map; fast but only suitable for certain devices
*/

enum maptype_e {am_none = 0, am_srch, am_indx};

struct addrfind_s;
/*
types: Address map structures

addrmap_u - an address map, type specified by maptype_e.

any_amap_s - dummy structure defining the fields common to all address
map types.

srch_amap_s - generic address map using binary search to find a 
region then possibly further tests as defined in <Algorithm for 
address search>. The search strategy divides the address space into 
a linear sorted array of regions defined by an upper and lower 
address bound within which one or more properties occur.

addrfind_s - a single element of the srch_amap_s search table.

indx_amap_s - linear vector address map for direct property lookup. The
fastest address lookup type but only practical with few properties with
closely grouped addresses.
*/

/*
Each type in the union contains the first two elements, type and 
size, in the same order so they are invariant with map type. The 
third element is also always the pointer to the map array, but its 
target type differs depending on map type. Size is always the size 
of the allocated map block in bytes.
*/

struct any_amap_s {
	uint8_t dcid[UUID_SIZE];
	enum maptype_e type;
	size_t size;
	uint8_t *map;
	uint16_t flags;
	uint16_t maxdims;
};

struct srch_amap_s {
	uint8_t dcid[UUID_SIZE];
	enum maptype_e type;
	size_t size;
	struct addrfind_s *map;
	uint16_t flags;
	uint16_t maxdims;
	uint32_t count;
};

struct indx_amap_s{
	uint8_t dcid[UUID_SIZE];
	enum maptype_e type;
	size_t size;
	struct dmpprop_s **map;
	uint16_t flags;
	uint16_t maxdims;
	uint32_t range;
	uint32_t base;
};

union addrmap_u {
	struct any_amap_s any;
	struct indx_amap_s indx;
	struct srch_amap_s srch;
};

struct addrfind_s {
	uint32_t adlo;   /* lowest address of the region */
	uint32_t adhi;   /* highest address */
	int ntests;      /* zero if address range is packed (no holes) */
	union {
		struct dmpprop_s *prop;
		struct dmpprop_s **pa;
	} p;
};

#define maplength(amapp, _type_) (amapp->any.size / sizeof(*amapp->_type_.map))
/**********************************************************************/
/*
prototypes
*/
const struct dmpprop_s *addr_to_prop(union addrmap_u *amap, uint32_t addr);
void freeamap(union addrmap_u *amap);
void indexprop(struct dmpprop_s *prop, struct dmpprop_s **imap, int dimx, uint32_t ad);
void xformtoindx(union addrmap_u *amap);
//void fillindexes(const struct dmpprop_s *prop, struct adspec_s *ads, uint32_t *indexes);

#endif /*  __dmpmap_h__       */
