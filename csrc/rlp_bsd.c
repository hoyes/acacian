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
file: rlp_bsd.c

Implementation of ACN route layer protocol on BSD sockets API including
Operating System and Stack dependent networking functions.

about: Network Strategies

Notes on use of interfaces, ports and multicast addressing (group
addressing).

This explanation is relevant to configuration options ACNCFG_LOCALIP_ANY
and RECEIVE_DEST_ADDRESS, The following applies to UDP on IPv4. UDP
on IPv6 will be similar, but transports other than UDP may be very
different.

IP addresses and interfaces:

A socket represents an endpoint and we bind it to an "interface" and
port. The interface is represented by  an IP address, but this can be
misleading because of the way multicast addressing works.

When a packet is received (incoming packet), the stack identifies the
"interface" (and therefore the socket to receive the packet), by the
incoming destination address in the packet. However, when this address
is a multicast address, there is no interface information there.
Therefore, simply binding a socket to a specific unicast IP address is
not helpful.

The exception is the case where there are multiple physical interfaces 
on separate networks which may be within different multicast zones. In
this case, the stack uses rules based on its routing tables.

For this reason, it is usually best to open any socket intended to
receive multicast using IN_ADDR_ANY as the interface and rely on the
stack to worry about interfaces. So while this API allows for sockets
to be bound to specific interfaces (IP addresses), there is no guarantee
that doing so will prevent multicast traffic from other networks
arriving at that socket.

Ports:

A socket (in this code) is always bound to a specific port; if bind is
called with netx_PORT_EPHEM then the stack will pick an "ephemeral" port
which is different from all others currently in use. Because of the
issues with multicasting above, and because there is no way to
coordinate the multiple receivers of a multicast group to use a
particular ephemeral port, the same port and therefore the same
socket, is used for all multicast reception (note a multicast packet can
be *transmitted* from any socket because it is only the destination
address which can be multicast). For example in SDT, all multicast traffic
uses the SDT_MULTICAST_PORT as the estination port.

Socket allocation for multicast reception:

Because all multicast traffic takes place on the same port, it could
theoretically use a single socket. However, the Linux stack like many 
others restricts
the number of multicast subscriptions that can be applied to a 
single socket. This code opens dummy sockets as necessary to increase 
the number of multicast subscriptions. As subscriptions are handled at 
host level and not at the individual socket level, this has the same 
effect as subscribing the original socket to the group.

Multicast group address in incoming packets:

To allow RLP to distinguish traffic for different multicast groups
Each time a higher layer registers a new listener corresponding to a
specific multicast address, we know that that listener is only concerned
with the traffic within that particular group. If the stack can provide
the incoming multicast address (the destination address from an incoming
packet we can filter) in the rlp layer and only send that packet to the
appropriate listener. Otherwise all listeners receive all multicast
packets, irrespective of their group address. They can eliminate all the
unwanted packets eventually but may need to process a considerable
amount of the packet in doing so.

The code here, does not filter incoming packets by destination address,
but if it can extract that information (and RECEIVE_DEST_ADDRESS is true)
then we do so and pass the group address on to the higher layer incoming
packet handler. NOTE: We only need to pass the group address, not a full
address and port structure, because the port (and interface if desired
subject to the limitations above) are determined by the socket.

topic: Receiving

To send or receive from RLP you need at least one RLP socket.

Each separate incoming address and port, whether unicast or multicast
requires a separate call to rlpSubscribe(). Note though that the handle
returned may well not be unique, depending on lower layer implementation
details. In general, for a given port, there may be just one rlpsocket,
or one for each separate multicast address, or some intermediate number.
For a shared rlpsocket, if callback and ref do not match values from
previous calls for the protocol, the call will fail. So you should
assume one callback and reference for each port used.

At present unicast packets are received via any interface and multicast
via the default chosen by the stack, so any unicast address supplied
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

|  struct rxcontext_s {
|  	struct netx_context_s {
|  		netx_addr_t        source;
|  		struct rxbuf_s     *rxbuf;
|  	} netx;
|  	struct rlp_context_s {
|  		struct rlpsocket_s *rlsk;
|  		const uint8_t      *srcCID;
|  		void               *handlerRef;
|  	} rlp;
|  	...
|  };

*/
/**********************************************************************/

#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/epoll.h>
#include <net/if.h>
#include <assert.h>


#include "acn.h"

/**********************************************************************/
#define lgFCTY LOG_RLP
/**********************************************************************/

static const uint8_t rlpPreamble[RLP_PREAMBLE_LENGTH] = RLP_PREAMBLE_VALUE;

/**********************************************************************/
/*
constants: Integer constants used for pass-by-reference values

int_zero - integer with value 0
int_one - integer with value 1
*/
const int int_zero = 0;
const int int_one = 1;

/**********************************************************************/
/*
pointer: rlpsocks [static]

A list of current RLP sockets.
*/
static struct rlpsocket_s *rlpsocks = NULL;

/**********************************************************************/
/*
topic: Transmit buffer management

This has been simplified by observing that there are few if any 
realistic use-cases for packing multiple PDUs at the root layer We 
therefore do not support this on send (though we still correctly 
receive them).

*/

#define RLP_OFS_LENFLG  RLP_PREAMBLE_LENGTH
#define RLP_OFS_PROTO   (RLP_PREAMBLE_LENGTH + 2)
#define RLP_OFS_SRCCID  (RLP_OFS_PROTO + 4)

#if ACNCFG_RLP_MAX_CLIENT_PROTOCOLS == 1
#define PROTO ACNCFG_RLP_CLIENTPROTO
#else
#define PROTO protocol
#endif

/**********************************************************************/
/*
func: rlp_init

Initialize RLP and networking.
Need only be called once -
subsequent calls do nothing (but do no harm).
*/
int
rlp_init(void)
{
	static bool initialized = 0;
	struct sigaction sig;

	LOG_FSTART();
	if (initialized) {
		acnlogmark(lgDBUG,"already initialized");
		return 0;
	}
	randomize(false);
	if (evl_init() < 0) return -1;

	memset(&sig, 0, sizeof(sig));
	sig.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &sig, NULL) < 0) {
		acnlogerror(lgWARN);
	}
	if (sigaction(SIGHUP, &sig, NULL) < 0) {
		acnlogerror(lgWARN);
	}
	/* don't process twice */
	initialized = 1;
	LOG_FEND();
	return 0;
}

/**********************************************************************/
/*
topic: Sending
*/
/**********************************************************************/
/*
func: netx_send_to

Send UDP message to given address.
The call returns the number of characters sent, or negitive if an error occurred.
*/
int netx_send_to(
	nativesocket_t     sk,    /* contains a flag if port is open and the local port number */
	const netx_addr_t *destaddr,   /* contains dest port and ip numbers */
	void              *pkt,        /* pointer data packet if type UPDPacket (return from netx_new_txbuf()) */
	ptrdiff_t            datalen     /* length of data */
)
{
	LOG_FSTART();

	assert(sk >= 0);
	assert(pkt);

#if defined(RANDOMDROP)
	if ((random() % RANDOMDROP) == 0) {
		acnlog(lgINFO, "drop");
		LOG_FEND();
		return datalen;
	}
#endif

	if ((datalen = sendto(sk, (char *)pkt, datalen, 0, (struct sockaddr *)destaddr, 
								sizeof(netx_addr_t))) < 0) {
		acnlogmark(lgERR, "return %" PRIdPTR ", errno %d %s", datalen, errno, strerror(errno));
	}

	LOG_FEND();
	return datalen;
}

#if ACNCFG_RLP
extern void rlp_packetRx(const uint8_t *buf, ptrdiff_t length, struct rxcontext_s *rcxt);
#endif
/**********************************************************************/
/*
func: rlp_sendbuf

Finalizes a buffer and transmit it

txbuf - a buffer with exactly RLP_OFS_PDU1DATA octets of
data unused at the beginning for RLP to put its headers.
length - the total length of the buffer including these octets.
protocol - the protocol ID of the outermost protocol in the buffer 
(SDT, E1.31 etc.). *If* acn is built with ACNCFG_RLP_CLIENTPROTO 
set, then this argument must be omitted and  the configured single 
client protocol is used.
src - the RLP socket to send the data from (which determines the source
address).
dest - the destination address and srccid is the component ID
of the sender.

*/
int
rlp_sendbuf(
	uint8_t *txbuf,
	int length,
#if ACNCFG_RLP_MAX_CLIENT_PROTOCOLS > 1
	protocolID_t protocol,
#endif
	struct rlpsocket_s *src,
	netx_addr_t *dest,
	uint8_t *srccid
)
{
	uint8_t *bp;
	int rslt;

	LOG_FSTART();
	bp = marshalBytes(txbuf, rlpPreamble, RLP_PREAMBLE_LENGTH);
	bp = marshalU16(bp, length + FIRST_FLAGS - RLP_OFS_LENFLG);
	bp = marshalU32(bp, PROTO);
	bp = marshaluuid(txbuf + RLP_OFS_SRCCID, srccid);

	rslt = netx_send_to(src->sk, dest, txbuf, length);

	LOG_FEND();
	return rslt;
}
#undef PROTO

/**********************************************************************/
/*
findrlsk

Search for the rlpsocket_s which uses port
*/
static struct rlpsocket_s *
findrlsk(port_t port)
{
	struct rlpsocket_s *rs;
	
	LOG_FSTART();
	for (rs = rlpsocks; rs; rs = rs->lnk.r) {
		if (rs->port == port) break;
	}
	LOG_FEND();
	return rs;
}

/**********************************************************************/
/*
newSkt
*/
static int
newSkt(port_t port, bool reuse)
{
	int sk;
	netx_addr_t addr;

	LOG_FSTART();
	if ((sk = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		acnlogerror(lgERR);
		return -1;
	}
	addr.sin_family = netx_FAMILY;
	addr.sin_port = port;
	addr.sin_addr.s_addr = INADDR_ANY;

	if ((reuse && setsockopt(sk, SOL_SOCKET, SO_REUSEADDR,
										&int_one, sizeof(int_one)) < 0)
		|| bind(sk, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		acnlogerror(lgERR);
		close(sk);
		return -1;
	}
#if acntestlog(lgDBUG)
	{
		netx_addr_t naddr;
		unsigned int size = sizeof(naddr);

		getsockname(sk, (struct sockaddr *)&naddr, &size);
		acnlogmark(lgDBUG, "new socket %s:%hu", 
			inet_ntoa(netx_SINADDR(&naddr)), ntohs(netx_PORT(&naddr)));
	}
#endif
	LOG_FEND();
	return sk;
}

/**********************************************************************/
static struct sockaddr_in dummysa = {
	.sin_family = netx_FAMILY,
	.sin_port = 0,
	.sin_addr = {INADDR_ANY}
};

#define dummyport (dummysa.sin_port)

/**********************************************************************/
/*
newDummy
*/
static int
newDummy()
{
	int sk;
	unsigned int len = sizeof(dummysa);

	LOG_FSTART();
	if ((sk = newSkt(dummyport, true)) >= 0
		&& dummyport == 0
		&& getsockname(sk, (struct sockaddr *)&dummysa, &len) < 0)
	{
		close(sk);
		return -1;
	}
	LOG_FEND();
	return sk;
}

/**********************************************************************/
/*

*/
static struct skgroups_s *
findgroup(struct rlpsocket_s *rs, ip4addr_t group, int *ix)
{
	int i;
	struct skgroups_s *sgp;

	LOG_FSTART();
	for (sgp = rs->groups; sgp; sgp = sgp->lnk.r) {
		for (i = 0; i < IP_MAX_MEMBERSHIPS; ++i) {
			if (sgp->mad[i] == group) {
				if (ix) *ix = i;
				return sgp;
			}
		}
	}
	LOG_FEND();
	return NULL;
}

/**********************************************************************/
/*

*/
static struct skgroups_s *
findblankgp(struct rlpsocket_s *rs, int *ix)
{
	int i;
	struct skgroups_s *sgp;

	LOG_FSTART();
	for (sgp = rs->groups; ; sgp = sgp->lnk.r) {
		if (sgp == NULL) return sgp;
		if (sgp->ngp < IP_MAX_MEMBERSHIPS) {
			for (i = 0; i < IP_MAX_MEMBERSHIPS; ++i) {
				if (sgp->mad[i] == 0) {
					if (ix) *ix = i;
					return sgp;
				}
			}
			assert(0);  /* shouldn't get here */
		}
	}
	LOG_FEND();
}

/**********************************************************************/
/*

*/
static int
addgroup(struct rlpsocket_s *rs, ip4addr_t group)
{
	struct skgroups_s *sgp;
	struct ip_mreqn mreq;
	int i;

	LOG_FSTART();
	i = 0;
	if ((sgp = findgroup(rs, group, &i))) {
		/* already a member */
		sgp->nm[i] += 1;
		return 0;
	}

	if ((sgp = findblankgp(rs, &i)) == NULL) {
		sgp = acnNew(struct skgroups_s);
		if (rs->groups == NULL) sgp->sk = rs->sk; /* first one gets the main socket */
		else {
			if ((sgp->sk = newDummy()) < 0) {
				free(sgp);
				return -1;
			}
		}
		i = 0;
	}

	memset(&mreq, 0, sizeof(mreq));
	mreq.imr_address.s_addr = INADDR_ANY;
	mreq.imr_multiaddr.s_addr = group;
	if (setsockopt(sgp->sk, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
		if (sgp->ngp == 0) {
			close(sgp->sk);
			free(sgp);
		}
		return -1;
	}

	sgp->mad[i] = group;
	sgp->nm[i] = 1;
	if (sgp->ngp++ == 0) slAddHead(rs->groups, sgp, lnk);
	LOG_FEND();
	return 0;
}

/**********************************************************************/
/*

*/
static int
dropgroup(struct rlpsocket_s *rs, ip4addr_t group)
{
	struct skgroups_s *sgp;
	int i;
	int rslt;
	
	LOG_FSTART();
	if ((sgp = findgroup(rs, group, &i)) == NULL) {
		errno = EFAULT;
		return -1;
	}
	
	if (--(sgp->nm[i]) == 0) {
		struct ip_mreqn mreq;

		memset(&mreq, 0, sizeof(mreq));
		mreq.imr_address.s_addr = INADDR_ANY;
		mreq.imr_multiaddr.s_addr = group;

		if ((rslt = setsockopt(sgp->sk, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq))) < 0) {
			acnlogerror(lgERR);
		}
		sgp->mad[i] = 0;
		if (--sgp->ngp == 0) {
			slUnlink(struct skgroups_s, rs->groups, sgp, lnk);
			if (sgp->sk != rs->sk) close(sgp->sk);
			free(sgp);
		}
	}
	LOG_FEND();
	return 0;
}

/**********************************************************************/
/*
func: rlpSubscribe

Returns an rlpsocket_s suitable for sending and registers a callback function
for receiving packets.

lclad is the Local address to use (the structure may be used to send to any
remote address). lclad may be a multicast address, in which case this address
is added to the subscribed multicast addresses.

The rlpsocket_s returned may not be unique and in the case of multicast 
addresses will probably not be. The local address should be relinquished 
by calling
rlpunsubscribe when it is no longer needed.
*/
struct rlpsocket_s *
rlpSubscribe(
	netx_addr_t *lclad, 
	protocolID_t protocol, 
	rlpcallback_fn *callback, 
	void *ref
)
{
	struct rlpsocket_s *rs;
	port_t port;
	ip4addr_t addr;
	int p;

	LOG_FSTART();
#if ACNCFG_RLP_MAX_CLIENT_PROTOCOLS == 1
	assert(protocol == ACNCFG_RLP_CLIENTPROTO);
#endif

/*
FIXME: This overrides interface selection and forces INADDR_ANY
Address is only observed if it is multicast
*/
	if (lclad) {
		port = netx_PORT(lclad);
		addr = netx_INADDR(lclad);
		if (port == netx_PORT_EPHEM && addr != INADDR_ANY) {
			errno = EINVAL;
			return NULL;
		}
		if (!is_multicast(addr)) addr = INADDR_ANY;
	} else {
		port = netx_PORT_EPHEM;
		addr = INADDR_ANY;
	}
#if acntestlog(lgDBUG)
	{
		struct in_addr inaddr;
		
		inaddr.s_addr = addr;
		acnlogmark(lgDBUG, "subscribe %s:%d", inet_ntoa(inaddr), ntohs(port));
	}
#endif
	if (port == netx_PORT_EPHEM || (rs = findrlsk(port)) == NULL) {
		rs = acnNew(struct rlpsocket_s);

		if ((rs->sk = newSkt(port, is_multicast(addr))) < 0) {
			free(rs);
			return NULL;
		}
		if (port == netx_PORT_EPHEM) {
			netx_addr_t naddr;
			unsigned int size = sizeof(naddr);

			getsockname(rs->sk, (struct sockaddr *)&naddr, &size);
			port = netx_PORT(&naddr);
			if (lclad) memcpy(lclad, &naddr, sizeof(netx_addr_t));
		}
		rs->port = port;

		rs->rxfn = &udpnetxRx;
		if (evl_register(rs->sk, &rs->rxfn, EPOLLIN) < 0) {
			acnlogerror(lgERR);
			close(rs->sk);
			return NULL;
		}

		p = -1;      
	} else {
#if ACNCFG_RLP_MAX_CLIENT_PROTOCOLS > 1
		int f;

		for (p = 0, f = -1;;) {
			if (rs->handlers[p].protocol == protocol) {
				if (rs->handlers[p].func != callback || rs->handlers[p].ref != ref) {
					errno = EADDRINUSE;
					return NULL;
				}
				break;
			}
			if (rs->handlers[p].protocol == 0) f = p;
			if (++p >= ACNCFG_RLP_MAX_CLIENT_PROTOCOLS) {
				if ((p = f) < 0) {
					errno = ENOPROTOOPT;
					return NULL;
				}
				break;
			}
		}
#else
		p = 0;
		if (rs->handlers[0].func != callback || rs->handlers[p].ref != ref) {
			errno = EADDRINUSE;
			return NULL;
		}
#endif
	}

	if (is_multicast(addr) && addgroup(rs, addr) < 0) {
		if (p < 0) {
			close(rs->sk);
			free(rs);
		}
		return NULL;
	}
	if (p < 0) {
		p = 0;
		slAddHead(rlpsocks, rs, lnk);
	}
#if ACNCFG_RLP_MAX_CLIENT_PROTOCOLS > 1
	if (rs->handlers[p].protocol == 0) {
		rs->handlers[p].protocol = protocol;
#endif
		rs->handlers[p].func = callback;
		rs->handlers[p].ref = ref;
		rs->handlers[p].nsubs = 0;
#if ACNCFG_RLP_MAX_CLIENT_PROTOCOLS > 1
	}
#endif
	++rs->handlers[p].nsubs;
	LOG_FEND();
	return rs;
}

/**********************************************************************/
int
rlpUnsubscribe(struct rlpsocket_s *rs, netx_addr_t *lclad, protocolID_t protocol)
{
	port_t port;
	ip4addr_t addr;
	int p;
	int rslt;

	LOG_FSTART();
#if ACNCFG_RLP_MAX_CLIENT_PROTOCOLS == 1
	assert(protocol == ACNCFG_RLP_CLIENTPROTO);
#endif
	if (lclad) {
		port = netx_PORT(lclad);
		if (port != netx_PORT_EPHEM && port != rs->port) {
			errno = EINVAL;
			return -1;
		}
		addr = netx_INADDR(lclad);
	} else {
		port = rs->port;
		addr = netx_GROUP_UNICAST;
	}

#if ACNCFG_RLP_MAX_CLIENT_PROTOCOLS > 1
	for (p = 0; rs->handlers[p].protocol != protocol;) {
		if (++p >= ACNCFG_RLP_MAX_CLIENT_PROTOCOLS) {
			errno = EINVAL;
			return -1;
		}
	}
#else
	p = 0;
#endif

	if (is_multicast(addr)
		&& (rslt = dropgroup(rs, addr)) < 0) return rslt;
	
	if (--rs->handlers[p].nsubs == 0) {
#if ACNCFG_RLP_MAX_CLIENT_PROTOCOLS > 1
		rs->handlers[p].protocol = 0;
		for (p = 0; rs->handlers[p].nsubs == 0; ) {
			if (++p >= ACNCFG_RLP_MAX_CLIENT_PROTOCOLS) {
#endif
				assert(rs->groups == NULL);
				slUnlink(struct rlpsocket_s, rlpsocks, rs, lnk);
				evl_register(rs->sk, NULL, 0);
				close(rs->sk);
				free(rs);
#if ACNCFG_RLP_MAX_CLIENT_PROTOCOLS > 1
				break;
			}
		}
#endif
	}
	LOG_FEND();
	return 0;
}

/**********************************************************************/
#if ACNCFG_JOIN_TX_GROUPS && 0
nativesocket_t
netxTxgroupJoin(netx_addr_t *addr)
{
	LOG_FSTART();
	if (Lchan->membercount == 0 && !unicastLchan(Lchan)) {
		/* need to add group although we are not receiving for IGMP reasons */
		rlp_addGroup(membersock, netx_INADDR(&Lchan->outwd_ad));
	}
	
	LOG_FEND();
}
#endif


/**********************************************************************/
/*
func: rlp_packetRx
*/
void
rlp_packetRx(const uint8_t *buf, ptrdiff_t length, struct rxcontext_s *rcxt)
{
	const uint8_t *pdup;
	uint8_t flags;
	protocolID_t INITIALIZED(vector);
	const uint8_t *datap = NULL;
	int INITIALIZED(datasize);
	const uint8_t *pp;
	struct rlphandler_s *hp;
#if ACNCFG_RLP_MAX_CLIENT_PROTOCOLS > 1
	struct rlphandler_s *ep;
#endif
	LOG_FSTART();
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
			pp += UUID_SIZE;
		}
		if (pp > pdup) {/* if there is no following PDU in the message */
			acnlogmark(lgERR, "PDU length error");
			break;
		}
		if (flags & DATA_bFLAG)  {
			datap = pp; /* get pointer to start of the PDU */
			datasize = pdup - pp; /* get size of the PDU */
		}
#if ACNCFG_RLP_MAX_CLIENT_PROTOCOLS > 1
		for (hp = rcxt->rlp.rlsk->handlers, ep = hp + ACNCFG_RLP_MAX_CLIENT_PROTOCOLS; hp < ep; ++hp)
			if (hp->protocol == vector)
#else
		hp = rcxt->rlp.rlsk->handlers;
		if (vector == ACNCFG_RLP_CLIENTPROTO)
#endif
		{
			if (hp->func != NULL) {
				rcxt->rlp.handlerRef = hp->ref;
				(*hp->func)(datap, datasize, rcxt);
				break;
			}
		}
	}
	LOG_FEND();
}

/**********************************************************************/
static inline struct rxbuf_s*
newRxbuf()
{
	 return acnNew(struct rxbuf_s);
}
/**********************************************************************/
static struct rxbuf_s *rxbuf = NULL;

/**********************************************************************/
void
udpnetxRx(uint32_t evf, void *evptr)
{
	socklen_t addrLen;
	ssize_t length;
	struct rlpsocket_s *rlsk;
	struct rxcontext_s rcxt;
	
	LOG_FSTART();
	rlsk = container_of(evptr, struct rlpsocket_s, rxfn);
	addrLen = sizeof(rcxt.netx.source);
	if (evf != EPOLLIN) {
		acnlogmark(lgERR, "Poll returned 0x%08x", evf);
		return;
	}

	if (rxbuf == NULL
			&& (rxbuf = newRxbuf()) == NULL)
	{
		acnlogmark(lgERR, "can't get receive buffer");
		return;
	}
	rxbuf->usecount++;
	length = recvfrom(rlsk->sk, getRxdata(rxbuf), 
							getRxBsize(rxbuf),
							0, (struct sockaddr *)&rcxt.netx.source, &addrLen);

	if (length < 0) {
		acnlogerror(lgERR);
	} else {
		rcxt.rlp.rlsk = rlsk;
		rcxt.netx.rxbuf = rxbuf;
		rlp_packetRx(getRxdata(rxbuf), length, &rcxt);
	}
	if (--rxbuf->usecount > 0) {
		/* if still in use relinquish it - otherwise keep for next packet */
		rxbuf = NULL;
	}
	LOG_FEND();
}

/**********************************************************************/
#if RECEIVE_DEST_ADDRESS
groupaddr_t
netx_lastRxDest()
{
	struct cmsghdr *cmsg;
 
	/* Look for ancillary data of our type*/
	for (cmsg = CMSG_FIRSTHDR(&rxhdr); cmsg != NULL; cmsg = CMSG_NXTHDR(&rxhdr, cmsg)) {
		if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO) {
			return ((struct in_pktinfo *)(CMSG_DATA(cmsg)))->ipi_addr.s_addr;
		}
	}
	return 0;
}
#endif /* RECEIVE_DEST_ADDRESS */

/**********************************************************************/
/*
func: netxGetMyAddr

Return the IP address for a RLP socket.

This is not a useful function for looking up our own IP address as
in many cases it will simply return INADDR_ANY.
*/
int
netxGetMyAddr(struct rlpsocket_s *cxp, netx_addr_t *addr)
{
	unsigned int size = sizeof(netx_addr_t);
	nativesocket_t sk;

	sk = cxp->sk;

	LOG_FSTART();
	if (getsockname(sk, (struct sockaddr *)addr, &size) < 0) return -1;
	LOG_FEND();
	return 0;
}
