/*
#tabs=3t

Hash table handling copied and adapted from expat XML parser

Copyright (c) 1998, 1999, 2000 Thai Open Source Software Center Ltd
and Clark Cooper
Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006 Expat maintainers.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the 
"Software"), to deal in the Software without restriction, including 
without limitation the rights to use, copy, modify, merge, publish, 
distribute, sublicense, and/or sell copies of the Software, and to 
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be 
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS 
BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.
*/
#include "acn.h"

/*
Basic character hash algorithm, taken from Python's string hash:
h = h * 1000003 ^ character, the constant being a prime number.
*/
#define CHAR_HASH(h, c) \
	(((h) * 0xF4243) ^ (unsigned char)(c))

/*
For probing (after a collision) we need a step size relative prime 
to the hash table size, which is a power of 2. We use double-hashing,
since we can calculate a second hash value cheaply by taking those 
bits of the first hash value that were discarded (masked out) when 
the table index was calculated: index = hash & mask, where mask = 
table->size - 1. We limit the maximum step size to table->size / 4 
(mask >> 2) and make it odd, since odd numbers are always relative 
prime to a power of 2.
*/
#define SECOND_HASH(hash, mask, power) \
	((((hash) & ~(mask)) >> ((power) - 1)) & ((mask) >> 2))
#define PROBE_STEP(hash, mask, power) \
	((unsigned char)((SECOND_HASH(hash, mask, power)) | 1))

#define INITPOWER 6

static unsigned long hash_secret_salt;

/**********************************************************************/
static void
keys_init(void)
{
	static uint8_t initialized = false;

	if (!initialized) {
		randomize(false);
		hash_secret_salt = (unsigned long)acnrand();
		initialized = true;
	}
}

/**********************************************************************/
static bool
keyeq(const ddlchar_t * s1, const ddlchar_t * s2)
{
	for (; *s1 == *s2; s1++, s2++)
		if (*s1 == 0)
			return true;
	return false;
}

/**********************************************************************/
static unsigned long
hash(const ddlchar_t *s)
{
	unsigned long h = hash_secret_salt;
	while (*s)
		h = CHAR_HASH(h, *s++);
	return h;
}

/**********************************************************************/
const ddlchar_t **
findkey(struct hashtab_s *table, const ddlchar_t *name)
{
	size_t i;
	unsigned long mask;
	unsigned char step;
	unsigned long h;

	if (table->v == NULL) return NULL;

	mask = (unsigned long)(1 << table->power) - 1;
	h = hash(name);
	i = h & mask;

	if (table->v[i] && !keyeq(name, *table->v[i])) {
		step = PROBE_STEP(h, mask, table->power);
		do {
			i = (i - step) & mask;
		} while (table->v[i] && !keyeq(name, *table->v[i]));
	}
	return table->v[i];
}

/**********************************************************************/
const ddlchar_t **
replacekey(struct hashtab_s *table, const ddlchar_t **entry)
{
	size_t i;
	unsigned long mask;
	unsigned char step;
	unsigned long h;
	const ddlchar_t **oldentry;

	if (table->v == NULL) return NULL;

	mask = (unsigned long)(1 << table->power) - 1;
	h = hash(*entry);
	i = h & mask;

	if (table->v[i] && !keyeq(*entry, *table->v[i])) {
		step = PROBE_STEP(h, mask, table->power);
		do {
			i = (i - step) & mask;
		} while (table->v[i] && !keyeq(*entry, *table->v[i]));
	}
	if (table->v[i] == NULL) return NULL;
	oldentry = table->v[i];
	table->v[i] = entry;
	return oldentry;
}

/**********************************************************************/
static int
growtable(struct hashtab_s *table)
{
	size_t i;
	unsigned char newPower;
	size_t tsize;
	const ddlchar_t ***newV;
	unsigned long mask;
	unsigned long h;
	unsigned char step;

	newPower = table->power + 1;
	tsize = (size_t)1 << newPower;
	mask = (unsigned long)tsize - 1;
	tsize *= sizeof(const ddlchar_t **);
	if (!(newV = acnalloc(tsize))) return -1;
	memset(newV, 0, tsize);

	for (i = 0; i < (1 << table->power); i++) {
		if (table->v[i]) {
			size_t j;
			h = hash(*(table->v[i]));
			j = h & mask;

			if (newV[j]) {
				step = PROBE_STEP(h, mask, newPower);
				do {
					j = (j - step) & mask;
				} while (newV[j]);
			}
			newV[j] = table->v[i];
		}
	}
	acnfree(table->v);
	table->v = newV;
	table->power = newPower;
	return 0;
}
/**********************************************************************/
const ddlchar_t **
findornewkey(
	struct hashtab_s *table,
	const ddlchar_t *name, 
	struct pool_s *pool,
	size_t createsz
)
{
	size_t i;
	unsigned long mask;
	unsigned char step;
	unsigned long h;
	const ddlchar_t **entry;

	if (table->v) {
		mask = (1UL << table->power) - 1;
		h = hash(name);
		i = h & mask;

		if (table->v[i]) {
			if (keyeq(name, *table->v[i])) return table->v[i];
			step = PROBE_STEP(h, mask, table->power);
			while (table->v[i = (i - step) & mask]) {
				if (keyeq(name, *table->v[i])) return table->v[i];
			}
		}
	} else {
		size_t tsize;

		keys_init();
		table->power = INITPOWER;
		table->used = 0;
		/* tsize is a power of 2 */
		tsize = (size_t)1 << INITPOWER;
		mask = (unsigned long)tsize - 1;
		tsize *= sizeof(const ddlchar_t **);
		table->v = (const ddlchar_t ***)acnalloc(tsize);
		if (!table->v) return NULL;
		memset(table->v, 0, tsize);
		i = hash(name) & mask;
	}
	/*
	Entry is not already there and we've found position
	*/
	if ((entry = acnalloc(createsz)) == NULL) return NULL;
	memset(entry, 0, createsz);
	if ((*entry = pool_addstr(pool, name)) == NULL) {
		acnfree(entry /* , createsz */);
		return NULL;
	}
	
	table->v[i] = entry;
	table->used++;

	/*
	Check for overflow (more than half full)?
	*/
	if (table->used >> (table->power - 1) && growtable(table) < 0) {
		table->v[i] = NULL;
		table->used--;
		acnfree(entry /* , createsz */);
		return NULL;
	}
	return entry;
}

/**********************************************************************/
int
addkey(struct hashtab_s *table, const ddlchar_t **entry)
{
	size_t i;
	unsigned long mask;
	unsigned char step;
	unsigned long h;
	const ddlchar_t *name = *entry;

	if (!table->v) {
		size_t tsize;

		keys_init();
		table->power = INITPOWER;
		table->used = 0;
		/* tsize is a power of 2 */
		tsize = (size_t)1 << INITPOWER;
		mask = (unsigned long)tsize - 1;
		tsize *= sizeof(const ddlchar_t **);
		table->v = (const ddlchar_t ***)acnalloc(tsize);
		if (!table->v) return KEY_NOMEM;
		memset(table->v, 0, tsize);
		i = hash(name) & mask;
	} else {
		mask = (1UL << table->power) - 1;
		h = hash(name);
		i = h & mask;

		if (table->v[i]) {
			if (keyeq(name, *table->v[i])) return KEY_ALREADY;
			step = PROBE_STEP(h, mask, table->power);
			while (table->v[i = (i - step) & mask]) {
				if (keyeq(name, *table->v[i])) return KEY_ALREADY;
			}
		}
	}
	/*
	Entry is not already there and we've found position
	*/
	table->v[i] = entry;
	table->used++;

	/*
	Check for overflow (more than half full)?
	*/
	if (table->used >> (table->power - 1) && growtable(table) < 0) {
		table->v[i] = NULL;
		table->used--;
		acnfree(entry /* , createsz */);
		return KEY_NOMEM;
	}
	return 0;
}
/**********************************************************************/
struct pblock_s {
	struct pblock_s *nxt;
	size_t size;
	ddlchar_t s[];
};

#define BLK_CHARS 1024
#define BLK_SIZE(chars) (offsetof(struct pblock_s, s) \
	+ (chars) * sizeof(ddlchar_t))

/**********************************************************************/
static bool
pool_grow(struct pool_s *pool)
{
	struct pblock_s *blk;
	size_t bsize;
	size_t cpsize;

	if (pool->blocks == NULL) {
		bsize = BLK_CHARS;
		cpsize = 0;
	} else {
		bsize = pool->endp - pool->nxtp;
		bsize += BLK_CHARS - (bsize % BLK_CHARS);
		cpsize = pool->ptr - pool->nxtp;
	}
	if ((blk = acnalloc(BLK_SIZE(bsize))) == NULL) return false;
	blk->size = bsize;
	if (cpsize) {
		memcpy(blk->s, pool->nxtp, cpsize);
	}
	if (pool->blocks && pool->nxtp == pool->blocks->s) {
		/* block is unused but too small - free it */
		struct pblock_s *tmp;
		tmp = pool->blocks;
		pool->blocks = tmp->nxt;
		acnfree(tmp);
	}
	pool->nxtp = blk->s;
	pool->endp = blk->s + bsize;
	pool->ptr = blk->s + cpsize;
	blk->nxt = pool->blocks;
	pool->blocks = blk;
	return true;
}

/**********************************************************************/
static inline bool
pool_addch(struct pool_s *pool, ddlchar_t c)
{
	if (pool->ptr == pool->endp && !pool_grow(pool))
		return false;
	*pool->ptr++ = c;
	return true;
}

/**********************************************************************/
void
pool_init(struct pool_s *pool)
{
	memset(pool, 0, sizeof(struct pool_s));
}

/**********************************************************************/
void
pool_reset(struct pool_s *pool)
{
	struct pblock_s *blk;
	struct pblock_s *nblk;

	for (blk = pool->blocks; blk != NULL; blk = nblk) {
		nblk = blk->nxt;
		acnfree(blk /* , BLK_SIZE(blk->size) */);
	}
	pool_init(pool);
}

/**********************************************************************/
/*
add a string to pool leaving ready to extend
*/
const ddlchar_t *
pool_appendstr(struct pool_s *pool, const ddlchar_t *s)
{
	while (*s) {
		if (!pool_addch(pool, *s)) return NULL;
		s++;
	}
	return pool->nxtp;
}

/**********************************************************************/
/*
add a string to pool leaving ready to extend
*/
const ddlchar_t *
pool_appendn(struct pool_s *pool, const ddlchar_t *s, int n)
{
	if (!pool->blocks && !pool_grow(pool)) return NULL;

	while (n--) {
		if (!pool_addch(pool, *s++)) return NULL;
	}
	return pool->nxtp;
}

/**********************************************************************/
/*
Dump a partially accumulated string
*/
void
pool_dumpstr(struct pool_s *pool)
{
	pool->ptr = pool->nxtp;
}

/**********************************************************************/
/*
add a string to pool and terminate
*/
const ddlchar_t *
pool_addstr(struct pool_s *pool, const ddlchar_t *s)
{
	do {
		if (!pool_addch(pool, *s))
			return NULL;
	} while (*s++);
	s = pool->nxtp;
	pool->nxtp = pool->ptr;
	return s;
}
/**********************************************************************/
static inline bool
c_isspace(int c)
{
	switch (c) {
	case ' ':
	case '\r':
	case '\n':
	case '\t':
		return true;
	default:
		return false;
	}
}
/**********************************************************************/
/*
add a space folded string to pool and terminate
*/
enum spstate_e {
	sp_none = 0,
	sp_start,
	sp_mid,
};


const ddlchar_t *
pool_addfoldsp(struct pool_s *pool, const ddlchar_t *s)
{
	enum spstate_e spstate = sp_start;
	ddlchar_t c;

	do {
		c = *s++;

		if (c == 0) {
			if (spstate == sp_mid) --pool->ptr;
		} else if (!c_isspace(c)) {
			spstate = sp_none;
		} else if (spstate == sp_none) {
			c = ' ';
			spstate = sp_mid;
		} else {
			continue;
		}
		if (!pool_addch(pool, c))
			return NULL;
	} while (c);
	s = pool->nxtp;
	pool->nxtp = pool->ptr;
	return s;
}
/**********************************************************************/
/*
Terminate a string accumulated in pool
*/
const ddlchar_t *
pool_termstr(struct pool_s *pool)
{
	return pool_addstr(pool, "");
}

/**********************************************************************/
/*
Add a string of length n
*/
const ddlchar_t *
pool_addstrn(struct pool_s *pool, const ddlchar_t *s, int n)
{
	if ((s = pool_appendn(pool, s, n))) {
		s = pool_addstr(pool, "");
	}
	return s;
}
