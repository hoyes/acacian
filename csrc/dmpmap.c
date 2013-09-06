/**********************************************************************/
/*

	Copyright (c) 2011, Philip Nye, Engineering Arts (UK) philip@engarts.com
	All rights reserved.

	Author: Philip Nye

	$Id$

#tabs=3t
*/
/**********************************************************************/
/*
Incoming DMP requests need a DMP address to resolve very rapidly to 
an internal property structure. For a device it is possible to hand 
craft the address space and make the algorithm to fit, but for the 
generic case this is not possible and with a 32-bit address space, a 
simple table is not feasible.

As with UUIDs we may have various options including linear search,
tree searches or hashing.

Current radix search does not yet support array addressing - this means
we need a separate property structure for every element of an array!

Unlike uuids there are no illegal values for address so we cannot 
use a special value as a terminator - therefore we assign a special 
value of atst to a terminal node.
*/

/**********************************************************************/

#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include "acn.h"

/**********************************************************************/
static void
fillindexes(struct dmpprop_s *prop, uint32_t addr, uint32_t indexes)
{
	uint32_t a0;
	struct dmpdim_s *dp;

	a0 = addr - prop->addr;
	for (dp = prop->dim; a0; ) {
		indexes[dp->lvl] = a0 / dp->i;
		a0 %= dp->i;
		++dp;
	}
	while (dp < prop->dim + prop->ndims) indexes[(dp++)->lvl] = 0;
}

/**********************************************************************/
static bool
dimmatch(struct dmpdim_s *dp, int ndims, uint32_t a0, uint32_t maxad, uint32_t *t, uint32_t *indexes)
{
	if (a0 == 0) {  /* common case treated specially */
		if (indexes) while (ndims--) indexes[(dp++)->lvl] = 0;
		return true;
	} else if (ndims > 1) {
		uint32_t b;
		int x;

		maxad -= *t;  /* maxad is maximum offset due to smaller dimensions */
		if (maxad > a0) b = 0;
		else b = a0 - a0 % dp->i - maxad;
		for (x = b; x < *t; x += dp->i) {
		   if (dimmatch(dp + 1, ndims - 1, a0 - x, maxad, t + 1)) {
				indexes[dp->lvl] = x / dp->i;
				return true;
		   }
		}
	} else if (a0 % dp->i == 0) {
		uint32_t q = a0 / dp->i;
		
		if (q <= dp->r) {
		   indexes[dp->lvl] = q;
			return true;
		}
	}
	return false;
}

/**********************************************************************/
#define MAXDIMS 20
/*
func: propmatch

See DMP.txt for discussion and algorithm.
*/
static const uint32_t zero32[MAXDIMS] = { 0 };

static bool
propmatch(struct dmpprop_s *p, uint32_t addr, uint32_t *indexes)
{
	uint32_t t[MAXDIMS];
	uint32_t maxad;
	uint32_t a0;
	uint32_t x;
	struct dmpdim_s *dp;
	uint32_t *ip;

	a0 = addr - p->addr;
	if (a0 > p->ulim) return false;
	if (a0 == 0) {  /* special case when addr is first in array */
		if (indexes) memset(indexes, 0, sizeof(*indexes) * p->ndims);
		return true;
	}
	maxad = 0;
	if (p->ndims > 1) {
		for (dp = p->dim + p->ndims, ip = t + p->ndims; dp-- > p->dim;) {
			x = dp->r * dp->i;
			if (a0 < x) x = a0 - (a0 % dp->i);  /* truncate to multiple of inc */
			*--ip = x;
			maxad += x;
		}
	}
	return dimmatch(p->dim, p->ndims, a0, maxad, t, indexes);
}

/**********************************************************************/
/*
func: addr_to_map

Search the map for an address

Returns the region in the map which contains the given address, or 
NULL if the address does not match any region.
*/
static struct addrfind_s *
addr_to_map(union addrmap_u *amap, uint32_t addr)
{
	struct addrfind_s *af, *alo;
	int span;

	/* search the map for our insertion point */
	alo = amap->srch.map;
	span = amap->srch.count;
	while (span ) {
		af = alo + span / 2;
		if (addr < af->adlo) span = span / 2;
		else if (addr <= af->adhi) return af;
		else {
			span = (span - 1) / 2;
			alo = af + 1;
		}
	}
	return NULL;
}

/**********************************************************************/
/*
func: addr_to_prop

Find a property given an address.

First search the map for the region containing our address, then 
make any tests necessary to find a property whose address actually 
matches. See DMP.txt for discussion and algorithm.

Returns the property corresponding to the address or NULL if there 
is none.
*/
struct dmpprop_s *
addr_to_prop(union addrmap_u *amap, uint32_t addr, uint32_t *indexes)
{
	struct dmpprop_s *prop;

	switch (amap->any.type) {
	case am_indx: {
		uint32_t ofs;

		ofs = addr - amap->indx.base;
		if (ofs >= amap->indx.count) return NULL;
		if ((prop = amap->indx.map[ofs]) == NULL) return NULL;
		if (indexes) fillindexes(prop, addr, indexes);
		return prop;
	case am_srch: {
		struct addrfind_s *af;

		if ((af = addr_to_map(amap, addr)) == NULL) return NULL;
		switch (af->ntests) {
		case 0:
			prop = af->p.prop;
			if (indexes) fillindexes(prop, addr, indexes);
			return prop;
		case 1:
			prop = af->p.prop;
			return propmatch(prop, addr, indexes) ? prop : NULL;
		default: {
			struct dmpprop_s **pa;
			int i;

			pa = af->p.pa;
			for (i = 0; i < af->ntests; ++i) {
				if (propmatch(pa[i], addr, indexes)) return pa[i];
			}
			return NULL;
		}}
	}
	default:
		acnlogmark(lgERR, "Unrecognized address map type %d", amap->type);
		return NULL;
	}
}
/**********************************************************************/
/*
*/
static void
freesrchmap(struct addrfind_s *afarray, int count)
{
	struct addrfind_s *af;

	for (af = afarray; af < afarray + count; ++af)
		if (af->ntests > 1) free(af->p.pa);
	free(af);
}

/**********************************************************************/
/*
func: freeamap
*/
void
freeaddramap(union addrmap_u amap)
{
	switch (amap->any.type) {
	case am_srch:
		freesrchmap(amap->srch.map, amap->srch.count);
		break;
	default:
		acnlogmark(lgWARN, "Freeing unknown map type");
		/* fall through */
	case am_indx:
		free(amap->any.map);
		break;
	}
	free(amap);
}

/**********************************************************************/
indexprop(struct dmpprop_s *prop, struct dmpprop_s **imap, int dimx, uint32_t ad)
{
	uint32_t i;
	struct dmpdim_s *dp;

	if (dimx--) {
		dp = prop->dim + dimx;
		for (i = 0; i <= dp->r; ++i) {
			if (dimx == 0) imap[ad] = prop;
			else indexprop(prop, imap, dimx, ad);
			ad += dp->i;
		}
	} else imap[ad] = prop;
}
/**********************************************************************/
/*
func: xformtoindx
*/
void
xformtoindx(union addrmap_u *amap)
{
	struct dmpprop_s *imap;
	uint32_t base, range;
	struct addrfind_s *af;
	uint32_t i;
	struct dmpmap_s *prop;

	af = amap->srch.map;
	base = af->adlo;
	range = (af + amap->srch.count - 1)->adhi + 1 - base;
	imap = (uint32_t *)mallocxz(sizeof(uint32_t) * range);
	for (; af < amap->srch.map + amap->srch.count; ++af) {
		if (af->ntests < 2) {
			prop = af->p.prop;
			indexprop(prop, imap, prop->ndims, prop->addr - base);
		} else {
			for (i = 0; i < af->ntests; ++i) {
				prop = af->p.pa[i];
				indexprop(prop, imap, prop->ndims, base);
			}
		}
	}
	freesrchmap(amap->srch.map, amap->srch.count);
	amap->indx.type - am_indx;
	amap->indx.map = imap;
	amap->indx.size = sizeof(*imap) * range;
	amap->indx.base = base;
}
