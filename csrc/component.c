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
	struct Lcomponent_s *Lcomp,
	const char *fctn,
	char *uacn
)
{
#if !defined(ACNCFG_MULTI_COMPONENT)
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif

	ZEROTOEND(Lcomp, LcompZEROSTART);
	Lcomp->fctn = fctn;
	Lcomp->uacn = uacn;
#if defined(ACNCFG_SDT)
	if (mcast_initcomp(Lcomp, NULL) < 0) return -1;
#endif
	return 0;
}
/**********************************************************************/
int
initstr_Lcomponent(
	ifMC(struct Lcomponent_s *Lcomp,)
	const char* uuidstr,
	const char *fctn,
	char *uacn
)
{
#if !defined(ACNCFG_MULTI_COMPONENT)
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif
	/* first convert the string since this also checks its format */
	if (str2uuid(uuidstr, Lcomp->uuid) < 0) return -1;
	/* now copy it into our structure */
	strcpy(&Lcomp->uuidstr, uuidstr);
	/* and finish the initialization */
	return _initLcomp(Lcomp, fctn, uacn);
}
/**********************************************************************/
int
initbin_Lcomponent(
	ifMC(struct Lcomponent_s *Lcomp,)
	const uint8_t* uuid,
	const char *fctn,
	char *uacn
)
{
#if !defined(ACNCFG_MULTI_COMPONENT)
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif
	if (!quickuuidOK(uuid) || uuidIsNull(uuid)) return -1;
	uuidcpy(Lcomp->uuid, uuid);
	uuid2str(uuid, Lcomp->uuidstr);
	return _initLcomp(Lcomp, fctn, uacn);
}
