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

