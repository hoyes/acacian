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
#tabs=3
*/
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>
#include "acn.h"
#include "tohex.h"

const uuid_t  null_uuid = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/**********************************************************************/

int
str2uuid(const char *uuidstr, uuid_t uuid)
{
	const char *cp;
	uint8_t *uup;
	int byte;

	byte = 1;  /* bit 0 is shift marker */

	cp = uuidstr;
	for (uup = uuid; uup < uuid + UUID_SIZE; ++cp) {
		switch (*cp) {
		case '-':	/* ignore dashes */
			continue;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			byte = (byte << 4) | (*cp - '0');
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			byte = (byte << 4) | ((*cp | ('a' - 'A')) - 'a' + 10);
			break;
		default:
			return -1;
		}
		if (byte >= 0x100) {
			*uup++ = (uint8_t)byte;
			byte = 1;  /* restore shift marker */
		}
	}
	return 0;
}

/**********************************************************************/
/*
generate a UUID string
Return a pointer to start of string
String is fixed length CID_STR_SIZE (including Nul terminator)
e.g.
a834b30c-6298-46b3-ac59-c5a0286bb599
766c744e3d-3b11-e097-4700-17316c497d
*/
char *
uuid2str(const uuid_t uuid, char *uuidstr)
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
uuidIsNull(const uuid_t uuid)
{
	int count = UUID_SIZE;

	while (*uuid++ == 0) if (--count == 0) return true;
	return false;
}
#endif
/**********************************************************************/

#if !defined(uuidsEq)
bool
uuidsEq(const uuid_t uuid1, const uuid_t uuid2);
{
	int count = UUID_SIZE;

	while (*uuid1++ == *uuid2++) if (--count == 0) return true;
	return false;
}
#endif
/**********************************************************************/

#if !defined(uuidcpy)
uint8_t *
uuidcpy(uuid_t dst, const uuid_t src)
{
	uint8_t dp = dst;
	int count = UUID_SIZE;

	while (count--) *dp++ = *src++;
	return dst;
}
#endif
/**********************************************************************/
/*
UUID tracking

*/

#if CONFIG_UUIDTRACK == UUIDS_RADIX

/*
Radix search using Patricia tree

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

  bool testbit(uuid_t uuid, tstloc_t tstloc)

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
test for exact match - don't use uuidsEq() because if we fail we 
want to record the point of difference.

Handle this using macros to allow different uuidtst_t implementations
*/
#define MATCHVAL 0
#define match_eq(tstloc) ((tstloc) == MATCHVAL)

static inline uuidtst_t
matchuuid(const uuid_t uuid1, const uuid_t uuid2)
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
#if !CONFIG_UUIDTRACK_INLINE
#define TERMVAL 0x0fff
#define isterm(tstloc) ((tstloc) >= TERMVAL)

static inline int
testbit(uuid_t const uuid, uuidtst_t tstloc)
{
	return (((unsigned)(uuid[tstloc >> 8] | (uint8_t)tstloc)) + 1) >> 8;
}
#endif

/**********************************************************************/
#if !CONFIG_UUIDTRACK_INLINE
static struct uuidtrk_s *
_finduuid(uuidset_t *set, const uuid_t uuid)
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
#endif

/**********************************************************************/
#if !CONFIG_UUIDTRACK_INLINE
struct uuidtrk_s *
finduuid(uuidset_t *set, const uuid_t uuid)
{
	struct uuidtrk_s *tp;

	tp = _finduuid(set, uuid);
	if (tp && !uuidsEq(tp->uuid, uuid)) tp = NULL;
	return tp;
}
#endif
/**********************************************************************/

int
findornewuuid(uuidset_t *set, const uuid_t uuid, struct uuidtrk_s **rslt, size_t size)
{
	uuidtst_t tstloc;
	struct uuidtrk_s *tp;
	struct uuidtrk_s *np;
	struct uuidtrk_s **pp;

	assert(size >= sizeof(struct uuidtrk_s));
	/* find our point in the tree */
	tp = _finduuid(set, uuid);
	if (tp == NULL) {
		tstloc = TERMVAL;
	} else {
		/* find lowest bit difference */
		tstloc = matchuuid(uuid, tp->uuid);
		if (match_eq(tstloc)) {	/* got a match */
			*rslt = tp;
			return 0;
		}
	}
	/* allocate a new record */
	{
		uint8_t *mp;
	
		mp = mallocx(size);
		memset(mp + sizeof(struct uuidtrk_s), 0, size - sizeof(struct uuidtrk_s));
		np = (struct uuidtrk_s *)mp;
	}
	uuidcpy(np->uuid, uuid);
	np->tstloc = tstloc;
	np->nxt[0] = np->nxt[1] = np;

	/* now work out where to put it */
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

	*rslt = *pp = np;
	return 1;
}

/**********************************************************************/
int
adduuid(uuidset_t *set, struct uuidtrk_s *np)
{
	uuidtst_t tstloc;
	struct uuidtrk_s *tp;
	struct uuidtrk_s **pp;

	/* find our point in the tree */
	tp = _finduuid(set, np->uuid);
	if (tp == NULL) {
		tstloc = TERMVAL;
	} else {
		/* find lowest bit difference (or match) */
		tstloc = matchuuid(np->uuid, tp->uuid);
		if (match_eq(tstloc)) return -1;	/* already there */
	}
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
			pp = &tp->nxt[testbit(np->uuid, tl)];
			tp = *pp;
			if (tp->tstloc <= tl) break;	/* external link */
		}
		np->nxt[testbit(np->uuid, tstloc) ^ 1] = tp;
	}
	*pp = np;
	return 0;
}

/**********************************************************************/
int
unlinkuuid(uuidset_t *set, struct uuidtrk_s *uup)
{
	struct uuidtrk_s *pext;	/* external parent node (may be self) */
	struct uuidtrk_s *gpext; /* external grandparent node (may be set) */
	struct uuidtrk_s **pint; /* internal parent link */
	struct uuidtrk_s *tp;
	int bit;

	gpext = NULL;
	pint = &set->first;
	tp = set->first;
	while (1) {
		pext = tp;
		bit = testbit(uup->uuid, tp->tstloc);
		tp = tp->nxt[bit];
		if (tp->tstloc <= pext->tstloc) {
			if (tp != uup) return -1;	/* our node is not there */
			break;
		}
		if (tp == uup) pint = &(pext->nxt[bit]); /* save internal parent */
		if (isterm(tp->tstloc)) break;
		gpext = pext;
	}
	/* move node pext to position of node to be deleted - may be null op */
	*pint = pext;
	/* re-link any children of pext */
	if (gpext)
		gpext->nxt[testbit(uup->uuid, gpext->tstloc)] = pext->nxt[bit ^ 1];
	else {
		if (isterm(pext->tstloc)) {
			set->first = NULL;
			return 0;
		}
		set->first = pext->nxt[bit ^ 1];
	}
	/* replace tp with pext - may be null op */
	pext->tstloc = tp->tstloc;
	if (!isterm(pext->tstloc)) {
		pext->nxt[0] = tp->nxt[0];
		pext->nxt[1] = tp->nxt[1];
	} else {
		pext->nxt[0] = pext->nxt[1] = pext;
	}
	return 0;
}

/**********************************************************************/
#if !CONFIG_UUIDTRACK_INLINE
int
deluuid(uuidset_t *set, struct uuidtrk_s *uup, size_t size)
{
	if (unlinkuuid(set, uup) < 0) return -1;
	free(uup);
	return 0;
}
#endif

/**********************************************************************/
void
_foreachuuid(struct uuidtrk_s **pp, uuiditerfn *fn)
{
	struct uuidtrk_s *tp;

	do {
		if ((tp = *pp) == NULL) return;
		(*fn)(tp);
	} while (*pp != tp);

	if (tp->nxt[0]->tstloc > tp->tstloc)
		_foreachuuid(&tp->nxt[0], fn);
	if (tp->nxt[1]->tstloc > tp->tstloc)
		_foreachuuid(&tp->nxt[1], fn);
}

#elif CONFIG_UUIDTRACK == UUIDS_HASH
/**********************************************************************/
/*
Need to provide
	struct uuidtrk_s *finduuid(uuidset_t *set, uuid_t uuid)
	int adduuid(uuidset_t *set, struct uuidtrk_s *uup)
	int deluuid(uuidset_t *set, struct uuidtrk_s *uup)
	int findornewuuid(uuidset_t *set, uuid_t uuid, struct uuidtrk_s **rslt, size_t size)
*/
/**********************************************************************/
/*
These may be inlined
*/
#if !CONFIG_UUIDTRACK_INLINE

/**********************************************************************/
struct uuidtrk_s *
finduuid(uuidset_t *set, const uuid_t uuid)
{
	struct uuidtrk_s *cp;

	cp = set->table[uuidhash(uuid, set->mask)];
	while (cp && !uuidsEq(cp->uuid, uuid)) cp = cp->rlnk;
	return cp;
}
#endif
/**********************************************************************/
int
findornewuuid(uuidset_t *set, const uuid_t uuid, struct uuidtrk_s **rslt, size_t size)
{
	struct uuidtrk_s **pp;
	struct uuidtrk_s *tp;

	pp = set->table + uuidhash(uuid, set->mask);
	while ((tp = *pp) != NULL) {
		if (uuidsEq(tp->uuid, uuid)) {
			*rslt = tp;
			return 0;
		}
		pp = &tp->rlnk;
	}
	/* allocate a new record */
	{
		uint8_t *mp;
	
		mp = mallocx(size);
		memset(mp + sizeof(struct uuidtrk_s), 0, size - sizeof(struct uuidtrk_s));
		tp = (struct uuidtrk_s *)mp;
		uuidcpy(tp->uuid, uuid);
	}
	tp->rlnk = NULL;
	*rslt = *pp = tp;
	return 1;
}

/**********************************************************************/
#if !CONFIG_UUIDTRACK_INLINE
int
adduuid(uuidset_t *set, struct uuidtrk_s *uup)
{
	struct uuidtrk_s **entry;

	entry = set->table + uuidhash(uup->uuid, set->mask);
	uup->rlnk = *entry;
	*entry = uup;
	return 0;
}
#endif

/**********************************************************************/
#if !CONFIG_UUIDTRACK_INLINE
int
unlinkuuid(uuidset_t *set, struct uuidtrk_s *uup)
{
	struct uuidtrk_s **entry;
	
	entry = set->table + uuidhash(uup->uuid, set->mask);
	if (uup == *entry) *entry = uup->rlnk;
	else {
		struct uuidtrk_s *xp;

		for (xp = *entry; ; xp = xp->rlnk) {
			if (xp == NULL) return -1;
			if (xp->rlnk == uup) {
				xp->rlnk = uup->rlnk;
				break;
			}
		}
	}
	return 0;
}
#endif

/**********************************************************************/
#if !CONFIG_UUIDTRACK_INLINE
int
deluuid(uuidset_t *set, struct uuidtrk_s *uup, size_t size)
{
	if (unlinkuuid(set, uup) < 0) return -1;
	free(uup);
	return 0;
}
#endif

#endif  /* CONFIG_UUIDTRACK == UUIDS_HASH */
