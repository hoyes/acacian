/**********************************************************************/
/*
Copyright (c) 2011, Philip Nye, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3t
*/
/**********************************************************************/

#include <arpa/inet.h>
#include "acncommon.h"
#include "component.h"
#include "acnip.h"
#include "mcastalloc.h"
#include "acnlog.h"

#if CONFIG_EPI10
int
epi10_initcomp(Lcomponent_t *Lcomp, grouprx_t scope, int scopebits)
{
	uint32_t scopenhost;

	if (scope == 0 && scopebits == 0) {
		scope = E1_17_AUTO_SCOPE_ADDRESS;
		scopebits = E1_17_AUTO_SCOPE_BITS;
	}
	if (!is_multicast(scope) 
		|| scopebits < EPI10_SCOPE_MIN_BITS
		|| scopebits > EPI10_SCOPE_MAX_BITS
	) {
		return -1;
	}

	scopenhost = 0 - (1 << (32 - scopebits));
	scopenhost &= ntohl(scope);
	if (scopenhost != ntohl(scope)) {
		acnlogmark(lgWARN, "scope bits discarded");
	}
	scopenhost |= (ntohl(netx_getmyip(0)) & EPI10_HOST_PART_MASK) 
						<< (24 - scopebits);
	
	Lcomp->epi10.scopenhost = scopenhost;
	Lcomp->epi10.dyn_mask = (1 << (24 - scopebits)) - 1;
	Lcomp->epi10.dyn_mcast = unmarshalU16(Lcomp->hd.uuid + 2)
									^ unmarshalU16(Lcomp->hd.uuid + 14);
}
#endif

int
mcast_initcomp(Lcomponent_t *Lcomp)
{
	int rslt = 0;

#if CONFIG_EPI10
	rslt = epi10_initcomp(Lcomp, E1_17_AUTO_SCOPE_ADDRESS, E1_17_AUTO_SCOPE_BITS);
#endif
	return rslt;
}

