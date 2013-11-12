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

header: netx_bsd.h

Networking macros and functions for BSD sockets interface
*/

#if !defined(__netx_bsd_h__)
#define __netx_bsd_h__ 1

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
/*
   We use each stack's native structure for holding transport layer (UDP)
   addresses. This information is typedef'd to netx_addr_t. To unify the
   API we hide each stack's implementation of netx_addr_t behind macros
   which must be defined for each stack type.

   #define netx_PORT(netaddr_s_ptr)
   #define netx_INADDR(netaddr_s_ptr)
   #define netx_INIT_ADDR(addrp, inaddr, port)
   #define netx_INIT_ADDR_STATIC(inaddr, port)

   netx_PORT and netx_INADDR access the port and host address parts
   respectively. These should be lvalues so they can be assigned as well
   as read.

   netx_INIT_ADDR sets both address and port and initializes all fields
   netx_INIT_ADDR_STATIC expands to a static structure initializer

   Addresses are in "NETWORK order" (Big-Endian)
*/
/************************************************************************/

typedef int nativesocket_t;
#define NATIVE_NOSOCK -1

#if ACNCFG_NET_MULTI
#define netx_FAMILY AF_UNSPEC
typedef struct sockaddr_storage netx_addr_t;

/* operations performed on netx_addr_t */
#define netx_TYPE(addrp) (addrp)->ss_family
#define netx_PORT(addrp) ((addrp)->ss_family == AF_INET ?\
	((struct sockaddr_in *)(addrp))->sin_port :\
	(addrp)->ss_family == AF_INET6 ?\
	((struct sockaddr_in6 *)(addrp))->sin6_port :\
	netx_PORT_NONE)
#define netx_SINADDR(addrp) ((struct sockaddr_in *)(addrp))->sin_addr
#define netx_SIN6ADDR(addrp) ((struct sockaddr_in6 *)(addrp))->sin6_addr
#define netx_INADDR(addrp) ((struct sockaddr_in *)(addrp))->sin_addr.s_addr
#define netx_ADDRP(addrp) (netx_TYPE(addrp) == AF_INET ? \
							(&netx_SINADDR(addrp)) : \
							(&netx_SIN6ADDR(addrp)))

#define adhocIsValid(addrp) (netx_TYPE(addrp) == AF_INET || netx_TYPE(addrp) == AF_INET6)

#define SDT_TA_TYPE(addrp) ((addrp)->ss_family == AF_INET ?\
								SDT_ADDR_IPV4 :\
							((addrp)->ss_family == AF_INET) ?\
								SDT_ADDR_IPV6 :\
								SDT_ADDR_NULL)

#error not currently supported

#elif ACNCFG_NET_IPV4
#define netx_FAMILY AF_INET
typedef struct sockaddr_in netx_addr_t;
#define netx_ADDRLEN 4

/* operations performed on netx_addr_t */
#define netx_TYPE(addrp) (addrp)->sin_family
#define netx_PORT(addrp) (addrp)->sin_port
#define netx_SINADDR(addrp) (addrp)->sin_addr
#define netx_INADDR(addrp) (addrp)->sin_addr.s_addr
#define netx_ADDRP(addrp) (&netx_SINADDR(addrp))

#define netx_INIT_ADDR_STATIC(inaddr, port) {AF_INET, (port), {inaddr}}
#define netx_INIT_ADDR(addrp, inaddr, port) ( \
      netx_TYPE(addrp) = AF_INET, \
      netx_INADDR(addrp) = (inaddr), \
      netx_PORT(addrp) = (port) \
   )
#define netx_INIT_ADDR_ANY(addrp, port) netx_INIT_ADDR(addrp, 0, port)

#define adhocIsValid(addrp) (netx_TYPE(addrp) == AF_INET)
#define SDT_TA_TYPE(addrp) (((addrp)->sin_family == AF_INET) ? SDT_ADDR_IPV4 : SDT_ADDR_NULL)
#define netx_is_multicast(addrp) is_multicast(netx_INADDR(addrp))
#define netx_addrmatch(addrp1, addrp2) \
	((netx_INADDR(addrp1)) == (netx_INADDR(addrp2)))

#define addrsetANY(addrp) (netx_INADDR(addrp) = ((ip4addr_t)0))
#define netx_ISADDR_ANY(addrp) (netx_INADDR(addrp) == INADDR_ANY)

#if ACNCFG_LOCALIP_ANY
typedef ip4addr_t grouprx_t;
#else /* !ACNCFG_LOCALIP_ANY */
typedef struct grouprx_s {
   ip4addr_t group;
   ip4addr_t interface;
} grouprx_t;
#endif

#elif ACNCFG_NET_IPV6
#define netx_FAMILY AF_INET6
typedef struct sockaddr_in6 netx_addr_t;
#define netx_ADDRLEN 16

/* operations performed on netx_addr_t */
#define netx_TYPE(addrp) (addrp)->sin6_family
#define netx_PORT(addrp) (addrp)->sin6_port
#define netx_SIN6ADDR(addrp) (addrp)->sin6_addr
#define netx_IN6ADDR(addrp) (addrp)->sin6_addr.s6_addr
#define netx_ADDRP(addrp) (&netx_SIN6ADDR(addrp))

#define netx_INIT_ADDR_STATIC(inaddr, port) {.sin6_family = AF_INET6,\
                                             .sin6_port = (port),\
                                             .sin6_addr = {inaddr}}
#define netx_INIT_ADDR(addrp, inaddr, port) ( \
      netx_TYPE(addrp) = AF_INET6, \
      memcpy(netx_INADDR(addrp), (inaddr), sizeof(netx_INADDR(addrp))), \
      netx_PORT(addrp) = (port) \
   )
#define netx_INIT_ADDR_ANY(addrp, port) netx_INIT_ADDR(addrp, &in6addr_any, port)

#define adhocIsValid(addrp) (netx_TYPE(addrp) == AF_INET6)
#define SDT_TA_TYPE(addrp) (((addrp)->sin_family == AF_INET6) ? SDT_ADDR_IPV6 : SDT_ADDR_NULL)
#define netx_addrmatch(addrp1, addrp2) \
	(memcmp(netx_INADDR(addrp1), netx_INADDR(addrp2), netx_ADDRLEN) == 0)

#define addrsetANY(addrp) memcpy(&(addrp)->sin6_addr, &in6addr_any, 16)
#define netx_ISADDR_ANY(addrp) (memcmp(&netx_SIN6ADDR(addrp), IN6ADDR_ANY_INIT, sizeof(struct in6_addr)) == 0)

#if ACNCFG_LOCALIP_ANY
typedef struct in6_addr grouprx_t;
#else /* !ACNCFG_LOCALIP_ANY */
typedef struct grouprx_s {
   struct in6_addr group;
   struct in6_addr interface;
} grouprx_t;
#endif

#else
#error Unknown network configuration
#endif




/************************************************************************/
/*
There are fairly major differences if ACNCFG_LOCALIP_ANY is true (which
is the common case).
In this case we don't need to track local interface addresses at all, we
just leave it up to the stack. All we need to do is pass local port
numbers which are easy to handle. However, because of the differences in
passing conventions between ports which are integers (uint16_t) and
structures which are passed by reference, things are not so transparent
as would be nice. We define both localaddr_t (the storage type) and
localaddr_arg_t (the argument type) then use macros LCLAD_ARG(lcladdr)
and LCLAD_UNARG(lcladdrarg)
*/

#if ACNCFG_LOCALIP_ANY

typedef port_t localaddr_t;

/* operations on localaddr_t */
#define netx_INIT_LOCALADDR(addrp, addr, port) (*addrp = (port))
#define netx_INIT_LOCALADDR_STATIC(addr, port) (port)
#define lclad_PORT(lclad) (lclad)
#define lclad_INADDR(lclad) netx_INADDR_ANY
#define lcladEq(a, b) ((a) == (b))

#define groupaddr(gprx) (gprx)
#define groupiface(gprx) INADDR_ANY

#else /* !ACNCFG_LOCALIP_ANY */

typedef netx_addr_t localaddr_t;

/* operations on localaddr_t */
#define netx_INIT_LOCALADDR(addrp, addr, port) netx_INIT_ADDR(addrp, addr, port)
#define netx_INIT_LOCALADDR_STATIC(addr, port) netx_INIT_ADDR_STATIC(inaddr, port)
#define lclad_PORT(laddr) ((laddr).sin_port)
#define lclad_INADDR(laddr) ((laddr).sin_addr.s_addr)
#define lcladEq(a, b) (lclad_PORT(a) == lclad_PORT(b) && lclad_INADDR(a) == lclad_INADDR(b))

#define groupaddr(gprx) ((gprx).group)
#define groupiface(gprx) ((gprx).interface)

#endif /* !ACNCFG_LOCALIP_ANY */

/* operations on netxsock_t */
#define NSK_PORT(nskptr) netx_PORT(&(nskptr)->localaddr)
#define NSK_INADDR(nskptr) netx_INADDR(&(nskptr)->localaddr)

#if RECEIVE_DEST_ADDRESS
#define netx_PKTINFO_LEN CMSG_SPACE(sizeof(struct in_pktinfo))
#endif

/************************************************************************/
#ifndef netx_PORT_NONE
#define netx_PORT_NONE 0
#endif

#ifndef netx_PORT_HOLD
#define netx_PORT_HOLD 65535
#endif

#ifndef netx_PORT_EPHEM
#define netx_PORT_EPHEM (port_t)0
#endif

#ifndef netx_INADDR_ANY
#define netx_INADDR_ANY ((ip4addr_t)0)
#endif

#ifndef netx_INADDR_NONE
#define netx_INADDR_NONE ((ip4addr_t)0xffffffff)
#endif

#ifndef netx_GROUP_UNICAST
#define netx_GROUP_UNICAST netx_INADDR_ANY
#endif

#ifndef netx_INIT_ADDR
#define netx_INIT_ADDR(addrp, addr, port) (netx_INADDR(addrp) = (addr), netx_PORT(addrp) = (port))
#endif

#ifndef netx_SOCK_NONE
#define netx_SOCK_NONE 0
#endif

#define new_txbuf(size)  mallocx(size)
#define free_txbuf(x, size)  free(x)

#define netx_close(sock) close(sock)

#ifdef __cplusplus
}
#endif

#endif  /* #if (ACNCFG_STACK_BSD || ACNCFG_STACK_CYGWIN) && !defined(__netx_bsd_h__) */
