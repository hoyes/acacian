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

struct Lcomponent_s;

typedef struct epi10_Lcomp_s {
	grouprx_t  scopenhost;
	uint16_t   dyn_mask;
	uint16_t   dyn_mcast;
} epi10_Lcomp_t;

struct mcastscope_s {
	grouprx_t scope;
	uint8_t scopebits;
};

int mcast_initcomp(struct Lcomponent_s *Lcomp, struct mcastscope_s *pscope);

static inline grouprx_t 
new_mcast_epi10(epi10_Lcomp_t *Lcomp_epi10)
{
	grouprx_t dyn;
	dyn = (uint32_t)(Lcomp_epi10->dyn_mask 
							& Lcomp_epi10->dyn_mcast++);

	return Lcomp_epi10->scopenhost | htonl(dyn);
}

#define new_mcast(Lcomp) new_mcast_epi10(&(Lcomp)->epi10)
#endif  /* CONFIG_EPI10 */

#endif /* __mcastalloc_h__ */
