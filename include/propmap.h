/* vi: set sw=3 ts=3: */
/**********************************************************************/
/*

	Copyright (c) 2011, Philip Nye, Engineering Arts (UK) philip@engarts.com
	All rights reserved.

	Author: Philip Nye

	$Id$

*/
/**********************************************************************/

#ifndef __propmap_h__
#define __propmap_h__ 1

#define MAX_PROP_SIZE 256

struct member_s;

enum {
	bPROP_VALID,
	bPROP_READ,
	bPROP_WRITE,
	bPROP_EVENT,
	bPROP_VSIZE,
	bPROP_ABS,
};

#define PROP_VALID (1 << bPROP_VALID)
#define PROP_READ (1 << bPROP_READ)
#define PROP_WRITE (1 << bPROP_WRITE)
#define PROP_EVENT (1 << bPROP_EVENT)
#define PROP_VSIZE (1 << bPROP_VSIZE)
#define PROP_ABS (1 << bPROP_ABS)

typedef struct prophd_s prophd_t;
typedef struct propset_s propset_t;
typedef struct propinf_s propinf_t;
typedef struct propmap_s propmap_t;

typedef int getprop_fn(void *ref, uint8_t *valp);
typedef int setprop_fn(void *ref, const uint8_t *valp);
typedef int subscribe_fn(struct member_s *memb, void *ref);

struct propinf_s {
	uint8_t flags;
	int16_t size;
	getprop_fn *getfn;
	setprop_fn *setfn;
	subscribe_fn *subsfn;
	void *fnref;
};

struct propmap_s {
	int nprops;
	int maxaddr;
	int minaddr;
	int maxsize;
	int minsize;
	struct propinf_s map[];
};

typedef int addrtst_t;

struct prophd_s {
	uint32_t addr;
	uint32_t eaddr;
	uint32_t inc;
	addrtst_t atst;
	prophd_t *nxt[2];
};

struct propset_s {
	prophd_t *first;
};

prophd_t *findprop(propset_t *set, uint32_t addr);
int findornewprop(propset_t *set, uint32_t addr, prophd_t **rslt, size_t size);
int addprop(propset_t *set, prophd_t *prop);
int delprop(propset_t *set, prophd_t *prop);
#endif /*  __propmap_h__       */

