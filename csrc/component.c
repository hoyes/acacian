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
#if !ACNCFG_MULTI_COMPONENT
struct Lcomponent_s localComponent;
#elif ACNCFG_UUIDS_RADIX
struct uuidset_s Lcomponents = {NULL};
#elif ACNCFG_UUIDS_HASH
struct uuidset_s *Lcomponents = NULL;
#endif

#if ACNCFG_UUIDS_RADIX
struct uuidset_s Rcomponents = {NULL};
#elif ACNCFG_UUIDS_HASH
struct uuidset_s *Rcomponents = NULL;
#endif

/**********************************************************************/
int
components_init(void)
{
#if ACNCFG_UUIDS_HASH
	if (Rcomponents == NULL) {
		Rcomponents = mallocxz(UUIDSETSIZE(CONFIG_R_HASHBITS));
	}
#if ACNCFG_MULTI_COMPONENT
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
	ifMC(struct Lcomponent_s *Lcomp,)
	const char *fctn,
	char *uacn
)
{
#if !ACNCFG_MULTI_COMPONENT
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif
	int rslt;

	ZEROTOEND(Lcomp, LcompZEROSTART);
	Lcomp->fctn = fctn;
	Lcomp->uacn = uacn;
#if ACNCFG_SDT
	if ((rslt = mcast_initcomp(ifMC(Lcomp,) NULL)) < 0) {
		acnlogmark(lgDBUG, "mcast_initcomp failed %d", rslt);
		return -1;
	}
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
#if !ACNCFG_MULTI_COMPONENT
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif
	/* first convert the string since this also checks its format */
	if (str2uuid(uuidstr, Lcomp->uuid) < 0) return -1;
	/* now copy it into our structure */
	strcpy(Lcomp->uuidstr, uuidstr);
	/* and finish the initialization */
	return _initLcomp(ifMC(Lcomp,) fctn, uacn);
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
#if !ACNCFG_MULTI_COMPONENT
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif
	if (!quickuuidOK(uuid) || uuidIsNull(uuid)) return -1;
	uuidcpy(Lcomp->uuid, uuid);
	uuid2str(uuid, Lcomp->uuidstr);
	return _initLcomp(ifMC(Lcomp,) fctn, uacn);
}
