/**********************************************************************/
/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (c) 2013, Acuity Brands, Inc.

Author: Philip Nye <philip.nye@engarts.com>

This file forms part of Acacian a full featured implementation of 
ANSI E1.17 Architecture for Control Networks (ACN)

#tabs=3
*/
/**********************************************************************/
/*
header: getip.h

Header for <getip.c>
*/

#ifndef __getip_h__
#define __getip_h__

#include <net/if.h>

/*
enum: getipflag_e

Flags for filtering ip address search
	GIPF_UP          - self explanatory
	GIPF_BROADCAST   - self explanatory
	GIPF_LOOPBACK    - self explanatory
	GIPF_POINTOPOINT - self explanatory
	GIPF_PROMISC     - self explanatory
	GIPF_ALLMULTI    - self explanatory
	GIPF_MULTICAST   - self explanatory
	GIPF_IPv4        - self explanatory
	GIPF_IPv6        - self explanatory

*/
enum getipflag_e {
	GIPF_UP          = IFF_UP,
	GIPF_BROADCAST   = IFF_BROADCAST,
	GIPF_LOOPBACK    = IFF_LOOPBACK,
	GIPF_POINTOPOINT = IFF_POINTOPOINT,
	GIPF_PROMISC     = IFF_PROMISC,
	GIPF_ALLMULTI    = IFF_ALLMULTI,
	GIPF_MULTICAST   = IFF_MULTICAST,

	GIPF_IPv4        = 0x80000000,
	GIPF_IPv6        = 0x40000000,
};

#define GIPF_DEFAULT ((unsigned int)-1)
#define GIPF_ALL (0 \
	| GIPF_UP          \
	| GIPF_BROADCAST   \
	| GIPF_LOOPBACK    \
	| GIPF_POINTOPOINT \
	| GIPF_PROMISC     \
	| GIPF_ALLMULTI    \
	| GIPF_MULTICAST   \
	| GIPF_IPv4        \
	| GIPF_IPv6        \
	)

int netx_getmyip(
	const char *interfaces[],
	uint32_t flagmask,
	uint32_t flagmatch,
	void *addrlist,
	size_t size);

int netx_getmyipstr(
	const char *interfaces[],
	uint32_t flagmask,
	uint32_t flagmatch,
	char **ipstrs,
	int maxaddrs);

void netx_freeipstr(char **strs);

#endif  /* __getip_h__ */
