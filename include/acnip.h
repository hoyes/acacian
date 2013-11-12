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

header: acnip.h

  Common routines and macros for IPv4 - and eventually IPv6 - protocols
*/

#ifndef __acnip_h__
#define __acnip_h__ 1

#ifndef HAVE_port_t
  typedef uint16_t port_t;  /* Ports are part of UDP, not IPv4 or IPv6 */
  #define HAVE_port_t
#endif

#if ACNCFG_NET_IPV4

#define DD2HIP(B3, B2, B1, B0) ((((B3) << 8 | (B2)) << 8 | (B1)) << 8 | (B0))      /* in Host order    */
#if LITTLE_ENDIAN
#define DD2NIP(B3, B2, B1, B0) ((((B0) << 8 | (B1)) << 8 | (B2)) << 8 | (B3))   /* in Network order */
#else
#define DD2NIP(B3, B2, B1, B0) DD2HIP(B3, B2, B1, B0) /* in Network order */
#endif

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

#if ACNCFG_NET_IPV6

#endif

#endif  /* __acnip_h__ */
