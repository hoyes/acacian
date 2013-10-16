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
/*
Logging facility
*/

#define lgFCTY LOG_DMP
#undef ACNCFG_LOGFUNCS
#define ACNCFG_LOGFUNCS ((LOG_ON) | LOG_DEBUG)
#undef ACNCFG_LOGLEVEL
#define ACNCFG_LOGLEVEL LOG_DEBUG
/**********************************************************************/
void
fillindexes(const struct dmpprop_s *prop, struct adspec_s *ads, uint32_t *indexes)
{
	uint32_t a0;
	const struct dmpdim_s *dp;
	uint32_t count = 1;

	a0 = ads->addr - prop->addr;
	for (dp = prop->dim; a0; ) {
		*indexes = a0 / dp->inc;
		if (dp->inc == ads->inc) {
			count = dp->cnt - *indexes;
			if (count > ads->count) count = ads->count;
			
		}
		a0 %= dp->inc;
		++dp;
		++indexes;
	}
	ads->count = count;
	if ((prop->dim + prop->ndims - dp) > 0)
		memset(indexes, 0, (uint8_t *)(prop->dim + prop->ndims) - (uint8_t *)dp);
}

/**********************************************************************/
static bool
dimmatch(const struct dmpdim_s *dp, int ndims, uint32_t a0, uint32_t maxad, uint32_t *t)
{
	if (a0 == 0) {  /* common case treated specially */
		return true;
	} else if (ndims > 1) {
		uint32_t b;
		int x;

		maxad -= *t;  /* maxad is maximum offset due to smaller dimensions */
		if (maxad > a0) b = 0;
		else b = a0 - a0 % dp->inc - maxad;
		for (x = b; x < *t; x += dp->inc) {
		   if (dimmatch(dp + 1, ndims - 1, a0 - x, maxad, t + 1)) {
				return true;
		   }
		}
	} else if (a0 % dp->inc == 0) {
		uint32_t q = a0 / dp->inc;

		if (q < dp->cnt) {
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
static bool
propmatch(const struct dmpprop_s *p, uint32_t addr)
{
	uint32_t maxad;
	uint32_t a0;
	uint32_t x;
	const struct dmpdim_s *dp;
	uint32_t *ip;
	uint32_t t[p->ndims];

	a0 = addr - p->addr;
	if (a0 == 0) {  /* special case when addr is first in array */
		return true;
	}
	if (a0 > p->ulim) return false;
	maxad = 0;
	if (p->ndims > 1) {
		for (dp = p->dim + p->ndims, ip = t + p->ndims; --dp >= p->dim;) {
			x = (dp->cnt - 1) * dp->inc;
			if (a0 < x) x = a0 - (a0 % dp->inc);  /* truncate to multiple of inc */
			*--ip = x;
			maxad += x;
		}
	}
	return dimmatch(p->dim, p->ndims, a0, maxad, t);
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
const struct dmpprop_s *
addr_to_prop(union addrmap_u *amap, uint32_t addr)
{
	const struct dmpprop_s *prop;

	LOG_FSTART();
	switch (amap->any.type) {
	case am_indx: {
		uint32_t ofs;

		ofs = addr - amap->indx.base;
		if (ofs >= amap->indx.range
			|| (prop = amap->indx.map[ofs]) == NULL)
		{
			return NULL;
		}
		return prop;
	}
	case am_srch: {
		struct addrfind_s *af;

		if ((af = addr_to_map(amap, addr)) == NULL) return NULL;
		switch (af->ntests) {
		case 0:
			return af->p.prop;
		case 1:
			prop = af->p.prop;
			return propmatch(prop, addr) ? prop : NULL;
		default: {
			struct dmpprop_s **pa;
			int i;

			pa = af->p.pa;
			for (i = 0; i < af->ntests; ++i) {
				if (propmatch(pa[i], addr)) return pa[i];
			}
			return NULL;
		}}
	}
	default:
		acnlogmark(lgERR, "Unrecognized address map type %d", amap->any.type);
		return NULL;
	}
	LOG_FEND();
}
/**********************************************************************/
/*
*/
static void
freesrchmap(struct addrfind_s *afarray, int count)
{
	struct addrfind_s *af;

	LOG_FSTART();
	for (af = afarray; af < afarray + count; ++af)
		if (af->ntests > 1) free(af->p.pa);
	free(afarray);
	LOG_FEND();
}

/**********************************************************************/
/*
func: freeamap
*/
void
freeaddramap(union addrmap_u *amap)
{
	LOG_FSTART();
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
	LOG_FEND();
}

/**********************************************************************/
void
indexprop(struct dmpprop_s *prop, struct dmpprop_s **imap, int dimx, uint32_t ad)
{
	uint32_t i;
	struct dmpdim_s *dp;

	LOG_FSTART();
	if (dimx--) {
		dp = prop->dim + dimx;
		for (i = 0; i < dp->cnt; ++i) {
			if (dimx == 0) imap[ad] = prop;
			else indexprop(prop, imap, dimx, ad);
			ad += dp->inc;
		}
	} else imap[ad] = prop;
	LOG_FEND();
}
/**********************************************************************/
/*
func: xformtoindx
*/
void
xformtoindx(union addrmap_u *amap)
{
	struct dmpprop_s **imap;
	uint32_t base, range;
	struct addrfind_s *af;
	uint32_t i;
	struct dmpprop_s *prop;

	LOG_FSTART();
	assert(amap->any.type == am_srch);
	af = amap->srch.map;
	base = af->adlo;
	range = (af + amap->srch.count - 1)->adhi + 1 - base;
	imap = (struct dmpprop_s **)mallocxz(sizeof(struct dmpprop_s *) * range);
	
	for (; af < amap->srch.map + amap->srch.count; ++af) {
		if (af->ntests < 2) {
			prop = af->p.prop;
			indexprop(prop, imap, prop->ndims, prop->addr - base);
		} else {
			for (i = 0; i < af->ntests; ++i) {
				prop = af->p.pa[i];
				indexprop(prop, imap, prop->ndims, prop->addr - base);
			}
		}
	}
	freesrchmap(amap->srch.map, amap->srch.count);
	amap->indx.type = am_indx;
	amap->indx.map = imap;
	amap->indx.size = sizeof(struct dmpprop_s *) * range;
	amap->indx.range = range;
	amap->indx.base = base;
	/* maxdims and flags do not change */
	LOG_FEND();
}
