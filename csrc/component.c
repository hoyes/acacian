/**********************************************************************/
/*
Copyright (c) 2010, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3
*/
/**********************************************************************/

#include "acncommon.h"
#include "uuid.h"
#include "acnmem.h"
#include "acnlog.h"
#include "netxface.h"
#include "sdt.h"

/************************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_MISC
/**********************************************************************/
#if CONFIG_SINGLE_COMPONENT
Lcomponent_t *localComponent = NULL;
#else
#if CONFIG_UUIDTRACK == UUIDS_RADIX
uuidset_t Lcomponents = {NULL};
#elif CONFIG_UUIDTRACK == UUIDS_HASH
uuidset_t *Lcomponents = NULL;
#endif
#endif

#if CONFIG_UUIDTRACK == UUIDS_RADIX
uuidset_t Rcomponents = {NULL};
#elif CONFIG_UUIDTRACK == UUIDS_HASH
uuidset_t *Rcomponents = NULL;
#endif

/**********************************************************************/
int
components_init(void)
{
#if CONFIG_SINGLE_COMPONENT
	if (localComponent == NULL) {
		localComponent = acnNew(Lcomponent_t);
	}
#endif

#if CONFIG_UUIDTRACK == UUIDS_HASH
	if (Rcomponents == NULL) {
		Rcomponents = mallocxz(UUIDSETSIZE(CONFIG_R_HASHBITS));
	}
#if !CONFIG_SINGLE_COMPONENT
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
int
init_Lcomponent(Lcomponent_t *Lcomp, cid_t cid)
{
	int rslt;

	memset(Lcomp, 0, sizeof(comp);
	uuidcpy(Lcomp->hd.uuid, cid);
	rslt = mcast_initcomp(Lcomp);
}
