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
file: component.c

ACN Component management functions
*/

#include "acn.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_MISC
/**********************************************************************/
#if CF_MULTI_COMPONENT
#if CF_UUIDS_RADIX
struct uuidset_s Lcomponents = {NULL};
#elif CF_UUIDS_HASH
struct uuidset_s *Lcomponents = NULL;
#endif
#endif

#if CF_UUIDS_RADIX
struct uuidset_s Rcomponents = {NULL};
#elif CF_UUIDS_HASH
struct uuidset_s *Rcomponents = NULL;
#endif

/**********************************************************************/
int
components_init(void)
{
	LOG_FSTART();
#if CF_UUIDS_HASH
	if (Rcomponents == NULL) {
		Rcomponents = mallocxz(UUIDSETSIZE(CONFIG_R_HASHBITS));
	}
#if CF_MULTI_COMPONENT
	if (Lcomponents == NULL) {
		Lcomponents = mallocxz(UUIDSETSIZE(CONFIG_L_HASHBITS));
	}
#endif
#endif
	LOG_FEND();
	return 0;
}

/**********************************************************************/
void
component_stop(void)
{
	
}

/**********************************************************************/
static int
_initLcomp(
	ifMC(struct Lcomponent_s *Lcomp)
)
{
	ifnMC(struct Lcomponent_s *Lcomp = &localComponent;)
	int rslt;

	LOG_FSTART();
#if CF_SDT
	if ((rslt = mcast_initcomp(ifMC(Lcomp,) NULL)) < 0) {
		acnlogmark(lgDBUG, "mcast_initcomp failed %d", rslt);
		return -1;
	}
	/* start channels at a reasonably random point */
	Lcomp->sdt.lastChanNo = acnrand16(); 
#endif
	LOG_FEND();
	return 0;
}
/**********************************************************************/
int
initstr_Lcomponent(
	ifMC(struct Lcomponent_s *Lcomp,)
	const char* uuidstr
)
{
#if !CF_MULTI_COMPONENT
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif
	LOG_FSTART();
	/* first convert the string since this also checks its format */
	if (str2uuid(uuidstr, Lcomp->uuid) < 0) return -1;
	/* now copy it into our structure */
	strcpy(Lcomp->uuidstr, uuidstr);
	/* and finish the initialization */
	LOG_FEND();
	return _initLcomp(ifMC(Lcomp));
}
/**********************************************************************/
int
initbin_Lcomponent(
	ifMC(struct Lcomponent_s *Lcomp,)
	const uint8_t* uuid
)
{
#if !CF_MULTI_COMPONENT
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif
	LOG_FSTART();
	if (!quickuuidOK(uuid) || uuidIsNull(uuid)) return -1;
	uuidcpy(Lcomp->uuid, uuid);
	uuid2str(uuid, Lcomp->uuidstr);
	LOG_FEND();
	return _initLcomp(ifMC(Lcomp));
}
