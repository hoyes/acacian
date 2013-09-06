/**********************************************************************/
/*

	Copyright (c) 2011, Philip Nye, Engineering Arts (UK) philip@engarts.com
	All rights reserved.

	Author: Philip Nye

	$Id$

*/
/*
#tabs=3
*/
/**********************************************************************/

#ifndef __dmpmap_h__
#define __dmpmap_h__ 1

enum netflags_e {
	pflg_read       = 1,
	pflg_write      = 2,
	pflg_event      = 4,
	pflg_vsize      = 8,
	pflg_abs        = 16,
	pflg_constant   = 32,
	pflg_persistent = 64,
	pflg_volatile   = 128,
	pflg_packed     = 256,
	pflg_MAX        = 512
};

#define pflg_COUNT (nbits(pflg_MAX) - 1)

#define pflg_NAMES \
 	 "read", \
 	 "write", \
 	 "event", \
 	 "vsize", \
 	 "abs", \
 	 "constant", \
 	 "persistent", \
 	 "volatile", \
 	 "packed"

#define pflg_NAMELEN 54
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

#if !ACNCFG_DDL
#define prop_s dmpprop_s
#define getflags(pp) ((pp)->flags)
#define getsize(pp)  ((pp)->size)
#else
#define getflags(pp) ((pp)->v.net->dmp.flags)
#define getsize(pp)  ((pp)->v.net->dmp.size)

#endif

#if !ACNCFG_DMPMAP_NONE
#define PROP_P_ const struct dmpprop_s *prop,
#define PROP prop,
#else
#define PROP_P_
#define PROP_A_
#endif

struct dmptxcxt_s;

struct dmpdim_s {
   int32_t i;  /* increment */
   uint32_t r; /* range (= count - 1) */
#if ACNCFG_DDL
   int lvl; 	/* lvl shows original the tree order - 0 at the root */
#endif
};

struct dmpprop_s {
	struct prop_s *prop;
	enum netflags_e flags;
#if ACNCFG_DDL_BEHAVIORTYPES
	enum proptype_e etype;
#endif
	unsigned int size;
	uint32_t addr;
	uint32_t ulim;
#ifdef ACNCFG_EXTENDTOKENS
	char *extends[ACNCFG_NUMEXTENDFIELDS];
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
	enum maptype_e type;
	size_t size;
	uint8_t *map;
};

struct indx_amap_s{
	enum maptype_e type;
	size_t size;
	struct dmpprop_s **map;
	uint32_t base;
};

struct srch_amap_s {
		enum maptype_e type;
		size_t size;
		struct addrfind_s *map;
		uint32_t count;
};

union addrmap_u {
	struct any_amap_s any;
	struct indx_amap_s indx;
	struct srch_amap_s srch;
};

#define maplength(amapp, _type_) (amapp->any.size / sizeof(*amapp->_type_.map))

#endif /*  __dmpmap_h__       */
