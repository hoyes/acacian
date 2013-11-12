/**********************************************************************/
/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (c) 2013, Acuity Brands, Inc.

Author: Philip Nye <philip.nye@engarts.com>

#tabs=3
*/
/**********************************************************************/
/*
about: Acacian

Acacian is a full featured implementation of ANSI E1.17 2012
Architecture for Control Networks (ACN) from Acuity Brands

header: mcastalloc.h

Header for <mcastalloc.c>
*/

#ifndef __mcastalloc_h__
#define __mcastalloc_h__ 1

#if ACNCFG_EPI10

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

int mcast_initcomp(ifMC(struct Lcomponent_s *Lcomp,) const struct mcastscope_s *pscope);

static inline grouprx_t 
new_mcast_epi10(epi10_Lcomp_t *Lcomp_epi10)
{
	grouprx_t dyn;
	dyn = (uint32_t)(Lcomp_epi10->dyn_mask 
							& Lcomp_epi10->dyn_mcast++);

	return htonl(Lcomp_epi10->scopenhost | dyn);
}

#define new_mcast(Lcomp) new_mcast_epi10(&(Lcomp)->epi10)
#endif  /* ACNCFG_EPI10 */

union mcastspec_s {
#if ACNCFG_EPI10
	struct mcastscope_s epi10;
#endif
};

#endif /* __mcastalloc_h__ */
