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
#if CONFIG_SINGLE_COMPONENT
Lcomponent_t localComponent;
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
init_Lcomponent(Lcomponent_t *Lcomp, const char *cidstr)
{
	memset(Lcomp, 0, sizeof(Lcomponent_t));
	if (str2uuid(cidstr, Lcomp->hd.uuid) < 0) return -1;

#if !CONFIG_SINGLE_COMPONENT
	if (adduuid(&Lcomponents, &Lcomp->hd) < 0) return -1;
#endif
	return 0;
}
