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

    $Id: netx_bsd.h 351 2010-09-06 13:30:35Z philipnye $

*/
/*
#tabs=3s
*/
/*--------------------------------------------------------------------*/

#if (CONFIG_STACK_BSD || CONFIG_STACK_CYGWIN) && !defined(__netx_bsd_h__)
#define __netx_bsd_h__ 1

#include "acnip.h"
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

#if CONFIG_MULTI_NET
typedef struct sockaddr netx_addr_t;

#error not currently supported

#elif CONFIG_NET_IPV4
typedef struct sockaddr_in netx_addr_t;

/* operations performed on netx_addr_t */
#define netx_TYPE(addrp) (addrp)->sin_family
#define netx_PORT(addrp) (addrp)->sin_port
#define netx_INADDR(addrp) (addrp)->sin_addr.s_addr
#define netx_SINADDR(addrp) (addrp)->sin_addr

#define netx_INIT_ADDR_STATIC(inaddr, port) {AF_INET, (port), {inaddr}}
#define netx_INIT_ADDR(addrp, inaddr, port) ( \
      netx_TYPE(addrp) = AF_INET, \
      netx_INADDR(addrp) = (inaddr), \
      netx_PORT(addrp) = (port) \
   )

#define adhocIsValid(addrp) (netx_TYPE(addrp) == AF_INET)
#define SDT_TA_TYPE(addrp) (((addrp)->sin_family == AF_INET) ? SDT_ADDR_IPV4 : SDT_ADDR_NULL)
#define netx_is_multicast(addrp) is_multicast(netx_INADDR(addrp))

#elif CONFIG_NET_IPV6
typedef struct sockaddr_in6 netx_addr_t;


/* operations performed on netx_addr_t */
#define netx_TYPE(addrp) (addrp)->sin6_family
#define netx_PORT(addrp) (addrp)->sin6_port
#define netx_INADDR(addrp) (addrp)->sin6_addr.s6_addr

#define netx_INIT_ADDR_STATIC(inaddr, port) {.sin6_family = AF_INET6,\
                                             .sin6_port = (port),\
                                             .sin6_addr = {inaddr}}
#define netx_INIT_ADDR(addrp, inaddr, port) ( \
      netx_TYPE(addrp) = AF_INET6, \
      memcpy(netx_INADDR(addrp), (inaddr)), \
      netx_PORT(addrp) = (port) \
   )
#define adhocIsValid(addrp) (netx_TYPE(addrp) == AF_INET6)

#define SDT_TA_TYPE(addrp) (((addrp)->sin_family == AF_INET6) ? SDT_ADDR_IPV6 : SDT_ADDR_NULL)

#else
#error Unknown network configuration
#endif




/************************************************************************/
/*
There are fairly major differences if CONFIG_LOCALIP_ANY is true (which
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

#if CONFIG_LOCALIP_ANY

typedef port_t localaddr_t;

/* operations on localaddr_t */
#define netx_INIT_LOCALADDR(addrp, addr, port) (*addrp = (port))
#define netx_INIT_LOCALADDR_STATIC(addr, port) (port)
#define lclad_PORT(lclad) (lclad)
#define lclad_INADDR(lclad) netx_INADDR_ANY
#define lcladEq(a, b) ((a) == (b))

typedef ip4addr_t grouprx_t;

#define groupaddr(gprx) (gprx)
#define groupiface(gprx) INADDR_ANY

#else /* !CONFIG_LOCALIP_ANY */

typedef netx_addr_t localaddr_t;

/* operations on localaddr_t */
#define netx_INIT_LOCALADDR(addrp, addr, port) netx_INIT_ADDR(addrp, addr, port)
#define netx_INIT_LOCALADDR_STATIC(addr, port) netx_INIT_ADDR_STATIC(inaddr, port)
#define lclad_PORT(laddr) ((laddr).sin_port)
#define lclad_INADDR(laddr) ((laddr).sin_addr.s_addr)
#define lcladEq(a, b) (lclad_PORT(a) == lclad_PORT(b) && lclad_INADDR(a) == lclad_INADDR(b))

typedef struct grouprx_s {
   ip4addr_t group;
   ip4addr_t interface;
} grouprx_t;

#define groupaddr(gprx) ((gprx).group)
#define groupiface(gprx) ((gprx).interface)

#endif /* !CONFIG_LOCALIP_ANY */

/* operations on netxsock_t */
#define NSK_PORT(nskptr) netx_PORT(&(nskptr)->localaddr)
#define NSK_INADDR(nskptr) netx_INADDR(&(nskptr)->localaddr)

#if RECEIVE_DEST_ADDRESS
#define netx_PKTINFO_LEN CMSG_SPACE(sizeof(struct in_pktinfo))
#endif

/************************************************************************/
#if CONFIG_NET_IPV4
ip4addr_t netx_getmyip(netx_addr_t *destaddr);
ip4addr_t netx_getmyipmask(netx_addr_t *destaddr);
#endif /* CONFIG_NET_IPV4 */

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

#define new_txbuf(size)  acnAlloc(size)
#define free_txbuf(x, size)  acnDealloc(x, size)

#define netx_close(sock) close(sock)

#ifdef __cplusplus
}
#endif

#endif  /* #if (CONFIG_STACK_BSD || CONFIG_STACK_CYGWIN) && !defined(__netx_bsd_h__) */
