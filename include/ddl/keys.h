/************************************************************************/
/*
Copyright (c) 2011, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3s
*/
/************************************************************************/
#ifndef __ddlkeys_h__
#define __ddlkeys_h__ 1

typedef struct ddlkey_s ddlkey_t;

#include "uuid.h"

typedef struct kset_s kset_t;
typedef struct uuidset_s keycollection_t;

struct kset_s {
	uuidhd_t hd;
	unsigned int nkeys;
	ddlkey_t *keys;
};

struct ddlkey_s {
	/* unsigned int namelen; */
	const ddlchar_t *name;
};
int unlinkuuid(uuidset_t *set, uuidhd_t *uup);

ddlkey_t *findkey(keycollection_t *sets, const uuid_t uuid, const ddlchar_t *name);

static inline int addkset(keycollection_t *ksets, kset_t *newset)
{
	return adduuid(ksets, &newset->hd);
}

static inline int unlinkset(keycollection_t *ksets, kset_t *set)
{
	return unlinkuuid(ksets, &set->hd);
}

#endif  /* __ddlkeys_h__ */
