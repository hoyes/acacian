/************************************************************************/
/*
Copyright (c) 2010, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3s
*/
/************************************************************************/
/*
Implementation of ACN route layer protocol

API overview
--

Initialization
--
extern int rlp_init(void);

Call once before using RLP - in ACN typcally called by the equivalent
function in the next layer up (SDT, E1.31) rather than directly by the
application. No harm in calling multiple times.

Sending
--
int rlp_sendbuf(
            uint8_t *txbuf,
            int length, 
            PROTO_ARG 
            rlpsocket_t *src, 
            netx_addr_t *dest, 
            cid_t srccid);

rlp_sendbuf() expects a buffer with exactly RLP_OFS_PDU1DATA octets of
data unused at the beginning for RLP to put its headers. Length if the
total length of the buffer including these octets.

PROTO_ARG is the protocol ID of the outermost protocol in the buffer
(SDT, E1.31 etc.). However, if acn is built with
CONFIG_RLP_SINGLE_CLIENT set, then  PROTO_ARG evaluates to nothing, one
less argument is poassed and  the configured single client protocol is
used.

src is the RLP socket to send the data from (which determines the source
address), dest is the destination address and srccid is the component ID
of the sender.

Receiving
--
Note some functions decribed here are defined in netxface.c To send or
receive from RLP you need at least one RLP socket.

rlpsocket_t *rlpSubscribe(
               netx_addr_t *lclad, 
               protocolID_t protocol, 
               rlpcallback_fn *callback, 
               void *ref);

typedef void rlpcallback_fn(
               const uint8_t *data, 
               int datasize, 
               struct rxcontext_s *rcxt);

int rlpUnsubscribe(rlpsocket_t *rs, 
               netx_addr_t *lclad, 
               protocolID_t protocol);

Each separate incoming address and port, whether unicast or multicast
requires a separate call to rlpSubscribe(). Note though that the handle
returned may well not be unique, depending on lower layer implementation
details. In general, for a given port, there may be just one rlpsocket,
or one for each separate multicast address, or some intermediate number.
For a shared rlpsocket, if callback and ref do not match values from
previous calls for the protocol, the call will fail. So you should
assume one callback and reference for each port used.

At present unicast packets are received via any interface and multicast
via the defaault chosen by the stack, so any unicast address supplied
is treated as INADDR_ANY (or its equivalent for other protocols).

If lclad is NULL, the system will assign a new unicast rlpsocket bound
to an ephemeral port.

If lclad is supplied with a port value of netx_PORT_EPHEM, then a new
ephemeral port is assigned and the actual value is overwritten in lclad.

When using rlpsockets for sending, they determine the source address and
port used in the packet. Because the source must be unicast, even in
implementations where multiple rlpsockets are returned for differing
multicast subscriptions, any of thes may be used for sending with the
same effect. However, all unsubscribe calls must use the same rlpsocket
that was returned by subscribe.

When a packet is received at an rlpsocket, the given callback function
is invoked with the contents of the RLP PDU block. The reference pointer
passed to rlpSubscribe() is returned in the handlerRef field of the
rcxt structure which is valid for all netx_s and rlp_s fields:

struct rxcontext_s {
   struct netx_context_s {
      netx_addr_t        source;
      struct rxbuf_s     *rcvbuf;
   } netx;
   struct rlp_context_s {
      struct rlpsocket_s *rlsk;
      const uint8_t      *srcCID;
      void               *handlerRef;
   } rlp;
   ...
};
*/
/************************************************************************/

#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "acncommon.h"
#include "acnstd/rlp.h"
#include "acnstd/protocols.h"
#include "uuid.h"
#include "marshal.h"

#include "acnlog.h"
#include "netxface.h"
#include "component.h"
#include "rxcontext.h"
#include "rlp.h"

#include "acnmem.h"

static const uint8_t rlpPreamble[RLP_PREAMBLE_LENGTH] = RLP_PREAMBLE_VALUE;

/************************************************************************/
#define lgFCTY LOG_RLP
/************************************************************************/
/*
Transmit buffer management - this has been simplified by observing that
there are few if any realistic use-cases for packing multiple PDUs at
the root layer We therefore do not support this on send though (we still
correctly receive them).

rlp_sendbuf() finalizes the buffer and transmits it
*/

#define RLP_OFS_LENFLG  RLP_PREAMBLE_LENGTH
#define RLP_OFS_PROTO   (RLP_PREAMBLE_LENGTH + 2)
#define RLP_OFS_SRCCID  (RLP_OFS_PROTO + 4)

#if CONFIG_RLP_SINGLE_CLIENT
#define PROTO CONFIG_RLP_CLIENTPROTO
#else
#define PROTO protocol
#endif

int
rlp_sendbuf(
   uint8_t *txbuf,
   int length,
   if_RLP_MANYCLIENT(protocolID_t protocol,)
   rlpsocket_t *src,
   netx_addr_t *dest,
   cid_t srccid
)
{
   uint8_t *bp;
   int rslt;

   LOG_FSTART(lgFCTY);
   bp = marshalBytes(txbuf, rlpPreamble, RLP_PREAMBLE_LENGTH);
   bp = marshalU16(bp, length + FIRST_FLAGS - RLP_OFS_LENFLG);
   bp = marshalU32(bp, PROTO);
   bp = marshalCID(txbuf + RLP_OFS_SRCCID, srccid);

/*
#if (acntestlog(lgDBUG))
   char cidstr[CID_STR_SIZE];
#endif
   acnlogmark(lgDBUG, "length %d, src %s port %d, dest %s:%d", length,
               cidToText(srccid, cidstr),
               ntohs(src->port),
               inet_ntoa(dest->sin_addr),
               ntohs(dest->sin_port));
*/
   rslt = netx_send_to(src->sk, dest, txbuf, length);
   LOG_FEND(lgFCTY);
   return rslt;
}
#undef PROTO

/************************************************************************/

void
rlp_packetRx(const uint8_t *buf, ptrdiff_t length, rxcontext_t *rcxt)
{
   const uint8_t *pdup;
   uint8_t flags;
   protocolID_t INITIALIZED(vector);
   const uint8_t *datap = NULL;
   int INITIALIZED(datasize);
   const uint8_t *pp;
   struct rlphandler_s *hp, *ep;

   LOG_FSTART(lgFCTY);
/*
   acnlogmark(lgDBUG, "RLP packet from %s:%d",
               inet_ntoa(rcxt->netx.source.sin_addr),
               ntohs(rcxt->netx.source.sin_port));
*/

   if(length < (int)(RLP_OFS_PDU1DATA + RLP_POSTAMBLE_LENGTH)) {
      acnlogmark(lgINFO, "Packet too short to be valid");
      return;
   }

   /* Check and strip off EPI 17 preamble  */
   if(memcmp(buf, rlpPreamble, RLP_PREAMBLE_LENGTH) != 0) {
      acnlogmark(lgINFO, "Invalid Preamble");
      return;
   }

   pdup = buf + RLP_PREAMBLE_LENGTH;

   /* first PDU must have all fields */
   if ((*pdup & (FLAG_bMASK)) != FIRST_bFLAGS) {
      acnlogmark(lgERR, "Illegal first PDU flags");
      return;
   }

   /* pdup points to start of PDU */
   while (pdup != buf + length)
   {
      flags = *pdup;
      pp = pdup + 2;
      pdup += getpdulen(pdup);  /* pdup now points to end */
      if (pdup > buf + length)  { /* sanity check */
         acnlogmark(lgERR, "Packet length error");
         break;
      }
      if (flags & LENGTH_bFLAG) {
         acnlogmark(lgERR, "Length flag set");
         break;
      }
      if (flags & VECTOR_bFLAG) {
         vector = unmarshalU32(pp); /* get protocol type */
         pp += sizeof(uint32_t);
      }
      if (flags & HEADER_bFLAG) {
         rcxt->rlp.srcCID = pp; /* get pointer to source CID */
         pp += sizeof(cid_t);
      }
      if (pp > pdup) {/* if there is no following PDU in the message */
         acnlogmark(lgERR, "PDU length error");
         break;
      }
      if (flags & DATA_bFLAG)  {
         datap = pp; /* get pointer to start of the PDU */
         datasize = pdup - pp; /* get size of the PDU */
      }
      for (hp = rcxt->rlp.rlsk->handlers, ep = hp + MAX_RLP_CLIENT_PROTOCOLS; hp < ep; ++hp)
         if (hp->protocol == vector && hp->func != NULL) {
            rcxt->rlp.handlerRef = hp->ref;
            (*hp->func)(datap, datasize, rcxt);
            break;
         }
   }
   LOG_FEND(lgFCTY);
}

/************************************************************************/
/*
Initialize RLP (if not already done)
*/
int
rlp_init(void)
{
   int rslt;
   static bool initialized = 0;

   LOG_FSTART(lgFCTY);
   if (initialized) {
      acnlogmark(lgDBUG,"already initialized");
      return 0;
   }

   if ((rslt = netx_init()) == 0)
      initialized = 1;
   LOG_FEND(lgFCTY);
   return rslt;
}

