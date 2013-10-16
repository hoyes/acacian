/**********************************************************************/
/*
#tabs=3t

Copyright (c) 2013, Engineering Arts (UK)

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

 * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
 * Neither the name of Engineering Arts nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  $Id$

*/
/**********************************************************************/

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
