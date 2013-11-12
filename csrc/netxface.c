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
file: netxface.c

Operating System and Stack dependent netwroking functions.

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
stack to worry about interfaces. So while this code allows for sockets
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
address which can be multicast). For example in SDT, all multicast uses
the SDT_MULTICAST_PORT.

Destination (multicast group) address:

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
but if it can extract that information (RECEIVE_DEST_ADDRESS is true)
then we do so and pass the group address on to the higher layer incoming
packet handler. NOTE: We only need to pass the group address, not a full
address and port structure, because the port (and interface if desired
subject to the limitations above) are determined by the socket.

*/

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
#define lgFCTY LOG_NETX
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
	function: netx_init

	Initialize networking
*/
int netx_init(void)
{
	static bool initialized = 0;
	struct sigaction sig;

	LOG_FSTART();
	if (initialized) {
		acnlogmark(lgINFO, "already initialized");
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
pointer: rlpsocks [static]

A list of current RLP sockets.
*/
static rlpsocket_t *rlpsocks = NULL;

/**********************************************************************/
static rlpsocket_t *
findrlsk(port_t port)
{
	rlpsocket_t *rs;
	
	LOG_FSTART();
	for (rs = rlpsocks; rs; rs = rs->lnk.r) {
		if (rs->port == port) break;
	}
	LOG_FEND();
	return rs;
}

/**********************************************************************/
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
static struct skgroups_s *
findgroup(rlpsocket_t *rs, ip4addr_t group, int *ix)
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
static struct skgroups_s *
findblankgp(rlpsocket_t *rs, int *ix)
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
static int
addgroup(rlpsocket_t *rs, ip4addr_t group)
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
static int
dropgroup(rlpsocket_t *rs, ip4addr_t group)
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
rlpsocket_t *
rlpSubscribe(netx_addr_t *lclad, protocolID_t protocol, rlpcallback_fn *callback, void *ref)
{
	rlpsocket_t *rs;
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
		rs = acnNew(rlpsocket_t);

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
rlpUnsubscribe(rlpsocket_t *rs, netx_addr_t *lclad, protocolID_t protocol)
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
				slUnlink(rlpsocket_t, rlpsocks, rs, lnk);
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
	netx_send_to()
		Send message to given address
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
	rlpsocket_t *rlsk;
	struct rxcontext_s rcxt;
	
	LOG_FSTART();
	rlsk = container_of(evptr, rlpsocket_t, rxfn);
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
