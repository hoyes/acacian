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
header: rxcontext.h

Received data context.

Receipt of a packet consists of a series of functions which typically
analyse header information, then pass the remaining contents - or part
of them on to a sub-function at the next layer up the protocol stack.
Certain information e.g. source address, is relevant higher up the
layers and therefore usually gets passed from function to function as an
argument and there is a tendency to pass ever more function arguments
into subsequent handlers to provide context for those higher layers.

Rather than do this, we can accumulate context information on the data
block being processed in a context structure. At each layer, the
information in this structure is only valid for information coming up
from the layers below, only whilst nested processing is taking place,
and only for items not modified by layers above. - it is not safe to
save pointers to this structure and try and dereference them later!

There is currently just one global instance of this structure so it is
not thread safe. However, if implementing a multi-threaded version (e.g.
one thread per socket) it would be easy enough to have one structure per
thread and passing just one pointer to this structure from function to
function will still be more efficient.

The down side of this strategy is the necessity to include headers and
provide access to data from widely separated layers which provides
opportunities for sloppy and careless programming which must be avoided.
*/

#ifndef __rxcontext_h__
#define __rxcontext_h__ 1
struct netx_context_s {
	struct rxbuf_s     *rxbuf;
	netx_addr_t        source;
#if RECEIVE_DEST_ADDRESS
	uint8_t            pktinfo[netx_PKTINFO_LEN];
#endif
};

struct rxcontext_s {
	struct netx_context_s netx;
#if CF_RLP
	struct rlp_context_s {
		struct rlpsocket_s *rlsk;
		const uint8_t      *srcCID;
		void               *handlerRef;
	} rlp;
#endif
#if CF_MULTI_COMPONENT
	struct Lcomponent_s *Lcomp;
#endif
	struct Rcomponent_s *Rcomp;
	/*
#if CF_SDT
	struct sdt1_context_s {
		uint8_t             *txbuf;
	} sdt1;
	struct sdtw_context_s {
		struct member_s     *memb;
		protocolID_t        protocol;
		uint16_t            assoc;
	} sdtw;
#endif
	*/
};

/*
Context levels - this should be set immediately before calling the next
level up, allowing a handler to discern its context if it is called from
different levels. See sdtBlockRx() for an example.
Note that in cases where calls are absolutely determined (e.g. between
levels within SDT), it may not be necessary to set this. Also, the level
is not reset on the way back down the stack.
*/
#endif  /* __rxcontext_h__ */
