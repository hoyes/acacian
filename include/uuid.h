/************************************************************************/
/*
Copyright (c) 2011, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3
*/
/************************************************************************/
/*
Universally Unique Identifiers (UUIDs) are used in ACN for several 
purposes including Component IDs (CIDs) and DDL module identifiers 
(DCIDs).

See RFC 4122 for definition
*/

#ifndef __uuid_h__
#define __uuid_h__ 1

#include <string.h>
/*
UUID format
xxxxxxxx-xxxx-Vxxx-Txxx-xxxxxxxxxxxx

T (variant or type) must be binary 10xxxxxx (0x80 .. 0xbf)
V (version) is hex:
  1x  time and MAC based UUID
  2x  DCE Security version, with embedded POSIX UIDs
  3x  name-based version using MD5 hash
  4x  random version
  5x  name-based version using SHA-1 hash

e.g.
a834b30c-6298-46b3-ac59-c5a0286bb599
*/

/*
Since we cannot guarantee alignment (UUIDs are often within packets) 
and to avoid endian issues, always treat UUIDs as a simple array of 
16 octets.

In text form they must always be expressed in the 36-character format
shown above.
*/
#define UUID_SIZE 16
#define UUID_STR_SIZE 37  /* including null termination */
#define PRIUUIDstr "36"	/* printing width for string format */

/* generic uuid as array */
typedef uint8_t uuid_t[UUID_SIZE];
extern const uuid_t null_uuid;

/*
  Macros to access the internal structure Fields are in Network byte 
  order
*/
#define UUID_TIME_LOW(uuid) unmarshalU32(uuid)
#define UUID_TIME_MID(uuid) unmarshalU16((uuid) + 4))
#define UUID_TIME_HIV(uuid) unmarshalU16((uuid) + 6))
#define UUID_CLKSEQ_HI(uuid) ((uuid)[8])
#define UUID_CLKSEQ_LOW(uuid) ((uuid)[9])
#define UUID_NODE(uuid) ((uuid) + 10)

int str2uuid(const char *uuidstr, uuid_t uuidp);
char *uuid2str(const uuid_t uup, char *uuidstr);

/* undefine these macros to use external functions */
#define uuidsEq(uup1, uup2) (memcmp(uup1, uup2, UUID_SIZE) == 0)
#define uuidIsNull(uuid) uuidIsNull(uuid)
#define uuidcpy(dst, src) (memcpy(dst, src, UUID_SIZE))

#if defined(uuidIsNull)
static inline bool uuidIsNull(const uuid_t uuid)
{
	int count = UUID_SIZE;

	while (*uuid++ == 0) if (--count == 0) return true;
	return false;
}
#else
extern bool uuidIsNull(const uuid_t uuid)
#endif

#if !defined(uuidsEq)
extern bool uuidsEq(const uuid_t uuid1, const uuid_t uuid2);
#endif

#if !defined(uuidcpy)
extern uint8_t *uuidcpy(uuid_t uuid1, const uuid_t uuid2);
#endif

/*
Check a binary uuid or uuidstring for legal variant and version bits 
but nothing else.
*/

static inline bool quickuuidOKstr(const char *uuidstr)
{
	if (uuidstr == NULL) return false;
	switch (uuidstr[19]) {
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'a':
	case 'b':
		return (uuidstr[14] >= '1' && uuidstr[14] <= '5' && strlen(uuidstr) == (UUID_STR_SIZE - 1));
	default:
		return false;
	}
}

static inline bool quickuuidOK(uuid_t uuid)
{
	return ((uuid[8] & 0xc0) == 0x80 && uuid[6] >= 0x10 && uuid[6] <= 0x5f);
}

/*
Searching for UUIDs

There are several places where we need to store records indexed 
by UUID which need highly optimized lookup.

UUID records are contained in a uuidset, with operations to find, 
add and delete UUIDs from a set.

By providing a common header structure for these records we can use 
the same routines everywhere this is necessary.

Any method needs to provide these functions:

	struct uuidtrk_s *finduuid(struct uuidset_s *set, uuid_t uuid)

find the record matching uuid in the set given
returns pointer to the record header or NULL if not found

	int findornewuuid(struct uuidset_s *set, uuid_t uuid, struct uuidtrk_s **rslt, size_t size)

Find a record if it exists or create and add a new one if not - 
quicker than separate finduuid, malloc, adduuid. The resulting record
is stored in rslt. To allow for headers embedded in larger records a
size is also passed.

returns
  0 if an existing record found
  1 if a new record was created
  -1 on error (can't malloc)

	int adduuid(struct uuidset_s *set, struct uuidtrk_s *uup)
	
add a uuid record to the set
returns 0 for success, -1 if it was already there
	
	int deluuid(struct uuidset_s *set, struct uuidtrk_s *uup)

remove uuid record from the set
returns 0 for success, -1 if it wasn't found
*/

#if defined(ACNCFG_UUIDS_RADIX)
/* Our set is a simple pointer to the first item */
struct uuidset_s {
	struct uuidtrk_s *first;
};

/* used to test bits in the radix tree may vary with optimization 
for different architectures */
typedef unsigned int uuidtst_t;

struct uuidtrk_s {
	uuid_t uuid;
	uuidtst_t tstloc;
	struct uuidtrk_s *nxt[2];
};

#if !defined(ACNCFG_UUIDTRACK_INLINE)
extern struct uuidtrk_s *finduuid(struct uuidset_s *set, const uuid_t uuid);
extern int deluuid(struct uuidset_s *set, struct uuidtrk_s *uup, size_t size);
#endif
extern int adduuid(struct uuidset_s *set, struct uuidtrk_s *uup);
extern int unlinkuuid(struct uuidset_s *set, struct uuidtrk_s *uup);
extern int findornewuuid(struct uuidset_s *set, const uuid_t uuid, struct uuidtrk_s **rslt, size_t size);

#if defined(ACNCFG_UUIDTRACK_INLINE)
#define TERMVAL 0x0fff
#define isterm(tstloc) ((tstloc) >= TERMVAL)

static inline int
testbit(const uuid_t uuid, uuidtst_t tstloc)
{
	return (((unsigned)(uuid[tstloc >> 8] | (uint8_t)tstloc)) + 1) >> 8;
}

/**********************************************************************/
static inline struct uuidtrk_s *
_finduuid(struct uuidset_s *set, const uuid_t uuid)
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
static inline struct uuidtrk_s *
finduuid(struct uuidset_s *set, const uuid_t uuid)
{
	struct uuidtrk_s *tp;

	tp = _finduuid(set, uuid);
	if (tp && !uuidsEq(tp->uuid, uuid)) tp = NULL;
	return tp;
}

/**********************************************************************/
static inline int
deluuid(struct uuidset_s *set, struct uuidtrk_s *uup, size_t size)
{
	if (unlinkuuid(set, uup) < 0) return -1;
	free(uup);
	return 0;
}
#endif  /* defined(ACNCFG_UUIDTRACK_INLINE) */

/**********************************************************************/
typedef void uuiditerfn(struct uuidtrk_s *);

extern void _foreachuuid(struct uuidtrk_s **pp, uuiditerfn *fn);

static inline void
foreachuuid(struct uuidset_s *set, uuiditerfn *fn)
{
	_foreachuuid(&set->first, fn);
}

#elif defined(ACNCFG_UUIDS_HASH)

#define hashcount(bits) (1 << (bits))
#define hashmask(bits) (hashcount(bits) - 1)
#define cidHash(dp, mask) (((mask) > 255) ? ((dp[1] << 8 | dp[2]) & (mask)) : (dp[2]) & (mask))

struct uuidset_s {
	unsigned int mask;
	struct uuidtrk_s *table[];
};

struct uuidtrk_s {
	struct uuidtrk_s *rlnk;
	uuid_t uuid;
};

#define UUIDSETSIZE(hashbits) (\
						sizeof(struct uuidset_s) \
						+ sizeof(struct uuidtrk_s *) * hashcount(hashbits))

#define uuidhash(uuid, mask) ((uuid[1] << 8 | uuid[2]) & (mask))

#if !defined(ACNCFG_UUIDTRACK_INLINE)
extern struct uuidtrk_s *finduuid(struct uuidset_s *set, const uuid_t uuid);
extern int findornewuuid(struct uuidset_s *set, const uuid_t uuid, struct uuidtrk_s **rslt, size_t size);
extern int adduuid(struct uuidset_s *set, struct uuidtrk_s *uup);
extern int unlinkuuid(struct uuidset_s *set, struct uuidtrk_s *uup);
#else  /* defined(ACNCFG_UUIDTRACK_INLINE) */

/**********************************************************************/
static inline struct uuidtrk_s *
finduuid(struct uuidset_s *set, const uuid_t uuid)
{
   struct uuidtrk_s *cp;

   cp = set->table[uuidhash(uuid, set->mask)];
   while (cp && !uuidsEq(cp->uuid, uuid)) cp = cp->rlnk;
   return cp;
}

/**********************************************************************/
static inline int
adduuid(struct uuidset_s *set, struct uuidtrk_s *uup)
{
   struct uuidtrk_s **entry;

	entry = set->table + uuidhash(uup->uuid, set->mask);
   uup->rlnk = *entry;
   *entry = uup;
   return 0;
}

/**********************************************************************/
static inline int
unlinkuuid(struct uuidset_s *set, struct uuidtrk_s *uup)
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

/**********************************************************************/
static inline int
deluuid(struct uuidset_s *set, struct uuidtrk_s *uup, size_t size)
{
	if (unlinkuuid(set, uup) < 0) return -1;
	free(uup);
	return 0;
}

extern int findornewuuid(struct uuidset_s *set, const uuid_t uuid, struct uuidtrk_s **rslt, size_t size);

#endif  /* defined(ACNCFG_UUIDTRACK_INLINE) */

#endif  /* defined(ACNCFG_UUIDS_HASH) */

#endif /* __uuid_h__ */
