/**********************************************************************/
/*
Copyright (c) 2011, Philip Nye, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3t
*/
/**********************************************************************/

#include <arpa/inet.h>
#include "acn.h"

/************************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_MISC

/**********************************************************************/
#if ACNCFG_EPI10
static const struct mcastscope_s defaultscope = {
	.scope = E1_17_AUTO_SCOPE_ADDRESS,
	.scopebits = E1_17_AUTO_SCOPE_BITS,
};

/**********************************************************************/
int
mcast_initcomp(ifMC(struct Lcomponent_s *Lcomp,) const struct mcastscope_s *pscope)
{
#if !ACNCFG_MULTI_COMPONENT
	struct Lcomponent_s * const Lcomp = &localComponent;
#endif
	uint32_t scopenhost;
	netx_addr_t ipad;
	int rslt;

	if (pscope == NULL) pscope = &defaultscope;
	else if (!is_multicast(pscope->scope) 
		|| pscope->scopebits < EPI10_SCOPE_MIN_BITS
		|| pscope->scopebits > EPI10_SCOPE_MAX_BITS
	) {
		acnlogmark(lgERR, "bad multicast scope", rslt);
		return -1;
	}

	rslt = netx_getmyip(NULL, GIPF_DEFAULT, GIPF_DEFAULT,
							(void *)&ipad, sizeof(ipad));

	//acnlogmark(lgDBUG, "netx_getmyip returned %d addresses", rslt);
	if (rslt != 1) {
		acnlogmark(lgDBUG, "netx_getmyip fail", rslt);
		return -1;
	}

	scopenhost = 0 - (1 << (32 - pscope->scopebits));
	scopenhost &= ntohl(pscope->scope);
	if (scopenhost != ntohl(pscope->scope)) {
		acnlogmark(lgWARN, "scope bits discarded");
	}
	scopenhost |= (ntohl(netx_INADDR(&ipad)) & EPI10_HOST_PART_MASK) 
						<< (24 - pscope->scopebits);
	
	Lcomp->epi10.scopenhost = scopenhost;
	Lcomp->epi10.dyn_mask = (1 << (24 - pscope->scopebits)) - 1;
	Lcomp->epi10.dyn_mcast = unmarshalU16(Lcomp->uuid + 2)
									^ unmarshalU16(Lcomp->uuid + 14);
	return 0;
}
#endif  /* CONFIG_EPI10 */
