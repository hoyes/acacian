/**********************************************************************/
/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (c) 2013, Acuity Brands, Inc.

Author: Philip Nye <philip.nye@engarts.com>

This file forms part of Acacian a full featured implementation of 
ANSI E1.17 Architecture for Control Networks (ACN)

#tabs=3
*/
/**********************************************************************/
/*
file: dmpmap.c

DMP address and property handling. Mapping of device properties within
the 32-bit DMP address space.

topic: Format and Strategy for finding properties

On receipt of a property message (and this means most DMP messages), 
the property address must be mapped onto the internal functionality 
for that property. In a device, this probably means making a local 
function call to find the property's value or to initiate the action 
implied by setting the property. In a controller, this means mapping 
the received value or message into the specific remote device instance 
and property within that instance that the controller uses to do its 
work.

To perform this operation the code needs an address to object mapping
and since it is used for every received DMP message it should be 
efficient.

Direct Mapping:

The simplest and fastest method is direct mapping using the property 
address as an array address. For a device with a limited number of 
properties packed closely together within the address space, this is 
ideal.

However direct mapping does not generalize and is not suitable for 
controllers which have no control over the DDL for the devices they 
encounter. For cases where direct mapping is suitable a conversion
function is available to create a direct map from a more general one.

Array Properties:

A further complication is introduced by the fact that both DDL and DMP 
can express stepwise iteration through the address space with complete 
freedom of starting point, step size and address count. This is 
invaluable for large arrays but complicates matters.

topic: Algorithm for address search

The parser builds a single table (a 1 dimensional array) in which 
each each entry E defines a region within the space by a low address 
and a high address E_lo and E_hi (inclusive values). The array 
entries are non-overlapping and sorted in address order. Thus a 
straightforward binary search identifies whether an arbitrary 
address falls within one of these entries. If no entry is found then 
the address is not within the device.

Each `single` property in the device has a corresponding entry in this 
table for which both the low address and high address are the same. 
An `array` property likewise may form a single entry in which the low 
and high addresses are the extents of the array and, if the array is 
packed (occupies every address within its range), forms a single, 
complete entry. However, array properties may be sparse: if the 
increment of the array - or the smallest increment for 
multidimensional arrays - is greater than one, the array contains 
holes. Furthermore, these holes may legitimately be occupied by 
other properties - it is common to encounter inteleaved arrays of 
related properties - and interleaving can extend to multiple 
multidimensioned arrays.

So having found that an address falls within the extents of an 
array, it may need further testing to find whether it is actually a 
member, or whether it falls within a hole, and in the case of 
interleaved array properties we need to know which property it matches.

To handle these cases, each entry in the address table contains a 
test counter `ntests`. When an address falls within the range defined 
by the entry, ntests indicates how many array properties may overlap 
in that region and need to be tested for membership.

If ntests == 0 the address region has no holes and all its addresses 
unambiguously belong to a single property and the search is 
complete. This is always the case for singular properties where E_lo 
and E_hi are the same. It is also the case for packed array 
properties.

If ntests == 1 then there is only one property occupying this region 
but its address space contains holes and a further test is required 
to establish a match.

If ntests > 1 then we have interleaving and must test each relevant 
property for an address match.

topic: Finding the Array Member

Having established that an address matches an array property the 
application will usually want to identify the individual element. 
This is fairly simple arithmetic for a well behaved property - the 
property's entry <dmpprop_s> includes details of the range and 
increment for each of its dimensions. However, properties with 2 or 
more dimensions can be created whose address patterns are 
self-interleaving - i.e. the addresses at the end of one 'row' 
interleave with those at the start of the next. The code correctly 
handles these cases but designing a device this way is strongly 
discouraged and likely to lead to poor support and performance.
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
#undef CF_LOGFUNCS
#define CF_LOGFUNCS ((LOG_ON) | LOG_DEBUG)
#undef CF_LOGLEVEL
#define CF_LOGLEVEL LOG_DEBUG
/**********************************************************************/
#if 0
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
#endif
/**********************************************************************/
static bool
dimmatch(const struct dmpdim_s *dp, int ndims, uint32_t a0, uint32_t maxad, uint32_t *t)
{
	LOG_FSTART();
	acnlogmark(lgDBUG, "dimmatch ndims %i, a0 %u, maxad %u, *t %u", ndims, a0, maxad, *t);
	if (a0 == 0) {  /* common case treated specially */
		LOG_FEND();
		acnlogmark(lgDBUG, "zero remainder");
		return true;
	} else if (ndims > 1) {
		uint32_t b;
		int x;

		maxad -= *t;  /* maxad is maximum offset due to smaller dimensions */
		if (maxad >= a0) b = 0;
		else b = a0 - a0 % dp->inc - maxad;
		for (x = b; x <= *t; x += dp->inc) {
		   if (dimmatch(dp + 1, ndims - 1, a0 - x, maxad, t + 1)) {
				LOG_FEND();
				return true;
		   }
		}
	} else if (a0 % dp->inc == 0) {
		uint32_t q = a0 / dp->inc;

		if (q < dp->cnt) {
			acnlogmark(lgDBUG, "dimension fit");
			LOG_FEND();
			return true;
		}
	}
	acnlogmark(lgDBUG, "no match");
	LOG_FEND();
	return false;
}

/**********************************************************************/
#define MAXDIMS 20
/*
func: propmatch

Test whether an address matches a property.
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

	LOG_FSTART();
	a0 = addr - p->addr;
	acnlogmark(lgDBUG, "Offset %u", a0);

	if (a0 >= p->span) return false;
	/* common easy cases - first element or in range and packed */
	if (a0 == 0 || (p->flags & pflg(packed))) return true;
	if ( !(p->flags & pflg(overlap))) {
		for (dp = p->dim; dp < p->dim + p->ndims; ++dp) {
			if (a0 >= (dp->inc * dp->cnt)) return false;
			a0 %= dp->inc;
			if (a0 == 0) return true;
		}
		return false;
	}
	/* worst case - we have a self-overlapping array */
	maxad = 0;
	if (p->ndims > 1) {
		for (dp = p->dim + p->ndims, ip = t + p->ndims; --dp >= p->dim;) {
			x = (dp->cnt - 1) * dp->inc;
			if (a0 < x) x = a0 - (a0 % dp->inc);  /* truncate to multiple of inc */
			*--ip = x;
			maxad += x;
			acnlogmark(lgDBUG, "dim %i, x %u, maxad %u", (int)(dp - p->dim), x, maxad);
		}
	}
	acnlogmark(lgDBUG, "maxad %u", maxad);
	LOG_FEND();
	return dimmatch(p->dim, p->ndims, a0, maxad, t);
}

/**********************************************************************/
/*
func: addr_to_map

Given an address, search the map for a property which might match.

Returns the region in the map which contains the given address, or 
NULL if the address does not match any region.

If the region corresponds to a single non-array property or a property
whose address range is packed (the property uses all addresses 
within the range) then no further tests are needed.

If the region corresponds to multiple overlapping array properties, or 
to a single sparse array, further testing is needed to confirm whether
the address is a hit and if so to which property.
*/
static struct addrfind_s *
addr_to_map(union addrmap_u *amap, uint32_t addr)
{
	struct addrfind_s *af, *alo;
	int span;

	LOG_FSTART();
	/* search the map for our insertion point */
	acnlogmark(lgDBUG, "Search map for address %u", addr);
	alo = amap->srch.map;
	span = amap->srch.count;
	while (span ) {
		af = alo + span / 2;
		acnlogmark(lgDBUG, "trying %u..%u", af->adlo, af->adhi);
		if (addr < af->adlo) span = span / 2;
		else if (addr <= af->adhi) {
			LOG_FEND();
			return af;
		} else {
			span = (span - 1) / 2;
			alo = af + 1;
		}
	}
	acnlogmark(lgDBUG, "Address not in map");
	LOG_FEND();
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

		acnlogmark(lgDBUG, "Direct index map");
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

		acnlogmark(lgDBUG, "Binary search map");
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
freeamap(union addrmap_u *amap)
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
