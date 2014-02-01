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
property) is completed.

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
struct uuidset_s behaviorsets;

/**********************************************************************/
/*
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
*/
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

	set = (struct bvset_s *)finduuid(&behaviorsets, uuid);
	if (bvset) *bvset = set;

	if (set == NULL) return NULL;
	return (struct bv_s *)keylookup(&set->hasht, name);
}

/**********************************************************************/
/*
*/
const struct bv_s *
findornewbv(const uint8_t *uuid, const ddlchar_t *name)
{
	struct bvset_s *set;
	struct bv_s *bv;

	set = (struct bvset_s *)findornewuuid(&behaviorsets, uuid, sizeof(struct bvset_s));
	if (set == NULL) return NULL; /* out of memory */

	if (set->hasht.used == 0) {
		/*
		new set - need to parse it
		*/
		queue_module(dcxp, TK_behaviorset, set, NULL);
		
		
	}
	bv = (struct bv_s *)keylookup(&set->hasht, name);
	if (bv) return bv;

	if ((bv = findbv(uuid, name, &set))) return bv;
	if (set == NULL) {
		
	}
	setentry = getuuid(bvsets, uuid);
	if ((bvs = *setentry) == NULL) {
		bvs = acnNew(struct bvset_s);
		*setentry = bvs;
	}
	bv = (struct bv_s *)lookup(NULL, &bvs->hasht, name, sizeof(struct bv_s));
	return bv;
}

/**********************************************************************/
#if 0
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
#endif
/**********************************************************************/
/*
func: init_behaviors

Add our defined behaviorsets to the kbehaviors structure where we 
can find them quickly.
*/
void
init_behaviors(void)
{
	struct bv_s *kbv;
	struct bvset_s *bvset;
	static unsigned int nbvs = 0;

	if (nbvs) return;

	bvset = NULL;
	for (kbv = known_bvs; kbv->name; ++kbv) {
		if (kbv->action == NULL) {
			uint8_t uuid[UUID_SIZE];

			if (str2uuid(kbv->name, uuid) < 0) {
				acnlogmark(lgWARN, "Register behaviorset: bad format %s", kbv->name);
				bvset = NULL;
				continue;
			}
			bvset = (struct bvset_s *)findornewuuid(&behaviorsets, uuid, 
										sizeof(struct bvset_s));
			if (bvset == NULL) {
				acnlogmark(lgWARN, "Out of memory");
				continue;
			}
		} else if (bvset == NULL) {
			acnlogmark(lgNTCE, "Behavior \"%s\": unknown behaviorset", kbv->name);
		} else {
			switch (keyadd(&bvset->hasht, &kbv->name)) {
			case 0:
				++nbvs;
				break;
			case KEY_ALREADY:
				acnlogmark(lgNTCE, "Duplicate behavior \"%s\" ignored", kbv->name);
				break;
			case KEY_NOMEM:
			default:
				acnlogmark(lgWARN, "Out of memory");
				break;
			}
		}
	}
	acnlogmark(lgDBUG, "     Registered %u behaviors", nbvs);
}
