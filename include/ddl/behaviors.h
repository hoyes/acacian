/**********************************************************************/
/*

	Copyright (C) 2011, Engineering Arts. All rights reserved.

	Author: Philip Nye

	$Id$

#tabs=3
*/
/**********************************************************************/

#ifndef __behaviors_h__
#define __behaviors_h__ 1

#include "ddl/keys.h"

typedef struct bv_s bv_t;
typedef struct bvset_s bvset_t;

//struct prop_s;

typedef void bvaction(struct dcxt_s *dcxp, const bv_t *bv);

struct bv_s {
	ddlchar_t *name;
	bvaction *action;
};

struct bvset_s {
	struct uuidhd_s hd;
	unsigned int nbvs;
	const bv_t *bvs;
};

extern bvaction *unknownbvaction;
extern uuidset_t kbehaviors;
extern const bv_t *findbv(const uuid_t uuid, const ddlchar_t *name, bvset_t **bvset);
extern bvset_t *getbvset(bv_t *bv);

static inline int
register_bvset(bvset_t *bvs)
{
	return adduuid(&kbehaviors, &(bvs->hd));
}

void init_behaviors(void);

#endif /* __behaviors_h__ */
