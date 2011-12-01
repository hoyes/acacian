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
/*
#include "marshal.h"
#include "acnlog.h"
*/
#include "propmap.h"
#include "acnmem.h"

/**********************************************************************/
#define MATCHVAL -1
#define match_eq(atst) ((atst) < 0)

/*
const struct prophd_s termprop = {
	.addr = 0,
	.atst = 32,
	.nxt = {(prophd_t *)&termprop, (prophd_t *)&termprop},
};
*/
#define TERMVAL -1
#define isterm(atst) ((atst) < 0)
#define testbit(addr, atst) ((addr >> atst) & 1)

static inline addrtst_t
getbit(uint32_t bits)
{
	addrtst_t rslt;

	for (rslt = 0; bits >>= 1; ++rslt);
	return rslt;
}

/**********************************************************************/
static prophd_t *
_findprop(propset_t *set, uint32_t addr)
{
	addrtst_t atst;
	prophd_t *tp;

	if ((tp = set->first) == NULL)
		return NULL;

	do {
		atst = tp->atst;
		tp = tp->nxt[testbit(addr, atst)];
	} while (tp->atst < atst);
	return tp;
}

/**********************************************************************/
prophd_t *
findprop(propset_t *set, uint32_t addr)
{
	prophd_t *tp;

	tp = _findprop(set, addr);
	if (tp && tp->addr == addr) return tp;
	return NULL;
}

/**********************************************************************/

int
findornewprop(propset_t *set, uint32_t addr, prophd_t **rslt, size_t size)
{
	addrtst_t atst;
	prophd_t *tp;
	prophd_t *np;
	prophd_t **pp;

	assert(size >= sizeof(prophd_t));
	/* find our point in the tree */
	tp = _findprop(set, addr);
	if (tp == NULL) {
		atst = TERMVAL;
	} else {
		uint32_t bit;

		bit = tp->addr ^ addr;
		if (bit == 0) {
			*rslt = tp;
			return 0;
		}
		/* find highest bit difference */
		atst = getbit(bit);
	}

	/* allocate a new record */
	{
		uint8_t *mp;
	
		mp = mallocx(size);
		memset(mp + sizeof(prophd_t), 0, size - sizeof(prophd_t));
		np = (prophd_t *)mp;
	}
	np->addr = addr;
	np->atst = atst;
	np->nxt[0] = np->nxt[1] = np;

	/* now work out where to put it */
	pp = &set->first;
	tp = set->first;

	if (tp) {
		while (1) {
			addrtst_t at;
	
			at = tp->atst;
			if (at <= atst) break;	/* internal link */
			pp = &tp->nxt[testbit(addr, at)];
			tp = *pp;
			if (tp->atst >= at) break;	/* external link */
		}
		np->nxt[testbit(addr, atst) ^ 1] = tp;
	}

	*rslt = *pp = np;
	return 1;
}

/**********************************************************************/
#if 0
int
addprop(propset_t *set, prophd_t *prop)
{
	addrtst_t atst;
	prophd_t *tp;
	prophd_t **pp;
	uint32_t bit;

	if (set->first == NULL) set->first = (prophd_t *)&termprop;
	/* find our point in the tree */
	tp = _findprop(set, prop->addr);
	bit = tp->addr ^ prop->addr;
	if (bit == 0) {	/* already there */
		if (tp != &termprop) return -1;
		bit = 1;
	}

	/* find highest bit difference */
	atst = getbit(bit);

	/* now work out where to put it */
	prop->atst = atst;
	/* find insert location */
	pp = &set->first;
	tp = set->first;

	while (1) {
		addrtst_t tl;

		tl = tp->atst;
		if (tl <= atst) break;	/* internal link */
		pp = &tp->nxt[testbit(prop->addr, tl)];
		tp = *pp;
		if (tp->atst >= tl) break;	/* external link */
	}

	*pp = prop;
	bit = testbit(prop->addr, atst);
	prop->nxt[bit] = prop;
	prop->nxt[bit ^ 1] = tp;

	return 0;
}
#endif
/**********************************************************************/
int
delprop(propset_t *set, prophd_t *prop)
{
	prophd_t *pext;	/* external parent node (may be self) */
	prophd_t *gpext; /* external grandparent node (may be set) */
	prophd_t **pint; /* internal parent link */
	prophd_t *tp;
	int bit = 0;

	gpext = NULL;
	pint = &set->first;
	tp = set->first;
	while (1) {
		pext = tp;
		bit = testbit(prop->addr, tp->atst);
		tp = tp->nxt[bit];
		if (tp->atst >= pext->atst)  {
			if (tp != prop) return -1;	/* our node is not there */
			break;
		}
		if (tp == prop) pint = &(pext->nxt[bit]); /* save internal parent */
		if (isterm(tp->atst)) break;
		gpext = pext;
	}
	/* move node pext to position of node to be deleted - may be null op */
	*pint = pext;
	/* re-link any children of pext */
	if (gpext)
		gpext->nxt[testbit(prop->addr, gpext->atst)] = pext->nxt[bit ^ 1];
	else {
		if (isterm(pext->atst)) {
			set->first = NULL;
			return 0;
		}
		set->first = pext->nxt[bit ^ 1];
	}
	/* replace tp with pext - may be null op */
	pext->atst = tp->atst;
	if (!isterm(pext->atst)) {
		pext->nxt[0] = tp->nxt[0];
		pext->nxt[1] = tp->nxt[1];
	} else {
		pext->nxt[0] = pext->nxt[1] = pext;
	}
	return 0;
}

