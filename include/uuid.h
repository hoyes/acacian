/************************************************************************/
/*
Copyright (c) 2011, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3t
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
extern const uint8_t null_uuid[UUID_SIZE];

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

int str2uuid(const char *uuidstr, uint8_t *uuid);
char *uuid2str(const uint8_t *uuid, char *uuidstr);

/* undefine these macros to use external functions */
#define uuidsEq(uup1, uup2) (memcmp((uup1), (uup2), UUID_SIZE) == 0)
#define uuidIsNull(uuid) uuidIsNull(uuid)
#define uuidcpy(dst, src) (memcpy((dst), (src), UUID_SIZE))

#if defined(uuidIsNull)
static inline bool uuidIsNull(const uint8_t *uuid)
{
	int count = UUID_SIZE;

	while (*uuid++ == 0) if (--count == 0) return true;
	return false;
}
#else
extern bool uuidIsNull(const uint8_t *uuid)
#endif

#if !defined(uuidsEq)
extern bool uuidsEq(const uint8_t *uuid1, const uint8_t *uuid2);
#endif

#if !defined(uuidcpy)
extern uint8_t *uuidcpy(uint8_t *dest, const uint8_t *src);
#endif

/*
Check a binary uuid or uuidstring for legal variant and version bits 
but nothing else.

a834b30c-6298-46b3-ac59-c5a0286bb599

*/

static inline bool quickuuidOK(uint8_t *uuid)
{
	return ((uuid[8] & 0xc0) == 0x80 && uuid[6] >= 0x10 && uuid[6] <= 0x5f);
}

/**********************************************************************/
/*
section: UUID search

There are several places where we need to store records indexed 
by UUID which need highly optimized lookup.

UUID records are contained in a uuidset, with operations to find, 
add and remove UUIDs from a set.

These routines deal in pointers to the UUID itself and do not copy 
create or destroy these UUIDs which are typically embedded in larger 
structures which are being tracked.

hint: use <container_of()> to get from the UUID to the structure.

*/

/**********************************************************************/
#if defined(ACNCFG_UUIDS_RADIX)

/* Our set is a simple pointer to the first item */
struct uuidset_s {
	struct uuidtrk_s *first;
};

/* used to test bits in the radix tree may vary with optimization 
for different architectures */
typedef unsigned int uuidtst_t;

struct uuidtrk_s {
	uint8_t *uuid;
	uuidtst_t tstloc;
	struct uuidtrk_s *nxt[2];
};

/**********************************************************************/
#elif defined(ACNCFG_UUIDS_HASH)
/**********************************************************************/

struct uuidset_s {
	unsigned int mask;
	struct uuidtrk_s *table[];
};

struct uuidtrk_s {
	struct uuidtrk_s *rlnk;
	uint8_t *uuid;
};

#define UUIDSETSIZE(hashbits) (\
						sizeof(struct uuidset_s) \
						+ sizeof(struct uuidtrk_s *) * (1 << (hashbits)))

#define uuidhash(uuid, mask) ((uuid[1] << 8 | uuid[2]) & (mask))

#endif  /* defined(ACNCFG_UUIDS_HASH) */
/**********************************************************************/

/*
func: adduuid

Adds uuid to the set (storing the pointer uuid).

returns 0 if a new pointer was added, -1 if it was already there
*/
extern int adduuid(struct uuidset_s *set, const uint8_t *uuid);
/*
func: finduuid

Find the record in the set whose uuid matches the one passed.

Returns the pointer to a matching uuid that was previously added to 
the set or NULL if not found.
*/
extern uint8_t *finduuid(struct uuidset_s *set, const uint8_t *uuid);
/*
func: unlinkuuid

Removes uuid from the set
*/
extern void unlinkuuid(struct uuidset_s *set, const uint8_t *uuid);

#endif /* __uuid_h__ */
