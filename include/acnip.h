/*--------------------------------------------------------------------*/
/*

Copyright (c) 2007, Engineering Arts (UK)

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

	$Id: acnip.h 312 2010-08-26 16:26:18Z philipnye $

*/
/************************************************************************/
/*
  Common routines and macros for IPv4 - and eventually IPv6 - protocols
*/

#ifndef __acnip_h__
#define __acnip_h__ 1

#ifndef HAVE_port_t
  typedef uint16_t port_t;  /* Ports are part of UDP, not IPv4 or IPv6 */
  #define HAVE_port_t
#endif

#ifdef ACNCFG_NET_IPV4

#define DD2HIP(B3, B2, B1, B0) ((((B3) << 8 | (B2)) << 8 | (B1)) << 8 | (B0))      /* in Host order    */
#define DD2NIP(B3, B2, B1, B0) htonl(DD2HIP(B3, B2, B1, B0)) /* in Network order */

/*
port_t ip4addr_t and groupaddr_t variables are generally kept and stored
in network byte order to speed moving them into and out of packets
*/

#ifndef HAVE_ip4addr_t
  typedef uint32_t ip4addr_t;
  #define HAVE_ip4addr_t
#endif

#ifndef HAVE_groupaddr_t
  typedef ip4addr_t groupaddr_t; /* net group is a multicast address */
  #define HAVE_groupaddr_t
#endif

#define is_multicast(addr) (((addr) & htonl(0xf0000000)) == htonl(0xe0000000))
/* following works on in-packet addresses where address is in network order */
#define is_multicastp(addrp) ((*(addrp) & 0xf0) == 0xe0)

#endif

#ifdef ACNCFG_NET_IPV6

#endif

#endif  /* __acnip_h__ */
