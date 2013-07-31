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
#if ACNCFG_DMPMAP_SEARCH
/*

*/
/**********************************************************************/
static bool
dimmatch(struct dmpdim_s *dp, int ndims, uint32_t a0, uint32_t maxad, uint32_t *t)
{
	if (a0 == 0) {  /* common case treated specially */
		memset(t, 0, sizeof(*t) * ndims);
		return true;
	} else if (ndims > 1) {
		uint32_t b;
		int x;

		maxad -= *t;  /* maxad is maximum offset due to smaller dimensions */
		if (maxad > a0) b = 0;
		else b = a0 - a0 % dp->i - maxad;
		for (x = b; x < *t; x += dp->i) {
		   if (dimmatch(dp + 1, ndims - 1, a0 - x, maxad, t + 1)) {
				*t = x / dp->i;
				return true;
		   }
		}
	} else if (a0 % dp->i == 0) {
		uint32_t q = a0 / dp->i;
		
		if (q <= dp->r) {
		   *t = q;
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

	assert(addr >= p->addr);
	a0 = addr - p->addr;
	if (a0 == 0) {  /* special case when addr is first in array */
		if (indexes) memset(indexes, 0, sizeof(*indexes) * p->ndims);
		return true;
	}
	if (indexes == NULL) indexes = t;
	maxad = 0;
	if (p->ndims > 1) {
		for (dp = p->dim + p->ndims, indexes += p->ndims; dp-- > p->dim;) {
			x = dp->r * i;
			if (a0 < x) x = a0 - (a0 % dp->i);  /* truncate to multiple of inc */
			*--indexes = x;
			maxad += x;
		}
	}
	return dimmatch(p->dim, p->ndims, a0, maxad, indexes);
}

/**********************************************************************/
/*
func: addr_to_map

Search the map for an address

Returns the region in the map which contains the given address, or 
NULL if the address does not match any region.
*/
static struct addrfind_s *
addr_to_map(struct addrmap_s *amap, uint32_t addr)
{
	struct addrfind_s *af, *alo;
	int span;

	/* search the map for our insertion point */
	alo = amap->map;
	span = amap->h.count;
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
addr_to_prop(struct addrmap_s *amap, uint32_t addr, uint32_t *indexes)
{
	struct addrfind_s *af;
	struct dmpprop_s *prop;
	void *vp;
	int i;

	if ((af = addr_to_map(amap, addr)) == NULL) return NULL;
	vp = af->p;
	if ((i = af->ntests) == 0) return (struct dmpprop_s *)vp;
	do {
		if (--i == 0) prop = (struct dmpprop_s *)vp;
		else prop = ((struct addrtest_s *)vp)->prop;
		if (propmatch(prop, addr, indexes)) return prop;
		vp = ((struct addrtest_s *)vp)->nxt;
	} while (i);
	return NULL;
}
#endif /* ACNCFG_DMPMAP_SEARCH */
/**********************************************************************/

#if 0

uint32_t
findprop(addr, struct addrmap_s *amap, void **refp)
{
	unsigned int hi, lo, i;

	lo = 0; hi = amap->h.count;

	do {
		i = (lo + hi) / 2;
		if (addr > 
		ptr1 = ptr + i;
		if (addr < ptr1->lo)
			hi = i;
		else if (addr > ptr1->hi)
			lo = i + 1;
		else {
#if CONFIG_DMP_MATCH_INC
			if (inc == 1) {
				uint32_t n;
				n = ptr1->hi - addr + 1;
				if (count > n) count = n;
			} else count = 1;
#else
			uint32_t n;
			n = (ptr1->hi - addr)/inc + 1;
			if (count > n) count = n;
#endif
			*refp = ptr1->ref;
			return count;
		}
	while (hi > lo);

	for (ptab = propmap->nxt; ptab != NULL; ptab = ptab->nxt) {

		mod = addr % ptab->inc;
		s = ptab->tabsize;
		ptra = ptab->refs;
	
		do {
			s = s/2;
			ptra += s;
	
			if (mod > ptra->mod)
				ptra += 1;
			else if (mod < ptra->mod || addr < ptra->lo)
				ptra -= s;
			else if (addr > ptra->hi)
				ptra += 1;
			else {
#if CONFIG_DMP_MATCH_INC
				if (inc == ptab->inc)
#else
				if (inc % ptab->inc == 0)
#endif
				{
					uint32_t n;
					
					n = (ptra->hi - addr)/inc + 1;
					if (count > n) count = n;
				} else count = 1;
				*refp = ptra->ref;
				return count;
			}
		} while (s);
	}
	return NULL;
}

#endif
