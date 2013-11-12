/**********************************************************************/
/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (c) 2013, Acuity Brands, Inc.

Author: Philip Nye <philip.nye@engarts.com>

#tabs=3
*/
/**********************************************************************/
/*
about: Acacian

Acacian is a full featured implementation of ANSI E1.17 2012
Architecture for Control Networks (ACN) from Acuity Brands

header: dmpmap.h

DMP address and property mapping
*/

#ifndef __dmpmap_h__
#define __dmpmap_h__ 1

enum netflags_e {
	pflgb_read,
	pflgb_write,
	pflgb_event,
	pflgb_vsize,
	pflgb_abs,
	pflgb_constant,
	pflgb_persistent,
	pflgb_volatile,
	pflgb_packed,
	pflgb_overlap,
	pflgb_MAX
};

#define pflg(x) (1 << pflgb_ ## x)

#define pflg_COUNT pflgb_MAX

#define pflg_NAMES \
 	 "read", \
 	 "write", \
 	 "event", \
 	 "vsize", \
 	 "abs", \
 	 "constant", \
 	 "persistent", \
 	 "volatile", \
 	 "packed", \
 	 "overlap"

/* pflg_NAMELEN is sum of strlen(pflg_NAMES) */
#define pflg_NAMELEN 61

extern const char *pflgnames[pflg_COUNT];

#if ACNCFG_DDL_BEHAVIORTYPES
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
	etype_uuid,
	etype_bitmap
};
#endif

#define getflags(pp) ((pp)->flags)
#define getsize(pp)  ((pp)->size)

#if !ACNCFG_DMPMAP_NONE
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
#if ACNCFG_DDL
   int tref; 	/* reference to dim in tree order - 0 references the leaf */
#endif
};

struct dmpprop_s {
	struct dmpprop_s *nxt;
	struct ddlprop_s *prop;
	enum netflags_e flags;
#if ACNCFG_DDL_BEHAVIORTYPES
	enum proptype_e etype;
#endif
	unsigned int size;
	uint32_t addr;
	uint32_t span;
#ifdef ACNCFG_PROPEXT_TOKS
#if ACNCFG_MAPGEN
	char *extends[ACNCFG_NUMEXTENDFIELDS];
#else
#undef _EXTOKEN_
#define _EXTOKEN_(tk, type) type tk ;
   ACNCFG_PROPEXT_TOKS
#endif
#endif
	int ndims;
	struct dmpdim_s dim[];
};

#define _DMPPROPSIZE offsetof(struct dmpprop_s, dim)

#define dmppropsize(ndims) (_DMPPROPSIZE + sizeof(struct dmpdim_s) * (ndims))

/*
about: ACNCFG_DMPMAP_SEARCH

The search strategy divides the address space into a linear sorted 
array of regions defined by an upper and lower address bound within 
which one or more properties occur.

Single properties or array properties whose entire address range is 
packed constitute an exclusive region so for an address within this 
region no further testing is necessary and the entry identifies the 
property. For sparse array properties - of which many may overlap 
each other, the situation is more complex. The entry then identifies 
a list of all candidate properties with addresses in the region and 
each must be tested in turn for a hit.

*/

struct addrfind_s {
	uint32_t adlo;   /* lowest address of the region */
	uint32_t adhi;   /* highest address */
	int ntests;      /* zero if address range is packed (no holes) */
	union {
		struct dmpprop_s *prop;
		struct dmpprop_s **pa;
	} p;
};

enum maptype_e {am_none = 0, am_srch, am_indx};
/*
Each type in the union contains the first two elements, type and size, in the same order
so they are invariant with map type. The third element is also always the pointer 
to the map array, but its target type differs depending on map type.
Size is always the size of the allocated map block in bytes.
*/

struct any_amap_s {
	uint8_t dcid[UUID_SIZE];
	enum maptype_e type;
	size_t size;
	uint8_t *map;
	uint16_t flags;
	uint16_t maxdims;
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

struct srch_amap_s {
	uint8_t dcid[UUID_SIZE];
	enum maptype_e type;
	size_t size;
	struct addrfind_s *map;
	uint16_t flags;
	uint16_t maxdims;
	uint32_t count;
};

union addrmap_u {
	struct any_amap_s any;
	struct indx_amap_s indx;
	struct srch_amap_s srch;
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
