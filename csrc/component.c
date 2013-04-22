/**********************************************************************/
/*
Copyright (c) 2010, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3
*/
/**********************************************************************/

#include "acn.h"

/************************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_MISC
/**********************************************************************/
#if !defined(ACNCFG_MULTI_COMPONENT)
struct Lcomponent_s localComponent;
#elif defined(ACNCFG_UUIDS_RADIX )
uuidset_t Lcomponents = {NULL};
#elif defined(ACNCFG_UUIDS_HASH)
uuidset_t *Lcomponents = NULL;
#endif

#if defined(ACNCFG_UUIDS_RADIX )
uuidset_t Rcomponents = {NULL};
#elif defined(ACNCFG_UUIDS_HASH)
uuidset_t *Rcomponents = NULL;
#endif

/**********************************************************************/
static int
components_init(void)
{
#if defined(ACNCFG_UUIDS_HASH)
	if (Rcomponents == NULL) {
		Rcomponents = mallocxz(UUIDSETSIZE(CONFIG_R_HASHBITS));
	}
#if defined(ACNCFG_MULTI_COMPONENT)
	if (Lcomponents == NULL) {
		Lcomponents = mallocxz(UUIDSETSIZE(CONFIG_L_HASHBITS));
	}
#endif
#endif
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
#	if defined(ACNCFG_MULTI_COMPONENT)
	struct Lcomponent_s *Lcomp,
#	endif
)
{
	memset(((void *)Lcomp) + UUID_SIZE, 0, sizeof(struct Lcomponent_s) - UUID_SIZE);
#	if defined(ACNCFG_SDT)
	if (mcast_initcomp(Lcomp, NULL) < 0) return -1;
#	endif
	return 0;
}
/**********************************************************************/
int
initstr_Lcomponent(
#	if defined(ACNCFG_MULTI_COMPONENT)
	struct Lcomponent_s *Lcomp,
#	endif
	const char* uuidstr
)
{
#if !defined(ACNCFG_MULTI_COMPONENT)
#define Lcomp (&localComponent)
#endif
	if (str2uuid(uuidstr, Lcomp->uuid) < 0) return -1;
	return _initLcomp(Lcomp);
}
/**********************************************************************/
int
initbin_Lcomponent(
#	if defined(ACNCFG_MULTI_COMPONENT)
	struct Lcomponent_s *Lcomp,
#	endif
	const uint8_t* uuid
)
{
#if !defined(ACNCFG_MULTI_COMPONENT)
#define Lcomp (&localComponent)
#endif
	uuidcpy(Lcomp->uuid, uuid);
	return _initLcomp(Lcomp);
}
