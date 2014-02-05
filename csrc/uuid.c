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
file: uuid.c

Manipulation and tracking of UUIDs
*/

#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>
#include "acn.h"
#include "tohex.h"

const uint8_t  null_uuid[UUID_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

const signed char hexdigs[256] = {
	/* 00 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* 08 */ c_BAD,c_SPC,c_SPC,c_BAD,c_BAD,c_SPC,c_BAD,c_BAD,
	/* 10 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* 18 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* 20 */ c_SPC,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* 28 */ c_BAD,c_BAD,c_BAD,c_BAD,c_IGN,c_IGN,c_IGN,c_BAD,
	/* 30 */     0,    1,    2,    3,    4,    5,    6,    7, 
	/* 38 */     8,    9,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* 40 */ c_BAD,   10,   11,   12,   13,   14,   15,c_BAD,
	/* 48 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* 50 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* 58 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* 60 */ c_BAD,   10,   11,   12,   13,   14,   15,c_BAD,
	/* 68 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* 70 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* 78 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* 80 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* 88 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* 90 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* 98 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* a0 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* a8 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* b0 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* b8 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* c0 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* c8 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* d0 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* d8 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* e0 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* e8 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* f0 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
	/* f8 */ c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,c_BAD,
};

/**********************************************************************/
/*
about: UUID structure

> timelow  tmid Vthi Rseq node
> xxxxxxxx-xxxx-VVxx-RRxx-xxxxxxxxxxxx
> 
> VV = version
>      1x => time based
>      2x => DCE/Posix
>      3x => name based w MD5
>      4x => random
>      5x => name based w SHA1
>      
> RR = variant
>    = 10xx xxxx for UUID spec
>       others are NCS, Microsoft proprietary etc.
> 
> Char
> 0         1         2         3
> 012345678901234567890123456789012345
> xxxxxxxx-xxxx-VVxx-RRxx-xxxxxxxxxxxx
> 
> Octet
>                         1 1 1 1 1 1
> 0 1 2 3  4 5  6 7  8 9  0 1 2 3 4 5
> xxxxxxxx-xxxx-VVxx-RRxx-xxxxxxxxxxxx

*/
/**********************************************************************/
/*
func: str2uuid

Parse a UUID string and optionally convert to binary UUID

If argument `uuid` is NULL there is no output but the string is still
fully parsed.

return:
	0 on success or -1 on error
*/
int
str2uuid(const char *uuidstr, uint8_t *uuid)
{
	int i;
	int INITIALIZED(hib);
	int lob;

	if (uuidstr == NULL) return -1;
	i = 0;
	while (1) {
		switch (i++) {
		case  0: case  2: case  4: case  6:
		case  9: case 11: case 16: case 21: 
		case 24: case 26: case 28: case 30: case 32: case 34:
			if ((hib = hexdigs[(uint8_t)*uuidstr++]) < 0) return -1;
			break;
		case  1: case  3: case  5: case  7: 
		case 10: case 12: case 15: case 17: 
		case 20: case 22: case 25: case 27: 
		case 29: case 31: case 33: case 35:
			if ((lob = hexdigs[(uint8_t)*uuidstr++]) < 0) return -1;
			if (uuid) *uuid++ = (hib << 4) | lob;
			break;
		case  8: case 13: case 18: case 23:
			if (*uuidstr++ != '-') return -1;
			break;
		case 14:
			if ((hib = hexdigs[(uint8_t)*uuidstr++]) < 1 || hib > 5) return -1;
			break;
		case 19:
			if (((hib = hexdigs[(uint8_t)*uuidstr++]) & 0x0c) != 0x08) return -1;
			break;
		case 36:
			/*
			if (*uuidstr != 0) return -1;
			*/
			return 0;
		}
	}
}

/**********************************************************************/
/*
func: uuid2str

generate a UUID string
Return a pointer to start of string
String is fixed length CID_STR_SIZE (including Nul terminator)
e.g.
a834b30c-6298-46b3-ac59-c5a0286bb599
19e50ed0-a4f3-11e2-a1cb-0017316c497d
*/
char *
uuid2str(const uint8_t *uuid, char *uuidstr)
{
	int i;
	char *cp;

	cp = uuidstr;
	for (i = 16; i; --i) {
		*cp++ = tohex(*uuid >> 4);
		*cp++ = tohex(*uuid & 0x0f);
		++uuid;
		switch(i) {
		case 13 :
		case 11 :
		case 9 :
		case 7 :
			*cp++ = '-';
		default :
			break;
		}
	}
	*cp = '\0';  /* terminate */
	return uuidstr;
}

/**********************************************************************/
/* Only provide these if not defined as macros */
#ifndef uuidIsNull

bool
uuidIsNull(const uint8_t *uuid)
{
	int count = UUID_SIZE;

	while (*uuid++ == 0) if (--count == 0) return true;
	return false;
}
#endif
/**********************************************************************/

#if !defined(uuidsEq)
bool
uuidsEq(const uint8_t *uuid1, const uint8_t *uuid2);
{
	int count = UUID_SIZE;

	while (*uuid1++ == *uuid2++) if (--count == 0) return true;
	return false;
}
#endif
/**********************************************************************/

#if !defined(uuidcpy)
uint8_t *
uuidcpy(uint8_t *dst, const uint8_t *src)
{
	uint8_t dp = dst;
	int count = UUID_SIZE;

	while (count--) *dp++ = *src++;
	return dst;
}
#endif
/**********************************************************************/
/*
group: UUID search
*/
/**********************************************************************/
#if ACNCFG_UUIDS_RADIX
/**********************************************************************/
/*
about: Radix search using Patricia tree

This tree uses the technique described in Sedgewick: "Algorithms" 
for folding the external nodes back into internal nodes - the value 
within the node is ignored when descending the tree (as the node is 
being treated as an internal one) and only the testbit is 
considered. Only on detecting an external node (detected by a 
testbit value in the child which preceeds the current one) is the 
value stored within that child node considered.

Unlike Sedgewick and most common examples we descend the tree in 
order of *increasing* bit number, but we number the bits in 
individual bytes in reverse of normal so MSB of the first byte is 
bit 0. This means that UUIDs sort in numericasl order if treated as 
a single big-endian number.

Efficiency of inner loop is highly dependent on processor 
instruction set so may need to be optimized. To facilitate this We 
define two inline functions which are used throughout to test or 
calculate the necessary bit differences:

  bool testbit(uint8_t *uuid, tstloc_t tstloc)

returns 1 or 0 according to whether the bit in uuid identified by 
tstloc is set or cleared. tstloc_t may be defined as an integer or a 
bitflag or a combination but must sort in increasing order when the 
bits are numbered as above.

  tstloc_t matchloc(uuid1, uuid2) 

calculates the value of tstloc corresponding to the first (lowest 
numbered) bits which differ between two uuids - or returns MATCHVAL 
if they are equal;

The implementation here is well suited to many architectures. tstloc 
is a combination of the byte number (in bits 11..8) and a bitmask in 
bits 7..0. The bit mask is the inverse if the bit to be tested (e.g. 
0x7f to test bit 0 - numbered down from MSB remember). This ensures 
it sorts in increasing numerical order as required.

Note: Termination - there is a dangling link special case owing to 
the fact that any tree has one more external links than it has nodes 
(work it out!) There are several possibilities for dealing with 
this. We use a special value for tstloc to mark the terminating node 
and then have to treat it as a special case in a few places. The 
termination value needs to have two properties: First, it needs to 
sort after (be greater than) any "genuine" value. Second, it gets 
used in tests so whilst the result of such a test does not matter, 
it must be a valid 0..1 value and cannot attempt access of a byte 
outside the uuid.

Note: Because we need to test against UUIDs from packets we can make 
no guarantees about alignment of supplied UUID.
*/
/**********************************************************************/
/*
Memory allocation for tracking
*/
#define new_uuidtrk() ((struct uuidtrk_s *)mallocx(sizeof(struct uuidtrk_s)))
#define free_uuidtrk(x) free(x)

/**********************************************************************/
/*
Use macros to allow different uuidtst_t implementations
*/
#define MATCHVAL 0
#define match_eq(tstloc) ((tstloc) == MATCHVAL)

/*
func: matchuuid

Test for exact match - don't use uuidsEq() because if we fail we 
want to record the point of difference.

*/
static inline uuidtst_t
matchuuid(const uint8_t *uuid1, const uint8_t *uuid2)
{
	int tstloc;
	const uint8_t *cp1, *cp2;
	uint8_t b;
	uint8_t m;

	tstloc = 0xff;
	cp1 = uuid1;
	cp2 = uuid2;
	while ((b = *cp1 ^ *cp2) == 0) {
		tstloc += 0x100;
		if (tstloc >= (UUID_SIZE * 0x100)) return MATCHVAL;
		++cp1;
		++cp2;
	}

	m = 0x80;
	if ((b & 0xf0) == 0) {
		b <<= 4;
		m >>= 4;
	}
	if ((b & 0xc0) == 0) {
		b <<= 2;
		m >>= 2;
	}
	if ((b & 0x80) == 0) {
		m >>= 1;
	}
	return tstloc ^ m;
}

/**********************************************************************/
/*
func: testbit

Return 1 if the bit from uuid defined by tstloc is set, 0 if clear.
*/
static inline int
testbit(const uint8_t *uuid, uuidtst_t tstloc)
{
	return (((unsigned)(uuid[tstloc >> 8] | (uint8_t)tstloc)) + 1) >> 8;
}

/**********************************************************************/
/*
func: _finduuid

Find the tree entry where uuid should go. This is the nearest value
to uuid but not necessarily equal to it.
*/
static struct uuidtrk_s *
_finduuid(struct uuidset_s *set, const uint8_t *uuid)
{
	unsigned int tstloc;
	struct uuidtrk_s *tp;

	if ((tp = set->first) != NULL) {
		do {
			tstloc = tp->tstloc;
			tp = tp->nxt[testbit(uuid, tstloc)];
		} while (tp->tstloc > tstloc);
	}
	return tp;
}
/**********************************************************************/
const uint8_t *
finduuid(struct uuidset_s *set, const uint8_t *uuid)
{
	struct uuidtrk_s *tp;

	tp = _finduuid(set, uuid);
	if (tp == NULL || !uuidsEq(tp->uuid, uuid)) return NULL;
	return tp->uuid;
}

/**********************************************************************/
const uint8_t *
findornewuuid(struct uuidset_s *set, const uint8_t *uuid, size_t *create)
{
	uuidtst_t tstloc;
	struct uuidtrk_s *tp;
	struct uuidtrk_s *np;
	struct uuidtrk_s **pp;
	uint8_t *nstruct;

	assert(*create >= UUID_SIZE);
	if ((tp = _finduuid(set, uuid)) == NULL) {
		tstloc = UUTERM;
	} else {
		/* find lowest bit difference (or match) */
		tstloc = matchuuid(uuid, tp->uuid);
		if (match_eq(tstloc)) return tp->uuid;
	}
	/* uuid is not in set - create an entry for it */
	/* allocate a new uuidtrk_s */
	if ((nstruct = acnalloc(*create)) == NULL)
		return NULL;
	np = new_uuidtrk();
	uuidcpy(nstruct, uuid);
	memset(nstruct + UUID_SIZE, 0, *create - UUID_SIZE);
	np->uuid = nstruct;
	/* now work out where to put it */
	np->tstloc = tstloc;
	np->nxt[0] = np->nxt[1] = np;
	/* find insert location */
	pp = &set->first;
	tp = set->first;

	if (tp) {
		while (1) {
			uuidtst_t tl;
	
			tl = tp->tstloc;
			if (tl >= tstloc) break;	/* internal link */
			pp = &tp->nxt[testbit(uuid, tl)];
			tp = *pp;
			if (tp->tstloc <= tl) break;	/* external link */
		}
		np->nxt[testbit(uuid, tstloc) ^ 1] = tp;
	}
	*pp = np;
	*create = 0;
	return np->uuid;
}

/**********************************************************************/
int
adduuid(struct uuidset_s *set, const uint8_t *uuid)
{
	uuidtst_t tstloc;
	struct uuidtrk_s *tp;
	struct uuidtrk_s *np;
	struct uuidtrk_s **pp;

	/* find our point in the tree */
	tp = _finduuid(set, uuid);
	if (tp == NULL) {
		tstloc = UUTERM;
	} else {
		/* find lowest bit difference (or match) */
		tstloc = matchuuid(uuid, tp->uuid);
		if (match_eq(tstloc)) return -1;	/* already there */
	}
	/* allocate a new uuidtrk_s */
	np = new_uuidtrk();
	np->uuid = uuid;
	/* now work out where to put it */
	np->tstloc = tstloc;
	np->nxt[0] = np->nxt[1] = np;
	/* find insert location */
	pp = &set->first;
	tp = set->first;

	if (tp) {
		while (1) {
			uuidtst_t tl;
	
			tl = tp->tstloc;
			if (tl >= tstloc) break;	/* internal link */
			pp = &tp->nxt[testbit(uuid, tl)];
			tp = *pp;
			if (tp->tstloc <= tl) break;	/* external link */
		}
		np->nxt[testbit(uuid, tstloc) ^ 1] = tp;
	}
	*pp = np;
	return 0;
}

/**********************************************************************/
int
unlinkuuid(struct uuidset_s *set, const uint8_t *uuid)
{
	struct uuidtrk_s *pext;	/* external parent node (may be self) */
	struct uuidtrk_s *gpext; /* external grandparent node (may be set) */
	struct uuidtrk_s **pint; /* internal parent link */
	struct uuidtrk_s *tp;
	int bit;
	struct uuidtrk_s *uup;

	uup = _finduuid(set, uuid);
	if (uup == NULL || uup->uuid != uuid) return -1;

	gpext = NULL;
	pint = &set->first;
	tp = set->first;
	while (1) {
		pext = tp;
		bit = testbit(uuid, tp->tstloc);
		tp = tp->nxt[bit];
		if (tp->tstloc <= pext->tstloc) {
			break;
		}
		if (tp == uup) pint = &(pext->nxt[bit]); /* save internal parent */
		if (isuuterm(tp->tstloc)) break;
		gpext = pext;
	}
	/* move node pext to position of node to be deleted - may be null op */
	*pint = pext;
	/* re-link any children of pext */
	if (gpext) {
		gpext->nxt[testbit(uuid, gpext->tstloc)] = pext->nxt[bit ^ 1];
		/* replace tp with pext - may be null op */
		pext->tstloc = tp->tstloc;
		if (!isuuterm(pext->tstloc)) {
			pext->nxt[0] = tp->nxt[0];
			pext->nxt[1] = tp->nxt[1];
		} else {
			pext->nxt[0] = pext->nxt[1] = pext;
		}
	} else {
		if (!isuuterm(pext->tstloc)) {
			set->first = pext->nxt[bit ^ 1];
			/* replace tp with pext - may be null op */
			pext->tstloc = tp->tstloc;
			pext->nxt[0] = tp->nxt[0];
			pext->nxt[1] = tp->nxt[1];
		} else {
			set->first = NULL;
		}
	}
	free_uuidtrk(uup);
	return 0;
}

/**********************************************************************/
#elif ACNCFG_UUIDS_HASH
/**********************************************************************/
uint8_t *
finduuid(struct uuidset_s *set, const uint8_t *uuid)
{
	struct uuidtrk_s *cp;

	for (cp = set->table[uuidhash(uuid, set->mask)]; cp; cp = cp->rlnk) {
		if (uuidsEq(cp->uuid, uuid)) return cp->uuid;
	}
	return NULL;
}

/**********************************************************************/
int
adduuid(struct uuidset_s *set, uint8_t *uuid)
{
	struct uuidtrk_s **entry;
	struct uuidtrk_s *tp;

	entry = set->table + uuidhash(uuid, set->mask);

	while ((tp = *entry) != NULL) {
		if (uuidsEq(tp->uuid, uuid)) return -1;
		entry = &tp->rlnk;
	}
	tp = acnNew(struct uuidtrk_s);
	tp->uuid = uuid;
	*entry = tp;
	return 0;
}

/**********************************************************************/
void
unlinkuuid(struct uuidset_s *set, uint8_t *uuid)
{
	struct uuidtrk_s **entry;
	struct uuidtrk_s *tp;
	
	entry = set->table + uuidhash(uup->uuid, set->mask);
	while ((tp = *entry) != NULL) {
		if (tp->uuid == uuid) {
			*entry = tp->rlnk;
			free(tp);
			return 0;
		}
		entry = &tp->rlnk;
	}
	return -1;
}

/**********************************************************************/
#endif  /* ACNCFG_UUIDS_HASH */
/**********************************************************************/
