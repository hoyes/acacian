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
//	pflg_valid      = 1,
	pflg_read       = 2,
	pflg_write      = 4,
	pflg_event      = 8,
	pflg_vsize      = 16,
	pflg_abs        = 32,
	pflg_persistent = 64,
	pflg_constant   = 128,
	pflg_volatile   = 256,
};

#if CONFIG_DDL_BEHAVIORTYPES
enum proptype_e {   /* encoding type */
	etype_none = 0,
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

#if !CONFIG_DDL
#define prop_s dmpprop_s
#define getflags(pp) ((pp)->flags)
#define getsize(pp)  ((pp)->size)
#else
#define getflags(pp) ((pp)->v.net->dmp.flags)
#define getsize(pp)  ((pp)->v.net->dmp.size)

#endif

#if !CONFIG_DMPMAP_NONE
#define PROP_P_ const struct prop_s *prop,
#define PROP_A_ prop,
#else
#define PROP_P_
#define PROP_A_
#endif

struct dmptxcxt_s;

struct dmpdim_s {
   int32_t i;  /* increment */
   uint32_t r; /* range (= count - 1) */
#if CONFIG_DDL
   int lvl; 	/* lvl shows original the tree order - 0 at the root */
#endif
};

struct dmpprop_s {
	enum netflags_e flags;
#if CONFIG_DDL_BEHAVIORTYPES
	enum proptype_e etype;
#endif
	unsigned int size;
	uint32_t addr;
	//int32_t inc;
	int ndims;
	struct dmpdim_s dim[];
};
#define _DMPPROPSIZE (((struct dmpprop_s *)0)->dim - NULL)

#define dmppropsize(ndims) (_DMPPROPSIZE + sizeof(struct dmpdim_s) * (ndims))

#if CONFIG_DMPMAP_SEARCH
/*
addrfind_s comes in a sorted arrays which are used for rapid
finding of a property from it's address
*/

union proportest_u {
	struct prop_s *prop;	/* pointer to the property data */
	struct addrtest_s *test;
};

struct addrtest_s {
	struct prop_s *prop;
	union proportest_u nxt;
};

struct addrfind_s {
	uint32_t adlo;	/* lowest address of the region */
	uint32_t adhi;	/* highest address */
	int ntests; /* true if address range is packed (no holes) */
	union proportest_u p;
};

typedef struct addrfind_s addrfind_t;

struct addrmapheader_s {
#if CONFIG_DDL
	unsigned int mapsize;
#endif
	unsigned int count;
};

struct addrmap_s {
	struct addrmapheader_s h;
	struct addrfind_s map[];
};

extern struct prop_s *findaddr(struct addrfind_s *map, int maplen,
					uint32_t addr, int32_t inc, uint32_t *nprops);
#elif CONFIG_DMPMAP_INDEX
/*
With direct maps we simply have an array of prop_s pointers indexed by
property address
FIXME: findaddr() does not handle packed multidimensional ranges well
*/
typedef struct prop_s * addrfind_t;

struct addrmapheader_s {
	unsigned int count;
};

struct addrmap_s {
	struct addrmapheader_s h;
	addrfind_t *map;
};

static inline const struct prop_s *
findaddr(addrfind_t *map, int maplen,
					uint32_t addr, int32_t inc, uint32_t *nprops)
{
	int i;
	uint32_t np;
	const struct prop_s *pp;

	if (addr >= maplen) return NULL;
	pp = map[addr];
	np = 1;
	for (i = 0; i < pp->ndims; ++i) {
		if (inc == pp->dim[i].i) {
			np += pp->dim[i].r;
			break;
		}
	}
	np -= addr - pp->addr;
	if (np < *nprops) *nprops = np;
	return pp;
}

#endif  /* CONFIG_DMPMAP_INDEX */

#endif /*  __dmpmap_h__       */
