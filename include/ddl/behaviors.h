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
struct prop_s;

typedef void bvaction(struct prop_s *prop, bv_t *bv);

struct bv_s {
	ddlkey_t key;
	bvaction *action;
};

extern ddlkey_t *kbehaviors;

static inline bv_t *
findbv(const uuid_t set, const ddlchar_t *name)
{
	return container_of(findkey(&kbehaviors, set, name), bv_t, key);
}

static inline int
register_bv(bv_t *bv)
{
	return addkey(&kbehaviors, &bv->key);
}

#endif /* __behaviors_h__ */
