/************************************************************************/
/*
Copyright (c) 2010, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3s
*/
/************************************************************************/
/*
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

typedef struct rxcontext_s rxcontext_t;

struct rxcontext_s {
   struct netx_context_s {
      struct rxbuf_s     *rcvbuf;
#if CONFIG_NET_UDP
      netx_addr_t        source;
#if RECEIVE_DEST_ADDRESS
      uint8_t            pktinfo[netx_PKTINFO_LEN];
#endif
#endif
   } netx;
#if CONFIG_RLP
   struct rlp_context_s {
      struct rlpsocket_s *rlsk;
      const uint8_t      *srcCID;
      void               *handlerRef;
   } rlp;
#endif
#if CONFIG_SDT
   struct sdt1_context_s {
#if !CONFIG_SINGLE_COMPONENT
      struct Lcomponent_s *Lcomp;
#endif
      struct Rcomponent_s *Rcomp;
      uint8_t             *txbuf;
   } sdt1;
   struct sdtw_context_s {
      struct member_s     *memb;
      protocolID_t        protocol;
      uint16_t            assoc;
   } sdtw;
#endif
#if CONFIG_NET_TCP
	struct tcp_context_s {
		struct tcpconnect_s *cxn;
	} tcp;
#endif
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
