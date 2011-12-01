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

A behavior is identified by a UUID and a name. This is associated 
with a behavior action structure which defines how to treat 
properties with a given behavior. The action structure is largely 
application dependent and it is likely that multiple behavior names 
will use the same action structure. Actions may be required at class 
build time, at class instantiate time, when the property is 
accessed, when an instance is deleted or when the whole class is 
deleted.

For each behaviorset we generate an array of keys each of which connects
the set UUID, the name string and the action structure. For each 
property we then store an array of behavior keys.

The key array is sorted in lexical order of names allowing binary search
strategies.

Before calling the parser, the application can register known 
behaviorsets. On completion of parsing each device class, we can then 
optionally call the application with any class build time actions.

For behaviors which are not recognized as parts of pre-registered 
behaviorsets, new sets and records are generated with NULL actions, 
but this is a relatively expensive process - it is therefore 
beneficial to pre-register as many behaviors as possible, even if no 
actions are provided for them.

TODO: implement tracing refinements for unknown behaviors.
*/

#include <expat.h>
#include "acncommon.h"
#include "acnlog.h"
#include "acnmem.h"
#include "uuid.h"
#include "behaviors.h"

/**********************************************************************/
/*
Variables
*/
bvaction *unknownbvaction;
uuidset_t kbehaviors;

/**********************************************************************/
/*
Find a behavior (name and set-UUID) from the known behaviors
returns NULL if the behavior is not present
*/

struct bvkey_s *
findbv(uuid_t setuuid, const XML_Char *name, struct bvset_s **bsetp)
{
	struct bvset_s *bset;
	struct bvkey_s *bkey;
	unsigned int hi, lo, i;
	int cmp;

	bset = findbset(setuuid);
	if (bset == NULL) return NULL;
	if (bsetp) *bsetp = bset;

	lo = 0;
	hi = bset->numkeys;
	bkey = bset->keys;

	while (hi > lo) {
		i = (hi + lo) / 2;
		cmp = strcmp(name, bkey[i].name);
		if (cmp == 0) return bkey + i;
		if (cmp < 0) hi = i;
		else lo = i + 1;
	}
	return NULL;
}

#if 0
/**********************************************************************/
/*
Call to register a new behavior. If the number of behaviors is known
or can be estimated then this number of entries will be created, else
the set will be created for DFLTBVCOUNT behaviors.

If the number of behaviors added to a set exceeds the number of 
entries then an error is returned. (the set should be dynamically 
expanded but this is an expensive operation).
*/

int
findornewbv(uuid_t setuuid, const XML_Char *name, int countifnew, struct bvkey_s **rtnkey)
{
	struct uuidhd_s *bsethd;
	struct bvset_s *bset;
	struct bvkey_s *bkey;
	unsigned int hi, lo, i;
	int cmp;
	int rslt;

	rslt = findornewuuid(&kbehaviors, setuuid, &bsethd, sizeof(struct bvset_s));
	if (rslt < 0) return rslt;

	bset = container_of(bsethd, struct bvset_s, hd);

	if (rslt) {   /* creating whole new set */
		if (countifnew <= 0) countifnew = DFLTBVCOUNT;
		bset->maxkeys = countifnew;
		
		countifnew *= sizeof(struct bvkey_s);
		
		/* allocate an array */
		bkey = mallocxz(countifnew);
		hi = 0;
	} else {  /* existing set */
		/* is the behavior there? */
		lo = 0;
		hi = bset->numkeys;
		bkey = bset->keys;

		while (hi > lo) {
			i = (hi + lo) / 2;
			cmp = strcmp(name, bkey[i].name);
			if (cmp == 0) {
				if (rtnkey) *rtnkey = bkey + i;
				return 0;
			} else if (cmp < 0) hi = i;
			else lo = i + 1;
		}
		if (bset->numkeys >= bset->maxkeys) return -1;	/* no space */
		bkey += hi;
		memmove(bkey + 1, 
					bkey, 
					sizeof(struct bvkey_s) * (bset->numkeys - hi));
	}
	/* got a new behavior which goes at bkey */
	bset->numkeys++;
	bkey->name = name;
	bkey->set = bset;
	bkey->action = NEWBVACTION;
	if (rtnkey) *rtnkey = bkey;
	return 1;
}
#endif
