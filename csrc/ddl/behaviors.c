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
file: behaviors.c

DDL Behaviors.

A behavior is identified by a UUID and a name. Behavior structures 
may be registered (using ddlkey handling). When a registered 
behavior is encountered during parse the action associated with it 
is executed.

Since behaviors occur in unspecified order and before the property 
content is known, behavior actions may also queue tasks for later 
execution (see add_proptask()) once a property (or ancestor 
proeprty) is completed.

Typical actions in response to behaviors range from simply setting 
property flags (e.g. see persistent behavior) to adding reference 
structures or handler routines to the run-time property map.

A set of standard behaviors are provided here, but applications may 
register other behavior actions.

TODO: implement tracing refinements for unknown behaviors.
*/
/**********************************************************************/

#include <expat.h>
#include "acn.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL
/**********************************************************************/
/*
Variables
*/
bvaction *unknownbvaction = NULL;
struct uuidset_s kbehaviors;

/**********************************************************************/
extern struct bvset_s bvset_acnbase;
extern struct bvset_s bvset_acnbase_r2;
extern struct bvset_s bvset_acnbaseExt1;
extern struct bvset_s bvset_sl;
extern struct bvset_s bvset_artnet;

struct bvset_s *known_bvs[] = {
	&bvset_acnbase,
	&bvset_acnbase_r2,
	&bvset_acnbaseExt1,
	&bvset_sl,
	&bvset_artnet,
};

#define Nknown_bvs ARRAYSIZE(known_bvs)
/**********************************************************************/
/*
func: findbv

Find the struct bv_s corresponding to a behavior in a set.

We use standard acn UUID search to find the set structure, then do a
binary search within the set to find the name.

If argument bvset is not NULL its target is filled in with the set in
which the behavior was found.
*/
const struct bv_s *
findbv(const uint8_t *uuid, const ddlchar_t *name, struct bvset_s **bvset)
{
	struct bvset_s *set;
	const struct bv_s *sp, *ep, *tp;
	int c;
	
	if ((set = container_of(finduuid(&kbehaviors, uuid), struct bvset_s, uuid[0])) == NULL)
		return NULL;

	sp = set->bvs;
	ep = sp + set->nbvs;
	while (ep > sp) {
		tp = sp + (ep - sp) / 2;
		if ((c = strcmp(tp->name, name)) == 0) {
			if (bvset) *bvset = set;
			return tp;
		}
		if (c < 0) sp = tp + 1;
		else ep = tp;
	}
	return NULL;
}

/**********************************************************************/
struct bvset_s *
getbvset(struct bv_s *bv)
{
	int i;
	struct bvset_s *bvset;
	
	for (i = 0; i < Nknown_bvs; ++i) {
		bvset = known_bvs[i];
		if (bv >= bvset->bvs && bv < (bvset->bvs + bvset->nbvs))
			return bvset;
	}
	return NULL;
}

/**********************************************************************/

void
init_behaviors(void)
{
	struct bvset_s **bp;
	static bool initialized = false;

	if (initialized) return;

	for (bp = known_bvs; bp < (known_bvs + Nknown_bvs); ++bp) {
		if (register_bvset(*bp)) {
			acnlogmark(lgWARN, "     Error registering behaviorset");
			
		}
	}
	initialized = true;
	acnlogmark(lgDBUG, "     Registered %lu behavior sets", Nknown_bvs);
}
