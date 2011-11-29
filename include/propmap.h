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

enum propflags_e {
	pflg_valid      = 1,
	pflg_read       = 2,
	pflg_write      = 4,
	pflg_event      = 8,
	pflg_vsize      = 16,
	pflg_abs        = 32,
    pflg_persistent = 64,
    pflg_constant   = 128,
    pflg_volatile   = 256,
};

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

