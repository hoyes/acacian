/*
#tabs=3t
*/
/**********************************************************************/
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

*/
/**********************************************************************/

#ifndef __netxface_h__
#define __netxface_h__ 1

/**********************************************************************/
/*
header: netxface.h

Interface to OS and stack dependent networking
*/
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

extern void rlpnetxRx(uint32_t evf, void *evptr);

int netxGetMyAddr(struct rlpsocket_s *rs, netx_addr_t *addr);

#endif  /* #ifndef __netxface_h__ */
