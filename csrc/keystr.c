
typedef struct keyhd_s keyhd_t;
/* used to test bits in the radix tree may vary with optimization 
for different architectures */
typedef unsigned int strbtst_t;

struct keyhd_s {
	const char *key;
	strbtst_t tstloc;
	struct keyhd_t *nxt[2];
};


**********************************************************************/
/*
test for exact match - don't use keysEq() because if we fail we 
want to record the point of difference.

Handle this using macros to allow different strbtst_t implementations
*/
#define MATCHVAL 0
#define match_eq(tstloc) ((tstloc) == MATCHVAL)

static inline strbtst_t
matchkey(const char *str1, const char *str2, int minlen)
{
	int tstloc;
	int i;
	char b;
	uint8_t m;

	tstloc = 0xff;
	for (i = 0; (b = str1[i] ^ str2[i]) == 0; ++i) {
		if (i >= minlen && str1[i] == 0) return MATCHVAL;
	}
	while ((b = *cp1 ^ *cp2) == 0) {
		if (*cp1 == 0) return MATCHVAL;
		++cp1;
		++cp2;
	}
	tstloc += (i << 8);

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
#if !CONFIG_KEYTRACK_INLINE
#define TERMVAL 0x0fff
#define isterm(tstloc) ((tstloc) >= TERMVAL)

static inline int
testkbit(const char *str, int len, strbtst_t tstloc)
{
	if ((tstloc >> 8) >= len) return 0;
	return (((unsigned)(str[tstloc >> 8] | (uint8_t)tstloc)) + 1) >> 8;
}
#endif

/**********************************************************************/
#if !CONFIG_KEYTRACK_INLINE
static keyhd_t *
_findkey(keyhd_t **set, const char *str, int len)
{
	unsigned int tstloc;
	keyhd_t *tp;

	if ((tp = *set) != NULL) {
		do {
			tstloc = tp->tstloc;
			tp = tp->nxt[testkbit(str, len, tstloc)];
		} while (tp->tstloc > tstloc);
	}
	return tp;
}
#endif

/**********************************************************************/
#if !CONFIG_KEYTRACK_INLINE
keyhd_t *
findkey(keyhd_t **set, const char *key)
{
	keyhd_t *tp;

	tp = _findkey(set, key, strlen(key));
	if (tp && !strcmp(tp->key, key)) tp = NULL;
	return tp;
}
#endif
/**********************************************************************/
int
findornewkey(keyhd_t **set, const char *key, keyhd_t **rslt, size_t size)
{
	keytst_t tstloc;
	keyhd_t *tp;
	keyhd_t *np;
	keyhd_t **pp;
	int len;

	assert(size >= sizeof(keyhd_t));
	/* find our point in the tree */
	len = strlen(key);
	tp = _findkey(set, key, len);
	if (tp == NULL) {
		tstloc = TERMVAL;
	} else {
		/* find lowest bit difference */
		tstloc = matchkey(key, tp->key);
		if (match_eq(tstloc)) {	/* got a match */
			*rslt = tp;
			return 0;
		}
	}
	/* allocate a new record */
	{
		uint8_t *mp;
	
		if (!(mp = _acnAlloc(size))) return -1;
		memset(mp + sizeof(keyhd_t), 0, size - sizeof(keyhd_t));
		np = (keyhd_t *)mp;
	}
	np->key = key;

	strcpy(np->key, key);
	np->tstloc = tstloc;
	np->nxt[0] = np->nxt[1] = np;

	/* now work out where to put it */
	/* find insert location */
	pp = &set->first;
	tp = set->first;

	if (tp) {
		while (1) {
			keytst_t tl;
	
			tl = tp->tstloc;
			if (tl >= tstloc) break;	/* internal link */
			pp = &tp->nxt[testkbit(key, tl)];
			tp = *pp;
			if (tp->tstloc <= tl) break;	/* external link */
		}
		np->nxt[testkbit(key, tstloc) ^ 1] = tp;
	}

	*rslt = *pp = np;
	return 1;
}

/**********************************************************************/
int
addkey(keyhd_t **set, keyhd_t *np)
{
	keytst_t tstloc;
	keyhd_t *tp;
	keyhd_t **pp;

	/* find our point in the tree */
	tp = _findkey(set, np->key);
	if (tp == NULL) {
		tstloc = TERMVAL;
	} else {
		/* find lowest bit difference (or match) */
		tstloc = matchkey(np->key, tp->key);
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
			keytst_t tl;
	
			tl = tp->tstloc;
			if (tl >= tstloc) break;	/* internal link */
			pp = &tp->nxt[testkbit(np->key, tl)];
			tp = *pp;
			if (tp->tstloc <= tl) break;	/* external link */
		}
		np->nxt[testkbit(np->key, tstloc) ^ 1] = tp;
	}
	*pp = np;
	return 0;
}

/**********************************************************************/
int
unlinkkey(keyhd_t **set, keyhd_t *uup)
{
	keyhd_t *pext;	/* external parent node (may be self) */
	keyhd_t *gpext; /* external grandparent node (may be set) */
	keyhd_t **pint; /* internal parent link */
	keyhd_t *tp;
	int bit;

	gpext = NULL;
	pint = &set->first;
	tp = set->first;
	while (1) {
		pext = tp;
		bit = testkbit(uup->key, tp->tstloc);
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
		gpext->nxt[testkbit(uup->key, gpext->tstloc)] = pext->nxt[bit ^ 1];
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
delkey(keyhd_t **set, keyhd_t *uup, size_t size)
{
	if (unlinkkey(set, uup) < 0) return -1;
	_acnFree(uup, size);
	return 0;
}
#endif


