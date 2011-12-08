/**********************************************************************/
/*

	Copyright (C) 2011, Engineering Arts. All rights reserved.

	Author: Philip Nye

	$Id$

#tabs=3
*/
/**********************************************************************/
/*
Patricia tree implementation for tracking behavior and string keys.

Although the UUID (in binary format) and name/key are stored 
separately, we can maintain a common tree for both with the UUID 
taking greatest significance.
*/
/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL

#include <assert.h>
#include "acncommon.h"
#include "acnlog.h"
#include "acnmem.h"
#include "propmap.h"
#include "uuid.h"
#include "ddl/parse.h"
#include "ddl/keys.h"

/**********************************************************************/
/*
test for exact match - don't use keysEq() because if we fail we 
want to record the point of difference.

Handle this using macros to allow different keytst_t implementations
*/
#define MATCHVAL 0
#define match_eq(tstloc) ((tstloc) == MATCHVAL)

static inline keytst_t
matchkey(uuid_t uuid, const ddlchar_t *name, ddlkey_t *k2)
{
	keytst_t tstloc;
	int i;
	ddlchar_t b;
	uint8_t m;

	for (i = 0; i < 16; ++i) {
		if ((b = uuid[i] ^ k2->uuid[i]) != 0) goto matchfail;
	}

	for (i = 0; (b = name[i] ^ k2->name[i]) == 0; ++i) {
		if (name[i] == 0) return MATCHVAL;
	}

matchfail:
	/* upper part of tstloc is the index, low byte all 1s */
	tstloc = (i << 8) + 0xff;
	/* lowest is the highest order bit set in b */
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
#define TERMVAL 0xffff
#define isterm(tstloc) ((tstloc) >= TERMVAL)

static inline int
testkbit(uuid_t uuid, const ddlchar_t *name, unsigned int namelen, keytst_t tstloc)
{
	unsigned int offs;

	if (tstloc < (UUID_SIZE << 8))
		return ((uuid[tstloc >> 8] | (uint8_t)tstloc) + 1) >> 8;
	
	offs = (tstloc >> 8) - UUID_SIZE;
	if (offs >= namelen) return 0;
	
	return (((unsigned)(name[offs] | (uint8_t)tstloc)) + 1) >> 8;
}

/**********************************************************************/
static inline int
testkt(ddlkey_t *kp, keytst_t tstloc)
{
	unsigned int offs;

	if (tstloc < (UUID_SIZE << 8))
		return ((kp->uuid[tstloc >> 8] | (uint8_t)tstloc) + 1) >> 8;
	
	offs = (tstloc >> 8) - UUID_SIZE;
	if (offs >= kp->namelen) return 0;
	
	return (((unsigned)(kp->name[offs] | (uint8_t)tstloc)) + 1) >> 8;
}

/**********************************************************************/
static ddlkey_t *
_findkey(ddlkey_t **set, uuid_t uuid, const ddlchar_t *name, unsigned int namelen)
{
	keytst_t tstloc;
	ddlkey_t *tp;

	if ((tp = *set) != NULL) {
		do {
			tstloc = tp->tstloc;
			tp = tp->nxt[testkbit(uuid, name, namelen, tstloc)];
		} while (tp->tstloc > tstloc);
	}
	return tp;
}

/**********************************************************************/
ddlkey_t *
findkey(ddlkey_t **set, const uuid_t uuid, const ddlchar_t *name)
{
	ddlkey_t *tp;

	tp = _findkey(set, uuid, name, strlen(name));
	if (tp && uuidsEq(uuid, tp->uuid) && strcmp(tp->name, name) == 0) return tp;
	return NULL;
}

/**********************************************************************/
int
findornewkey(ddlkey_t **set, uuid_t uuid, const ddlchar_t *name, ddlkey_t **rslt, size_t size)
{
	keytst_t tstloc;
	ddlkey_t *tp;
	ddlkey_t *np;
	ddlkey_t **pp;
	int len;

	assert(size >= sizeof(ddlkey_t));
	/* find our point in the tree */
	len = strlen(name);
	tp = _findkey(set, uuid, name, len);
	if (tp == NULL) {
		tstloc = TERMVAL;
	} else {
		/* find lowest bit difference */
		tstloc = matchkey(uuid, name, tp);
		if (match_eq(tstloc)) {	/* got a match */
			*rslt = tp;
			return 0;
		}
	}
	/* allocate a new record */
	{
		uint8_t *mp;
	
		mp = mallocx(size);
		memset(mp + sizeof(ddlkey_t), 0, size - sizeof(ddlkey_t));
		np = (ddlkey_t *)mp;
	}
	uuidcpy(np->uuid, uuid);
	np->namelen = savestr(name, &np->name);
	np->tstloc = tstloc;
	np->nxt[0] = np->nxt[1] = np;

	/* now work out where to put it */
	/* find insert location */
	pp = set;
	tp = *set;

	if (tp) {
		while (1) {
			keytst_t tl;
	
			tl = tp->tstloc;
			if (tl >= tstloc) break;	/* internal link */
			pp = &tp->nxt[testkt(np, tl)];
			tp = *pp;
			if (tp->tstloc <= tl) break;	/* external link */
		}
		np->nxt[testkt(np, tstloc) ^ 1] = tp;
	}

	*rslt = *pp = np;
	return 1;
}

/**********************************************************************/
int
addkey(ddlkey_t **set, ddlkey_t *np)
{
	keytst_t tstloc;
	ddlkey_t *tp;
	ddlkey_t **pp;

	/* find our point in the tree */
	tp = _findkey(set, np->uuid, np->name, strlen(np->name));
	if (tp == NULL) {
		tstloc = TERMVAL;
	} else {
		/* find lowest bit difference (or match) */
		tstloc = matchkey(np->uuid, np->name, tp);
		if (match_eq(tstloc)) return -1;	/* already there */
	}
	/* now work out where to put it */
	np->tstloc = tstloc;
	np->nxt[0] = np->nxt[1] = np;
	/* find insert location */
	pp = set;
	tp = *set;

	if (tp) {
		while (1) {
			keytst_t tl;
	
			tl = tp->tstloc;
			if (tl >= tstloc) break;	/* internal link */
			pp = &tp->nxt[testkt(np, tl)];
			tp = *pp;
			if (tp->tstloc <= tl) break;	/* external link */
		}
		np->nxt[testkt(np, tstloc) ^ 1] = tp;
	}
	*pp = np;
	return 0;
}

/**********************************************************************/
int
unlinkkey(ddlkey_t **set, ddlkey_t *uup)
{
	ddlkey_t *pext;	/* external parent node (may be self) */
	ddlkey_t *gpext; /* external grandparent node (may be set) */
	ddlkey_t **pint; /* internal parent link */
	ddlkey_t *tp;
	int bit;

	gpext = NULL;
	pint = set;
	tp = *set;
	while (1) {
		pext = tp;
		bit = testkt(uup, tp->tstloc);
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
		gpext->nxt[testkt(uup, gpext->tstloc)] = pext->nxt[bit ^ 1];
	else {
		if (isterm(pext->tstloc)) {
			*set = NULL;
			return 0;
		}
		*set = pext->nxt[bit ^ 1];
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
int
delkey(ddlkey_t **set, ddlkey_t *uup, size_t size)
{
	if (unlinkkey(set, uup) < 0) return -1;
	free(uup);
	return 0;
}
