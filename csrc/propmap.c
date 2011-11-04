/* vi: set sw=3 ts=3: */
/**********************************************************************/
/*
`
	Copyright (c) 2011, Philip Nye, Engineering Arts (UK) philip@engarts.com
	All rights reserved.

	Author: Philip Nye

	$Id$

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
#include "acncommon.h"

struct proprefa_s {
	uint32_t mod;
	uint32_t lo;
	uint32_t hi;
	void *ref;
};

struct proptablea_s {
	struct proptablea_s *nxt;
	int tabsize;
	uint32_t inc;
	struct proprefa_s refs[];
};

/* First table is a special case */
struct propref1_s {
	uint32_t lo;
	uint32_t hi;
	void *ref;
};

struct propmap_s {
	struct proptablea_s *nxt;
	int tabsize;
	struct propref1_s refs[];
};

uint32_t
findprop(addr, inc, count, struct propmap_s *propmap, void **refp)
{
	struct propref1_s ptr1;
	struct proprefa_s ptra;
	uint32_t mod;
	unsigned int s;

	s = propmap->tabsize;
	ptr = propmap->refs;

	do {
		s = s/2;
		ptr1 += s;
		if (addr < ptr1->lo)
			ptr1 -= s;
		else if (addr > ptr1->hi)
			ptr1 += 1;
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
	while (s);

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

