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
header: component.h

Utilities for management of local and remote components
*/

#ifndef __component_h__
#define __component_h__ 1

/*
	about: Local and remote components
	
	Structures for components

	Because component structures get passed up and down the layers a 
	lot, rather than elaborate cross linking between structures for 
	DMP, SDT RLP etc, we keep them all in sub-structures within a 
	single struct.
	
	This is of course open to abuse — don't.
*/

/*
	type: struct Lcomponent_s
	
	Local component. Members include:

	uint8_t uuid[UUID_SIZE] - CID must be first member for UUID object tracking. See <uuid.h>
	char uuidstr[UUID_STR_SIZE] - keep a string copy too
	const char *fctn - The Fixed Component Type Name as advertised in discovery
	struct epi10_Lcomp_s epi10 - see <struct epi10_Lcomp_s>. *only if* <CF_EPI10>.
	struct sdt_Lcomp_s sdt - SDT substructure. *only if* <CF_SDT>.
	struct dmp_Lcomp_s dmp - DMP substructure. *only if* <CF_DMP>.
	uint16_t lifetime - Discovery lifetime. *only if* <CF_EPI19>.
*/
struct Lcomponent_s {
	uint8_t uuid[UUID_SIZE];
	char uuidstr[UUID_STR_SIZE];
	unsigned usecount;
	const char *fctn;
	char *uacn;
	unsigned flags;
#if CF_EPI10
	struct epi10_Lcomp_s epi10;
#endif
#if CF_SDT
	struct sdt_Lcomp_s sdt;
#endif
#if CF_DMP
	struct dmp_Lcomp_s dmp;
#endif
#if CF_EPI19
	uint16_t lifetime;
	struct acnTimer_s lifetimer;
#endif
};
enum Lcomp_flag_e {
	Lc_advert = 1,
};

#if CF_EPI19
/*
SLP states
*/
enum slp_dmp_e {
	slp_found = 1,
	slp_ctl = 2,
	slp_dev = 4,
	slp_err = 8,
};

struct slp_Rcomp_s {
	uint16_t flags;
	char *fctn;
	char *uacn;
	uint8_t *dcid;
	acnTimer_t slpValidT;
};

#endif
/*
	type: struct Rcomponent_s
	
	Remote component. Members include:

	uint8_t uuid[UUID_SIZE] - CID must be first member for UUID object tracking. See <uuid.h>
	struct slp_Rcomp_s slp - Discovery related data.
	struct sdt_Rcomp_s sdt - SDT related data.
	struct dmp_Rcomp_s dmp - DMP related data

note:
	When there are multiple components using the same ACN instance, if
	one connects to another via ACN then both will create Rcomponent_s 
	as well as their Lcomponent_s.

*/
struct Rcomponent_s {
	uint8_t uuid[UUID_SIZE];
	unsigned usecount;
#if CF_EPI19
	struct slp_Rcomp_s slp;
#endif
#if CF_SDT
	struct sdt_Rcomp_s sdt;
#endif
#if CF_DMP
	struct dmp_Rcomp_s dmp;
#endif
};


/*
	var: Local component(s)
	
	Local component or component set depending on <CF_MULTI_COMPONENT>.
*/
#if CF_MULTI_COMPONENT
/*
	var: Lcomponents

	A <struct uuidset_s> with all registered local components (*if* <CF_MULTI_COMPONENT> is `true`).
*/
extern struct uuidset_s Lcomponents;
#else
/*
	var: localComponent

	A single global <struct Lcomponent_s> (*if* <CF_MULTI_COMPONENT> is `false`).
*/
extern struct Lcomponent_s localComponent;
#endif

/*
	var: Rcomponents

	Remote component set

	The set of remote components which we are communicating with. These
	are managed by the generic UUID tracking code of <uuid.h>.
*/
extern struct uuidset_s Rcomponents;

/**********************************************************************/
/*
func: findLcomp

Find a local component by its CID. If <CF_MULTI_COMPONENT> is false
then the UUID match is still checked before returning <localComponent>.
*/

static inline struct Lcomponent_s *
findLcomp(const uint8_t *uuid)
{
#if !CF_MULTI_COMPONENT
	if (uuidsEq(uuid, localComponent.uuid) && localComponent.usecount > 0)
		return &localComponent;
	return NULL;
#else
	return container_of(finduuid(&Lcomponents, uuid), struct Lcomponent_s, uuid[0]);
#endif
}

/**********************************************************************/
#if CF_MULTI_COMPONENT
/*
func: releaseLcomponent

Unlink a local component from <Lcomponents> and if it is unused free it.
If `not` <CF_MULTI_COMPONENT> a macro equivalent is defined.
*/

static inline void
releaseLcomponent(struct Lcomponent_s *Lcomp)
{
	unlinkuuid(&Lcomponents, Lcomp->uuid);
	if (--Lcomp->usecount == 0) free(Lcomp);
}
#else
#define releaseLcomponent(Lcomp) (Lcomp->usecount ? --Lcomp->usecount : 0)
#endif    /* !CF_SINGLE_COMPONENT */

/**********************************************************************/
#if CF_MULTI_COMPONENT
/*
func: addLcomponent

Add a local component to <Lcomponents>.

*if* <CF_MULTI_COMPONENT> is `true`.
*/
static inline int
addLcomponent(struct Lcomponent_s *Lcomp)
{
	return adduuid(&Lcomponents, Lcomp->uuid);
}
#endif

/**********************************************************************/
/*
func: findRcomp

Find a remote component by its CID.
*/
static inline struct Rcomponent_s *
findRcomp(const uint8_t *uuid)
{
	return container_of(finduuid(&Rcomponents, uuid), struct Rcomponent_s, uuid[0]);
}

/**********************************************************************/
/*
func: releaseRcomponent

Unlink a remote component from <Rcomponents> and if it is unused free it.
*/
static inline void
releaseRcomponent(struct Rcomponent_s *Rcomp)
{
	unlinkuuid(&Rcomponents, Rcomp->uuid);
	if (--(Rcomp->usecount) == 0) free(Rcomp);
}
/**********************************************************************/
/*
func: addRcomponent

Add a remote component to <Rcomponents>.
*/
static inline int
addRcomponent(struct Rcomponent_s *Rcomp)
{
	return adduuid(&Rcomponents, Rcomp->uuid);
}

/**********************************************************************/
/*
prototypes:
*/
extern int components_init(void);
extern int initstr_Lcomponent(ifMC(struct Lcomponent_s *Lcomp,)
						const char* uuidstr);
extern int initbin_Lcomponent(ifMC(struct Lcomponent_s *Lcomp,)
						const uint8_t* uuid);

#endif  /* __component_h__ */
