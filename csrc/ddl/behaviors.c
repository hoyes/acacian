/**********************************************************************/
/*

	Copyright (C) 2011, Engineering Arts. All rights reserved.

	Author: Philip Nye

	$Id$

#tabs=3
*/
/**********************************************************************/
/*
Behaviors

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
#include "acncommon.h"
#include "acnlog.h"
#include "acnmem.h"
#include "uuid.h"
#include "ddl/parse.h"
//#include "propmap.h"
#include "ddl/behaviors.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL
/**********************************************************************/
/*
Variables
*/
bvaction *unknownbvaction;
uuidset_t kbehaviors;

/**********************************************************************/
extern bvset_t bvset_acnbase;
extern bvset_t bvset_acnbase_r2;
extern bvset_t bvset_acnbaseExt1;
extern bvset_t bvset_sl;
extern bvset_t bvset_artnet;

bvset_t *known_bvs[] = {
	&bvset_acnbase,
	&bvset_acnbase_r2,
	&bvset_acnbaseExt1,
	&bvset_sl,
	&bvset_artnet,
};

#define Nknown_bvs arraycount(known_bvs)
/**********************************************************************/
const bv_t *
findbv(const uint8_t *uuid, const ddlchar_t *name, bvset_t **bvset)
{
	bvset_t *set;
	const bv_t *sp, *ep, *tp;
	int c;
	
	if ((set = container_of(finduuid(&kbehaviors, uuid), bvset_t, hd)) == NULL)
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
bvset_t *
getbvset(bv_t *bv)
{
	int i;
	bvset_t *bvset;
	
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
	bvset_t **bp;

	for (bp = known_bvs; bp < (known_bvs + Nknown_bvs); ++bp) {
		if (register_bvset(*bp)) {
			acnlogmark(lgWARN, "     Error registering behaviorset");
			
		}
	}
	acnlogmark(lgDBUG, "     Registered %lu behavior sets", Nknown_bvs);
}
