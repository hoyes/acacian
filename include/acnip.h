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
header: acnip.h

Common routines and macros for IPv4 - and eventually IPv6 - protocols
*/

#ifndef __acnip_h__
#define __acnip_h__ 1

/*
typedef: port_t

May already be defined in yout programming enironment. Otherwise 
defined here.

Note ports are part of UDP or TCP, not IPv4 or IPv6
*/
#ifndef HAVE_port_t
  typedef uint16_t port_t;
  #define HAVE_port_t
#endif

#if CF_NET_IPV4

/*
types: ip4addr_t and groupaddr_t

IPv4 address, group and mask variables are usually kept and stored
in network byte order.

Only defined if <CF_NET_IPV4> is set
*/

#ifndef HAVE_ip4addr_t
  typedef uint32_t ip4addr_t;
  #define HAVE_ip4addr_t
#endif

#ifndef HAVE_groupaddr_t
  typedef ip4addr_t groupaddr_t; /* net group is a multicast address */
  #define HAVE_groupaddr_t
#endif

/*
macros: multicast tests

In both cases addr is in network order.

Only defined if <CF_NET_IPV4> is set

is_multicast(addr) - test whether addr is a multicast group address
is_multicastp(addrp) - test whether the address pointed to by addrp
is multicast
*/
#define is_multicast(addr) (((addr) & htonl(0xf0000000)) == htonl(0xe0000000))
/* following works on in-packet addresses where address is in network order */
#define is_multicastp(addrp) ((*((uint8_t *)addrp) & 0xf0) == 0xe0)

/*
macros: Converting dotted decimal style literals to host or network 
order IPv4 addresses.

e.g. DD2HIP(192,168,1,222)

DD2HIP - Convert to host order
DD2NI - COnvert to network order
*/
#define DD2HIP(B3, B2, B1, B0) ((((B3) << 8 | (B2)) << 8 | (B1)) << 8 | (B0))      /* in Host order    */
#if LITTLE_ENDIAN
#define DD2NIP(B3, B2, B1, B0) ((((B0) << 8 | (B1)) << 8 | (B2)) << 8 | (B3))   /* in Network order */
#else
#define DD2NIP(B3, B2, B1, B0) DD2HIP(B3, B2, B1, B0) /* in Network order */
#endif

#endif

#if CF_NET_IPV6

/*
topic: IPv6 specific macros and types

Not yet complete
*/

#endif

#endif  /* __acnip_h__ */
