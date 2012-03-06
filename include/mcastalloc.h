/**********************************************************************/
/*
Copyright (c) 2011, Philip Nye, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3t
*/
/**********************************************************************/

#ifndef __mcastalloc_h__
#define __mcastalloc_h__ 1

#if CONFIG_EPI10
#include "acnstd/epi10.h"

typedef struct epi10_Lcomp_s {
   grouprx_t  scopenhost;
   uint16_t   dyn_mask;
   uint16_t   dyn_mcast;
} epi10_Lcomp_t;

int epi10_initcomp(Lcomponent_t *Lcomp, grouprx_t scope, uint8_t scopebits);

static inline grouprx_t 
new_mcast_epi10(Lcomponent_t *Lcomp)
{
    grouprx_t dyn;
    dyn = (uint32_t)(Lcomp->epi10.dyn_mask 
					& Lcomp->epi10.dyn_mcast++);

    return Lcomp->scopenhost | htonl(dyn);
}

#endif  /* CONFIG_EPI10 */



static inline grouprx_t 
new_mcast(Lcomponent_t *Lcomp, int type)
{
#if CONFIG_EPI10
    return new_mcast(Lcomp);
#endif
}

int mcast_initcomp(Lcomponent_t *Lcomp);

#endif /* __mcastalloc_h__ */
