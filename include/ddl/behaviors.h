/**********************************************************************/
/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (c) 2013, Acuity Brands, Inc.

Author: Philip Nye <philip.nye@engarts.com>

This file forms part of Acacian a full featured implementation of 
ANSI E1.17 Architecture for Control Networks (ACN)

#tabs=3
*/
/**********************************************************************/
/*
header: behaviors.h

DDL behavior handling
*/

#ifndef __behaviors_h__
#define __behaviors_h__ 1

#define MAX_REFINES 6

typedef void bvaction(struct dcxt_s *dcxp, const struct bv_s *bv);

struct bv_s {
	const ddlchar_t *name;
	bvaction *action;
	struct bv_s *refines[MAX_REFINES];
};

struct bvinit_s {
	const char *name;
	bvaction *action;
};

struct bvset_s {
	uint8_t uuid[UUID_SIZE];
	struct hashtab_s hasht;
};

extern struct uuidset_s kbehaviors;
extern const struct bv_s *findbv(const uint8_t *uuid, const ddlchar_t *name, struct bvset_s **bvset);
//extern struct bvset_s *getbvset(struct bv_s *bv);

void init_behaviors(void);

#endif /* __behaviors_h__ */
