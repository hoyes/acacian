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

//#include "ddl/keys.h"

//struct prop_s;

typedef void bvaction(struct dcxt_s *dcxp, const struct bv_s *bv);

struct bv_s {
	ddlchar_t *name;
	bvaction *action;
};

struct bvset_s {
	uint8_t uuid[UUID_SIZE];
	unsigned int nbvs;
	const struct bv_s *bvs;
};

extern bvaction *unknownbvaction;
extern struct uuidset_s kbehaviors;
extern const struct bv_s *findbv(const uint8_t *uuid, const ddlchar_t *name, struct bvset_s **bvset);
extern struct bvset_s *getbvset(struct bv_s *bv);

static inline int
register_bvset(struct bvset_s *bvs)
{
	return adduuid(&kbehaviors, bvs->uuid);
}

void init_behaviors(void);

#endif /* __behaviors_h__ */
