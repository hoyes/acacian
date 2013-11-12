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

header: netxface.h

Interface to OS and stack dependent networking
*/

#ifndef __netxface_h__
#define __netxface_h__ 1

#include "netx_bsd.h"

/**********************************************************************/
/*
struct: rxbuf_s

At minimum, the rxbuffer needs a usecount attaching. There are several
possibilities.

	o Define a structure with the usecount ahead of the data. This 
	allows a size field too and variable size data blocks. However, 
	it does not work in an OS/Stack where the raw packet is used and 
	the space ahead of the data is filled with UDP and IP headers.

	o As previous, but in the case of a stack supplying a raw 
	packet, the UDP/IP headers get overwritten by the usecount. Only 
	works if the stack will tolerate this.
	
	o Place the usecount after the end of the data. This limits the data
	buffer to a fixed size so we know where the usecount is.
	
	o Use a separate structure supplying the usecount and a pointer to
	the data buffer. This adds the overhead of memory management for a
	very small structure. In many impolementations, the pointer to the
	buffer is bigger than the whole of the rest of the structure.

The definition here assumes the first.
*/

typedef struct rxbuf_s rxbuf_t;

struct rxcontext_s;
struct rlpsocket_s;

typedef void rlpcallback_fn(const uint8_t *data, int datasize, 
								struct rxcontext_s *rcxt);

struct rxbuf_s {
	uint16_t usecount;
	uint8_t data[MAX_MTU];
};

static inline void
releaseRxbuf(struct rxbuf_s *rxbuf)
{
	if (--rxbuf->usecount <= 0) {
		free(rxbuf);
	}
}

#define getRxdata(rxbufp) (rxbufp)->data
#define getRxBsize(rxbufp) MAX_MTU

/**********************************************************************/
/*
function prototypes for netxface.c
*/
extern int netx_init(void);
extern nativesocket_t netx_open(localaddr_t *localaddr);

extern int netx_add_group(nativesocket_t sock, 
							grouprx_t group, bool initMcast);

extern int netx_drop_group(nativesocket_t sock, 
							grouprx_t group, bool clearMcast);

extern int netx_send_to(nativesocket_t sock, 
							const netx_addr_t *destaddr, 
							void  *pkt, ptrdiff_t datalen);

extern struct rlpsocket_s *rlpSubscribe(netx_addr_t *lclad, 
							protocolID_t protocol, 
							rlpcallback_fn *callback, void *ref);

extern int rlpUnsubscribe(struct rlpsocket_s *rs, netx_addr_t *lclad, 
							protocolID_t protocol);

extern void udpnetxRx(uint32_t evf, void *evptr);

int netxGetMyAddr(struct rlpsocket_s *rs, netx_addr_t *addr);

#include "getip.h"

#endif  /* #ifndef __netxface_h__ */
