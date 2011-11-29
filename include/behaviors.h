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

struct bvkey_s;
struct bvset_s;
struct prop_s;

typedef void bvaction(struct prop_s *prop, struct bvset_s *set, struct bvkey_s *key);

struct bvkey_s {
	const char *name;
	bvaction *action;
	void *actionref;
};

struct bvset_s {
	struct uuidhd_s hd;
	int numkeys;
	struct bvkey_s *keys;
};

extern uuidset_t kbehaviors;
extern bvaction *unknownbvaction;

static inline struct bvset_s *
findbset(const uuid_t cid)
{
	return container_of(finduuid(&kbehaviors, cid), struct bvset_s, hd);
}

static inline int
register_bvset(struct bvset_s *bset)
{
	return adduuid(&kbehaviors, &(bset->hd));
}

extern struct bvkey_s *findbv(uuid_t set, const XML_Char *name, struct bvset_s **bsetp);

#endif /* __behaviors_h__ */
