/**********************************************************************/
/*
Copyright (c) 2011, Philip Nye, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3
*/
/**********************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "acncommon.h"
#include "acnstd/protocols.h"
#include "acnmem.h"
#include "uuid.h"
#include "component.h"
#include "netxface.h"
#include "sdt.h"
#include "acntimer.h"
#include "acnlog.h"
#include "rlp.h"
#include "marshal.h"
#include "rxcontext.h"
#include "acnlists.h"

/**********************************************************************/
/*
	Command codes and classification - acn_sdt.h

Ad-hoc
	SDT_JOIN
	SDT_GET_SESSIONS
	SDT_SESSIONS

Channel out
	SDT_REL_WRAP
	SDT_UNREL_WRAP
	SDT_NAK (outbound from member)

Channel In
	SDT_JOIN_REFUSE
	SDT_JOIN_ACCEPT
	SDT_LEAVING
	SDT_NAK

Channel out wrapped
	SDT_CHANNEL_PARAMS
	SDT_LEAVE
	SDT_CONNECT
	SDT_DISCONNECT

Channel in wrapped
	SDT_ACK
	SDT_CONNECT_ACCEPT
	SDT_CONNECT_REFUSE
	SDT_DISCONNECTING
*/
/**********************************************************************/
/*
IP address and port strategy.


For our purposes we need one socket per incoming port (or per source
port for outgoing messages which comes to the same thing).

Adhoc messages - use one ephemeral source port for each local component

Owner channels (upstream traffic) - we use a separate ephemeral source
port for each local channel which means that we instantly identify the
channel by its incoming socket, but note that the SDT spec does not
allow a clean distinction between upstream and adhoc so we must be
prepared to answer adhoc messages on any channel inbound!

Remote channels (downstream traffic) - This is where the volume occurs.
Multicast traffic should be to SDT_PORT and unicast should give us
choice, but this is not guaranteed. There is no guarantee that
multicast addresses from multiple sources will be different.

Limits imposed by OS and system
No of open files (sockets) in a process
	Linux: ulimit -a
	e.g. 1024 (my Linux system)
No of multicast subscriptions per socket
	Linux: IP_MAX_MEMBERSHIPS in bits/in.h and compiled into stack
No of ephemeral ports
	Linux: /sbin/sysctl net.ipv4.ip_local_port_range

cat /proc/sys/fs/file-max
Binding each socket to just one address keeps it's traffic tidy but 
means that you need to pick a local address.

For each component define one upstream/adhoc socket with ephemeral 
port. This can be bound to INADDR_ANY without penalty because the 
port is unique. Incoming traffic then has its component already 
selected by socket. For some messages (e.g. NAK) we then need to do 
a lookup by channel No which could be avoided if we had a separate 
socket for each Lchannel.

For downstream traffic each packet must be filtered by Src CID and by
chanNo to identify the session. Source CID is hashed whilst it is 
unlikely that there are a huge number of channels from the same 
Rcomponent.

In the case that we have Lchannels with very large memberships we 
will have large numbers of Rchans and Rcomps - separate sockets for 
each would use resources fast. OTOH trying to use a single socket is 
doomed because of subscription limits and because we do not have 
full control.


group join latency: assume all channels are multicast. 4 cases 
remote member state, local member state local initiation, remote 
initiation

remote initiation:
1.       rx Join-Rchan [unicast, src Rchan inbound, dest Loc adhoc]
2.       tx JAccept-Rchan [unicast, src xxx (unless unicast), dest Rchan inbound] + IGMP join Rchan
					local -> JOIN_PENDING
3.       tx Join-Lchan [unicast, src Lchan inbound, dest Rchan inbound]
4.       rx JAccept-Lchan [inicast, src xxx, dest Lchan inbound]
					remote -> JOIN_PENDING
5 or 6.  rx ACK-Lchan [multicast, src Rchan inbound, dest Rchan outbound]
					may get missed if switch still not joined
					remote -> PRE_MEMBER (don't know rem is really receiving yet)
					if (local == PRE_MEMBER) local -> MEMBER
6 or 5.  tx ACK-Rchan [multicast, src Lchan inbound, dest Lchan outbound]
					if (remote >= PRE_MEMBER) local -> MEMBER (5 before 6)
					else local -> PRE_MEMBER (6 before 5)
...
7.       rx ACK-Lchan for subsequent wrapper
or             remote -> MEMBER
7.       tx ACK-Rchan repeat of 6. after 100 & 300 ms
					remote -> MEMBER (assume rem got a repeat)


remote initiation with ACK before JAccept:
1.       rx Join-Rchan [unicast, src Rchan inbound, dest Loc adhoc]
2.       tx JAccept-Rchan [unicast, src xxx (unless unicast), dest Rchan inbound] + IGMP join Rchan
					local -> JOIN_PENDING
3.       tx Join-Lchan [unicast, src Lchan inbound, dest Rchan inbound]
4.       rx ACK-Lchan [multicast, src Rchan inbound, dest Rchan outbound]
					remote -> ACKED (or assume we missed the JAccept?)
5 or 6.  rx JAccept-Lchan [inicast, src xxx, dest Lchan inbound]
					remote -> PRE_MEMBER (don't know rem is really receiving yet)
					if (local == PRE_MEMBER) local -> MEMBER
6 or 5.  tx ACK-Rchan [multicast, src Lchan inbound, dest Lchan outbound]
					may get missed if switch still not joined
					if (remote >= ACKED) local -> MEMBER
					else local -> PRE_MEMBER
...
7.       rx ACK-Lchan for subsequent wrapper
or             remote -> MEMBER
7.       tx ACK-Rchan repeat of 6. after 100 & 300 ms
					remote -> MEMBER (assume rem got a repeat)


local initiation:
1.       tx Join-Lchan [unicast, src Lchan inbound, dest Rchan inbound]
2.       rx JAccept-Lchan [inicast, src xxx, dest Lchan inbound]
					remote -> JOIN_PENDING
3.       rx Join-Rchan [unicast, src Rchan inbound, dest Loc adhoc]
4.       tx JAccept-Rchan [unicast, src xxx (unless unicast), dest Rchan inbound] + IGMP join Rchan
					local -> JOIN_PENDING
5 or 6.  tx ACK-Rchan [multicast, src Lchan inbound, dest Lchan outbound]
					if (remote >= PRE_MEMBER) local -> MEMBER
					else local -> PRE_MEMBER 
6 or 5.  rx ACK-Lchan [multicast, src Rchan inbound, dest Rchan outbound]
					remote -> PRE_MEMBER (don't know rem is really receiving yet)
					if (local == PRE_MEMBER) local -> MEMBER
...
7.       rx ACK-Lchan for subsequent wrapper
or             remote -> MEMBER
7.       tx ACK-Rchan repeat of 5. after 100 & 300 ms
					remote -> MEMBER (assume rem got a repeat)

*/

/**********************************************************************/

#define unicastLchan(chan) ((chan)->flags & CHF_UNICAST)
/**********************************************************************/
/*
Timeout values
Primary values are mostly defined in epi18.h
*/
/**********************************************************************/
/* MAK_PRE_TIMEOUT_FACTOR is our own MAK algorithm, not part of the spec */
#define KEEPALIVE_FACTOR 0.5

#define FTIMEOUT_ms(exp_s, factor) timerval_ms((exp_s) * (int)(1000.0 * (factor)))

#define RECIPROCAL_TIMEOUT(Lchan) FTIMEOUT_ms((Lchan)->params.expiry_sec, RECIPROCAL_TIMEOUT_FACTOR)
#define Lchan_MAK_TIMEOUT(Lchan) FTIMEOUT_ms((Lchan)->params.expiry_sec, MAK_TIMEOUT_FACTOR)
#define Lchan_KEEPALIVE_ms(Lchan) FTIMEOUT_ms((Lchan)->params.expiry_sec, KEEPALIVE_FACTOR)
#define Lchan_NAK_BLANK_TIME(Lchan) timerval_ms(NAK_BLANKTIME((Lchan)->params.nakholdoff))

#define memb_NAK_TIMEOUT(memb) FTIMEOUT_ms((memb)->loc.params.expiry_sec, NAK_TIMEOUT_FACTOR)
#define memb_NAK_MAX_TIME(memb) timerval_ms((memb)->loc.params.nakmaxtime)

/**********************************************************************/
/*
MAK cycle values
FIXME these are a starting point but need to be dynamically regulated
MAKspan may need to vary with session size
*/
/**********************************************************************/

#define DFLT_MAKTHR     4
#define DFLT_MAKSPAN    5
 
/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_SDT
/**********************************************************************/
/*
Prototypes
*/
/**********************************************************************/
static void sdtRxAdhoc(const uint8_t *pdus, int blocksize, rxcontext_t *rcxt);
static void sdtRxLchan(const uint8_t *pdus, int blocksize, rxcontext_t *rcxt);
static void sdtRxRchan(const uint8_t *pdus, int blocksize, rxcontext_t *rcxt);
static void sdtLevel2Rx(const uint8_t *pdus, int blocksize, member_t *memb);
static int createRecip(Lcomponent_t *Lcomp, Rcomponent_t *Rcomp, member_t *memb);
static int sendJoinAccept(Rchannel_t *Rchan, member_t *memb);
static txwrap_t *justACK(member_t *memb, bool keep);
static void firstACK(member_t *memb);
static int sendJoinRefuseData(const uint8_t *joinmsg, uint8_t refuseCode, rxcontext_t *rcxt);
static int sendJoinRefuse(member_t *memb, uint8_t refuseCode);
static int sendLeaving(member_t *memb, uint8_t refuseCode);
static void sendNAK(Rchannel_t *Rchan, bool suppress);
static int connectAll(Lchannel_t *Lchan);
static int disconnectAll(Lchannel_t *Lchan, uint8_t reason, bool eject);
static int sendSessions(Lcomponent_t *Lcomp, netx_addr_t *dest);
static void resendWrappers(Lchannel_t *Lchan, int32_t first, int32_t last);
static void updateRmembSeq(member_t *memb, int32_t Rseq);
static uint8_t *setMAKs(uint8_t *bp, Lchannel_t *Lchan, uint16_t flags);

int addProtoMsg(txwrap_t *txwrap, member_t *memb, protocolID_t proto,
				const uint8_t *data, int size, uint16_t wflags);

int sendWrap(Lchannel_t *Lchan, member_t *memb, protocolID_t proto, 
				const uint8_t *data, int size, uint16_t wflags);

#define addSDTmsg(txwrap, memb, msg, flags) \
					addProtoMsg(txwrap, memb, SDT_PROTOCOL_ID, msg, sizeof(msg), flags)
#define SDTwrap(memb, msg, flags) \
					sendWrap((memb)->rem.Lchan, (memb), SDT_PROTOCOL_ID, \
								(msg), sizeof(msg), flags)
static txwrap_t *nextProtoMsg(txwrap_t *txwrap, member_t *memb, protocolID_t proto,
										const uint8_t *data, int size, uint16_t wflags);
/* private flags to go with nextProtoMsg() */
#define _WRAP_ALL    0x8000
#define _WRAP_CLOSE  0x4000
#define _WRAP_RPT    0x2000
#define _WRAP_MASK   0x1fff
#define nextSDTmsg(txwrap, memb, msg, flags) \
					nextProtoMsg(txwrap, memb, SDT_PROTOCOL_ID, msg, sizeof(msg), flags)

extern void compDiscoverSDT(Rcomponent_t *Rcomp, netx_addr_t *src, uint8_t expiry);

/* Timeout callbacks */
static void joinTimeoutAction(acnTimer_t *timer);
static void recipTimeoutAction(acnTimer_t *timer);
static void prememberAction(acnTimer_t *timer);
static void makTimeoutAction(acnTimer_t *timer);
static void expireAction(acnTimer_t *timer);
static void NAKfailAction(acnTimer_t *timer);
static void NAKholdoffAction(acnTimer_t *timer);
static void blanktimeAction(acnTimer_t *timer);
static void keepaliveAction(acnTimer_t *timer);

/* SDT message reeivers */
static void rx_join(const uint8_t *data, int length, rxcontext_t *rcxt);
static void rx_getSessions(const uint8_t *data, int length, rxcontext_t *rcxt);
static void rx_joinAccept(const uint8_t *data, int length, rxcontext_t *rcxt);
static void rx_joinRefuse(const uint8_t *data, int length, rxcontext_t *rcxt);
static void rx_leaving(const uint8_t *data, int length, rxcontext_t *rcxt);
static void rx_ownerNAK(const uint8_t *data, int length, rxcontext_t *rcxt);
static void rx_sessions(const uint8_t *data, int length, rxcontext_t *rcxt);
static void rx_wrapper(const uint8_t *data, int length, rxcontext_t *rcxt, bool reliable);
static void rx_outboundNAK(const uint8_t *data, int length, rxcontext_t *rcxt);
static void rx_chparams(const uint8_t *data, int length, member_t *memb);
static void rx_leave(const uint8_t *data, int length, member_t *memb);
static void rx_connect(const uint8_t *data, int length, member_t *memb);
static void rx_conaccept(const uint8_t *data, int length, member_t *memb);
static void rx_conrefuse(const uint8_t *data, int length, member_t *memb);
static void rx_disconnect(const uint8_t *data, int length, member_t *memb);
static void rx_disconnecting(const uint8_t *data, int length, member_t *memb);
static void rx_ack(const uint8_t *data, int length, member_t *memb);

/**********************************************************************/
/*
External functions
*/
/**********************************************************************/
#if !CONFIG_RLP_SINGLE_CLIENT
#define rlp_sendbuf(txbuf, length, src, dest, srccid) rlp_sendbuf(txbuf, length, SDT_PROTOCOL_ID, src, dest, srccid) 
#endif
/**********************************************************************/
/*
Globals
*/
/**********************************************************************/
/*
Some strings for better debug messages
*/
static const char *jstates[] = {
	[MS_NULL]      = "MS_NULL",
	[MS_JOINRQ]    = "MS_JOINRQ",
	/* [MS_ACKED]     = "MS_ACKED", */
	[MS_JOINPEND]  = "MS_JOINPEND",
	/* [MS_PREMEMBER] = "MS_PREMEMBER", */
	[MS_MEMBER]    = "MS_MEMBER"
};

/**********************************************************************/
const unsigned short tasizes[] = {
	LEN_TA_NULL,
	LEN_TA_IPV4,
	LEN_TA_IPV6
};

/* Packet queue */
struct rxwrap_s *rxqueue;

/* List of first ACK packets (which get repeated) */
txwrap_t *firstacks;

/* Track last assigned channel No */
uint16_t lastChanNo;

/**********************************************************************/
/*
Helper functions and inlines
*/
/**********************************************************************/
/*
Layer 1 txbufs
*/
static inline int
sdt1_sendbuf(uint8_t *txbuf, int length, rlpsocket_t *src, netx_addr_t *dest, uuid_t srccid)
{
	marshalU16(txbuf + SDT1_OFS_LENFLG, length - SDT1_OFS_LENFLG + FIRST_FLAGS);
	return rlp_sendbuf(txbuf, length, src, dest, srccid);
}

#if acntestlog(lgDBUG)
void showcomponents(void);
const uint8_t *dbugloc = NULL;

#else
#define showcomponents()
#endif

/**********************************************************************/
/*
Marshal and unmarshal channel parameters
*/
static inline uint8_t *
marshalParams(uint8_t *bufp, chanParams_t *pp)
{
	bufp = marshalU8(bufp, pp->expiry_sec);
	bufp = marshalU8(bufp, (pp->flags & PARAM_FLAG_MASK));
	bufp = marshalU16(bufp, pp->nakholdoff);
	bufp = marshalU16(bufp, pp->nakmodulus);
	bufp = marshalU16(bufp, pp->nakmaxtime);
	return bufp;
}

static inline const uint8_t *
unmarshalParams(const uint8_t *bp, chanParams_t *pp)
{
	pp->expiry_sec =  unmarshalU8(bp); bp += 1;
	pp->flags =       unmarshalU8(bp); bp += 1;
	pp->nakholdoff =  unmarshalU16(bp); bp += 2;
	pp->nakmodulus =  unmarshalU16(bp); bp += 2;
	pp->nakmaxtime =  unmarshalU16(bp); bp += 2;
	return bp;
}

/**********************************************************************/
/*
Marshal and unmarshal transport addresses into sockets style address
structure. Note that address and port are in network byte order in this
structure irrespective of host byte ordering.
*/
static uint8_t *
marshalTA(uint8_t *bufp, const netx_addr_t *na)
{
	uint8_t *bp;

	switch (((struct sockaddr *)na)->sa_family) {
#if CONFIG_NET_IPV4
	case AF_INET:
		bp = marshalU8(bufp, SDT_ADDR_IPV4);
		bp = marshalBytes(bp, (uint8_t *)&netx_PORT(na), 2);
		bp = marshalBytes(bp, (uint8_t *)&netx_INADDR(na), 4);
		break;
#endif      
#if CONFIG_NET_IPV6
	case AF_INET6:
		bp = marshalU8(bufp, SDT_ADDR_IPV6);
		bp = marshalBytes(bp, (uint8_t *)&netx_PORT(na), 2);
		bp = marshalBytes(bp, (uint8_t *)&netx_IN6ADDR(na), 16);
		break;
#endif
	default:
		acnlogmark(lgERR,
				"Tx unknown socket address type %u",
				((struct sockaddr *)na)->sa_family);
		/* fall through */
	case 0:
		bp = marshalU8(bufp, SDT_ADDR_NULL);
		break;
	}
	return bp;
}

static const uint8_t *
unmarshalTA(const uint8_t *bufp, netx_addr_t *na)
{
	const uint8_t *bp;

	LOG_FSTART(lgFCTY);
	bp = bufp;
	switch (*bp++) {
	case SDT_ADDR_NULL:
		netx_TYPE(na) = 0;
		acnlogmark(lgDBUG, "Rx SDT_ADDR_NULL");
		break;
#if CONFIG_NET_IPV4
	case SDT_ADDR_IPV4:
		netx_TYPE(na) = AF_INET;
		bp = unmarshalBytes(bp, (uint8_t *)&netx_PORT(na), 2);
		bp = unmarshalBytes(bp, (uint8_t *)&netx_INADDR(na), 4);
		acnlogmark(lgDBUG, "Rx SDT_ADDR_IPV4 %s:%d", inet_ntoa(na->sin_addr), ntohs(na->sin_port));
		break;
#endif      
#if CONFIG_NET_IPV6
	case SDT_ADDR_IPV6:
		netx_TYPE(na) = AF_INET6;
		bp = unmarshalBytes(bp, (uint8_t *)&netx_PORT(na), 2);
		bp = unmarshalBytes(bp, (uint8_t *)&netx_INADDR(na), 16);
		acnlogmark(lgDBUG, "Rx SDT_ADDR_IPV6");
		break;
#endif
	default:
		acnlogmark(lgERR,
				"Rx unknown TA type %u", unmarshalU8(bufp));
		break;
	}
	LOG_FEND(lgFCTY);
	return bp;
}

/**********************************************************************/
/*
memory allocations
*/

#define new_member()     acnNew(member_t)
#define free_member(x)   free(x)
#define new_txwrap()    acnNew(txwrap_t)
#define free_txwrap(x)  free(x)
#define new_Lchannel()   acnNew(Lchannel_t)
#define free_Lchannel(x) free(x)
#define new_Rchannel()   acnNew(Rchannel_t)
#define free_Rchannel(x) free(x)

/**********************************************************************/
/*
Search functions - find things in lists and other groups
*/
/**********************************************************************/

/* find channels by channel No */
static inline Lchannel_t *
findLchan(Lcomponent_t *Lcomp, uint16_t chanNo)
{
	Lchannel_t *Lchan;

	if (Lcomp == NULL) return NULL;
	for (Lchan = Lcomp->sdt.Lchannels; Lchan != NULL; Lchan = Lchan->lnk.r) {
		if (Lchan->chanNo == chanNo) break;
	}
	return Lchan;
}

/**********************************************************************/

static inline Rchannel_t *
findRchan(Rcomponent_t *Rcomp, uint16_t chanNo)
{
	Rchannel_t *Rchan;

	if (Rcomp == NULL) return NULL;
	for (Rchan = Rcomp->sdt.Rchannels; Rchan != NULL; Rchan = Rchan->lnk.r) {
		if (Rchan->chanNo == chanNo) break;
	}
	return Rchan;
}

/**********************************************************************/
/* find members by Component */
static inline member_t *
findRmembComp(Lchannel_t *Lchan, Rcomponent_t *Rcomp)
{
	if (Lchan->membspace) {
		uint16_t i;

		for (i = 0; i < Lchan->himid; ++i)
			if (Lchan->members.many[i] && Lchan->members.many[i]->rem.Rcomp == Rcomp)
				return Lchan->members.many[i];
	} else if (Lchan->members.one && Lchan->members.one->rem.Rcomp == Rcomp)
		return Lchan->members.one;
	return NULL;
}

/**********************************************************************/
static inline member_t *
findLmembComp(Rchannel_t *Rchan, Lcomponent_t *Lcomp)
{
#if CONFIG_SINGLE_COMPONENT
	(void) Lcomp;
	return firstMemb(Rchan);
#else
	member_t *memb;
	
	if (Rchan == NULL) return NULL;
	for (memb = Rchan->members; memb != NULL; memb = memb->loc.lnk.r) {
		if (memb->loc.Lcomp == Lcomp) break;
	}
	return memb;
#endif
}

/**********************************************************************/
/* find members by MID */
#define getRmemb(Lchan, i) (((Lchan)->membspace) ? (Lchan)->members.many[i] : (Lchan)->members.one)

static inline member_t *
findRmembMID(Lchannel_t *Lchan, uint16_t mid)
{
	return getRmemb(Lchan, mid - 1);
}

/**********************************************************************/
static inline member_t *
findLmembMID(Rchannel_t *Rchan, uint16_t mid)
{
#if CONFIG_SINGLE_COMPONENT
	return firstMemb(Rchan)->loc.mid == mid ? firstMemb(Rchan) : NULL;
#else
	member_t *memb;
	
	if (Rchan == NULL) return NULL;
	for (memb = Rchan->members; memb != NULL; memb = memb->loc.lnk.r) {
		if (memb->loc.mid == mid) break;
	}
	return memb;
#endif
}

/**********************************************************************/
#if CONFIG_SDT_SINGLE_CLIENT
static struct sdt_client_s *
findConnectedClient(member_t *memb, protocolID_t proto)
{
	return (proto == CONFIG_SDT_CLIENTPROTO && memb->connect)
				? &membLcomp(memb)->sdt.client : NULL;
}

#else

static struct sdt_client_s *
findConnectedClient(member_t *memb, protocolID_t proto)
{
	struct sdt_client_s *client;

	if (Lcomp == NU::) return NULL;
	for (client = Lcomp->sdt.clients; client != NULL; client = client->lnk.r) {
		if (client->protocol == proto) break;
	}
	return client;
}

#endif

/**********************************************************************/
/*
Link/unlink functions
*/

/*
	Remote member links
	Remote members are asigned a MID when they are linked into the local
	channel (Lchan) and this MID is freed when they are unlinked.

	They are no longer linked in a list. Instead, an array of member
	pointers indexed by MID is maintained. However, to conserve space,
	this array is grown in chunks as members are added. In the degenerate
	case of a single member, the Lchan->members field points directly to
	this member. With more members, it points to the array so to lookup a
	member by mid we simply index the MID into this array.

	To improve performance of code which needs to scan this array (e.g.
	MAK cycle) we try and keep it as dense as possible, so new members
	are only added to the end if there are no vacant spaces within the
	array.

	No attempt is currently made to shrink the array as members are
	removed, until they are all gone (or as a special case, all but MID
	1)
*/
/* size steps */
static const unsigned int mssizes[] = {
	sizeof(struct member_s *),
	pp0size,
	pp1size,
	pp2size,
	pp3size
};

#define maxmembers(ms) (mssizes[ms]/sizeof(struct member_s *))

static int
growMembSpace(Lchannel_t *Lchan)
{
	uint8_t *ospace;
	uint8_t *nspace;
	unsigned int osize, nsize;

	osize = mssizes[Lchan->membspace];
	nsize = mssizes[Lchan->membspace + 1];
	nspace = mallocx(nsize);
	ospace = (Lchan->membspace) ? (uint8_t *)(Lchan->members.many) : (uint8_t *)(&Lchan->members.one);
	memcpy(nspace, ospace, osize);
	memset(nspace + osize, 0, nsize - osize);
	Lchan->members.many = (member_t **)nspace;
	if (Lchan->membspace++ > 0) free(ospace);
	assert(Lchan->membspace < arraycount(mssizes));
	return 0;
}

static int
linkRmemb(Lchannel_t *Lchan, member_t *memb)
{
	uint16_t ix;

	if (Lchan->himid == 0) {
		/* assign member 1 */
		Lchan->himid = memb->rem.mid = 1;
		Lchan->members.one = memb;
	} else {
		if (Lchan->membercount < Lchan->himid) {
			/* find an empty slot */
			for (ix = 0; Lchan->members.many[ix] != NULL;) {
				++ix;
				assert(ix < Lchan->himid);
			}
		} else {
			/* check overflow and get new memberspace */
			if (Lchan->himid == 0xfffe
				|| (Lchan->himid == maxmembers(Lchan->membspace)
					&& growMembSpace(Lchan) < 0))
			{
				errno = ENOMEM;
				return -1;
			}
			/* assign next higher member */
			ix = Lchan->himid++;
		}
		memb->rem.mid = ix + 1;
		Lchan->members.many[ix] = memb;
	}
	return ++Lchan->membercount;
}

static int
unlinkRmemb(Lchannel_t *Lchan, member_t *memb)
{

	if (Lchan->membspace == 0) {
		assert(Lchan->himid == 1 && Lchan->membercount == 1);
		Lchan->members.one = NULL;
		Lchan->himid = Lchan->membercount = 0;
	} else {
		uint16_t ix;

		ix = memb->rem.mid - 1;
		Lchan->members.many[ix] = NULL;
		if (memb->rem.mid == Lchan->himid && Lchan->membercount > 1) {
			while (Lchan->members.many[--ix] == NULL);
			Lchan->himid = ix + 1;
		}
		if (--Lchan->membercount == 0 || Lchan->himid == 1) {
			member_t **many;

			many = Lchan->members.many;
			if (Lchan->membercount)
				Lchan->members.one = many[0];
			else {
				Lchan->members.one = NULL;
				Lchan->himid = 0;
			}
			free(many);
			Lchan->membspace = 0;
		}
	}
	return Lchan->membercount;
}
/**********************************************************************/
static inline void
linkLchan(Lcomponent_t *Lcomp, Lchannel_t *Lchan)
{
	Lchan->lnk.r = Lcomp->sdt.Lchannels;
	Lcomp->sdt.Lchannels = Lchan;
}

static inline int
unlinkLchan(Lcomponent_t *Lcomp, Lchannel_t *Lchan)
{
	Lchannel_t *lp = Lcomp->sdt.Lchannels;

	if (lp == Lchan) return ((Lcomp->sdt.Lchannels = Lchan->lnk.r) != NULL);
	else while (1) {
		if (lp == NULL) return -1;
		if (lp->lnk.r == Lchan) {
			lp->lnk.r = Lchan->lnk.r;
			return 1;
		}
		lp = lp->lnk.r;
	}
}

/**********************************************************************/
#if !CONFIG_SINGLE_COMPONENT
static inline void
linkLmemb(Rchannel_t *Rchan, member_t *memb)
{
	memb->loc.lnk.r = Rchan->members;
	Rchan->members = memb;
}

static inline member_t *
unlinkLmemb(Rchannel_t *Rchan, member_t *memb)
{
	member_t *mp;

	if (Rchan->members == memb) Rchan->members = memb->loc.lnk.r;
	else for (mp = Rchan->members; mp; mp = mp->loc.lnk.r)
		if (mp->loc.lnk.r == memb) {
			mp->loc.lnk.r = memb->loc.lnk.r;
			break;
		}
	return Rchan->members;
}
#endif

/**********************************************************************/
static inline void
linkRchan(Rcomponent_t *Rcomp, Rchannel_t *Rchan)
{
	Rchan->lnk.r = Rcomp->sdt.Rchannels;
	Rcomp->sdt.Rchannels = Rchan;
}

static inline Rchannel_t *
unlinkRchan(Rcomponent_t *Rcomp, Rchannel_t *Rchan)
{
	Rchannel_t *lp = Rcomp->sdt.Rchannels;

	if (Rcomp->sdt.Rchannels == Rchan) Rcomp->sdt.Rchannels = Rchan->lnk.r;
	else for (lp = Rcomp->sdt.Rchannels; lp; lp = lp->lnk.r)
		if (lp->lnk.r == Rchan) {
			lp->lnk.r = Rchan->lnk.r;
			break;
		}
	return Rcomp->sdt.Rchannels;
}

/**********************************************************************/
/*
Print a transport address (to a string)
*/
#include <arpa/inet.h>

static const uint8_t *
printTaddr(const uint8_t *ta, char *outstr)
{
	char addrstr[INET6_ADDRSTRLEN];

	switch (*ta) {
	case SDT_ADDR_NULL:
		strcpy(outstr, "NULL");
		break;
	case SDT_ADDR_IPV4: {
		struct in_addr addr;
		uint16_t port;

		port = unmarshalU16(ta + OFS_TA_PORT);
		unmarshalBytes(ta + OFS_TA_ADDR, (uint8_t *)&addr.s_addr, 4);
		inet_ntop(AF_INET, &addr, addrstr, INET6_ADDRSTRLEN);
		sprintf(outstr, "%s, port %u", addrstr, port);
		break;
		}
	case SDT_ADDR_IPV6: {
		struct in6_addr addr;
		uint16_t port;

		port = unmarshalU16(ta + OFS_TA_PORT);
		unmarshalBytes(ta + OFS_TA_ADDR, addr.s6_addr, 16);
		inet_ntop(AF_INET6, &addr, addrstr, INET6_ADDRSTRLEN);
		sprintf(outstr, "%s, port %u", addrstr, port);
		break;
		}
	default:
		strcpy(outstr, "WARNING: unknown transport address type!");
		return NULL;
	}
	return ta + tasizes[*ta] + 1;
}

/**********************************************************************/
/*
Print a protocol ID to a string
*/
static char *
showProtocol(protocolID_t protocol, char *tmpstr)
{
	switch (protocol) {
	case SDT_PROTOCOL_ID:
		return SDT_PROTOCOL_NAME;
	case DMP_PROTOCOL_ID:
		return DMP_PROTOCOL_NAME;
	case E131_PROTOCOL_ID:
		return E131_PROTOCOL_NAME;
	default:
		if ((protocol >> 16) == 0)
			sprintf(tmpstr, "esta.unknown (code %u)", protocol & 0xffff);
		else
			sprintf(tmpstr, "manufacturer %u, protocol code %u",
						protocol >> 16, protocol & 0xffff);
		return tmpstr;
	}
}

/**********************************************************************/
/*
Assign the next available channel No
*/

static uint16_t
getNewChanNo()
{
	while (++lastChanNo == 0)/* void */;
	return lastChanNo;
}

/**********************************************************************/
/*
Fixed or nearly fixed wrapped messages
*/
/**********************************************************************/
const uint8_t leave_msg[3] = {
	FIRST_bFLAGS,
	3,
	SDT_LEAVE
};
const uint8_t connect_msg[7] = {
	FIRST_bFLAGS,
	7,
	SDT_CONNECT,
	stmarshal32(CONFIG_SDT_CLIENTPROTO)
};
const uint8_t connectaccept_msg[7] = {
	FIRST_bFLAGS,
	7,
	SDT_CONNECT_ACCEPT,
	stmarshal32(CONFIG_SDT_CLIENTPROTO)
};
const uint8_t disconnect_msg[7] = {
	FIRST_bFLAGS,
	7,
	SDT_DISCONNECT,
	stmarshal32(CONFIG_SDT_CLIENTPROTO)
};

/* following are not constant as they need modification before sending */
uint8_t connectrefuse_msg[8] = {
	FIRST_bFLAGS,
	8,
	SDT_CONNECT_REFUSE,
	0, 0, 0, 0,          /* fill in with protocol */
	SDT_REASON_NONSPEC   /* overwrite with reason code */
};

uint8_t disconnecting_msg[8] = {
	FIRST_bFLAGS,
	8,
	SDT_DISCONNECTING,
	stmarshal32(CONFIG_SDT_CLIENTPROTO),
	SDT_REASON_NONSPEC   /* overwrite with reason code */
};

#define sendLeave(memb) SDTwrap(memb, leave_msg, WRAP_REL_ON)
#define sendConnect(memb) SDTwrap(memb, connect_msg, WRAP_REL_ON)
#define sendConnaccept(memb) SDTwrap(memb, connectaccept_msg, WRAP_REL_ON | WRAP_REPLY)
#define sendDisconnect(memb) SDTwrap(memb, disconnect_msg, WRAP_REL_ON)

#define sendConnrefuse(memb, proto, reason) (\
					marshalU32(connectrefuse_msg + 3, (proto)), \
					connectrefuse_msg[7] = (reason), \
					SDTwrap(memb, connectrefuse_msg, WRAP_REL_ON | WRAP_REPLY))

#define sendDisconnecting(memb, reason) (\
					disconnecting_msg[7] = (reason), \
					SDTwrap(memb, disconnect_msg, WRAP_REL_ON | WRAP_REPLY))

/**********************************************************************/
/*
SDT startup
*/
/**********************************************************************/
#define getrandomshort() 0

static int
sdt_startup()
{
	static bool initialized = false;
	int rslt;

	LOG_FSTART(lgFCTY);
	if (!initialized) {
		if ((rslt = startACNmem()) < 0) {
			acnlogmark(lgDBUG, "Startup fail!");
			return rslt;
		}

		rslt = rlp_init();
		assert(rslt == 0);

		lastChanNo = getrandomshort();

		initialized = true;
	}
	LOG_FEND(lgFCTY);
	return 0;
}

/**********************************************************************/
#if CONFIG_SINGLE_COMPONENT
#define FAIL -1
#define SUCCESS 0

int
#else
#define FAIL NULL
#define SUCCESS Lcomp

Lcomponent_t *
#endif
sdtRegister(uuid_t cid, grouprx_t scope, uint8_t scopebits,
						uint8_t expiry, memberevent_fn *membevent)
{
	Lcomponent_t *Lcomp;
	grouprx_t scopemask;
	int rslt;

	LOG_FSTART(lgFCTY);
	if (expiry == 0 || membevent == NULL) {
		errno = EINVAL;
		return FAIL;
	}
	if ((rslt = sdt_startup()) < 0) return FAIL;
#if CONFIG_SINGLE_COMPONENT
	Lcomp = &localComponent;
	if ((Lcomp->sdt.flags & CF_OPEN)) {  /* already in use */
		errno = EADDRNOTAVAIL;
		return FAIL;
	}
	uuidcpy(Lcomp->hd.uuid, cid);
#else
	rslt = findornewLcomp(cid, &Lcomp);
	if (rslt < 0) return FAIL;
	if (rslt == 0 && (Lcomp->sdt.flags & CF_OPEN)) {  /* already in use */
		errno = EADDRNOTAVAIL;
		return FAIL;
	}
	Lcomp->useflags |= USEDBY_SDT;
#endif

#if CONFIG_NET_IPV4 && CONFIG_EPI10
	assert(scopebits <= 24);
	scopemask = htonl(0 - (1 << (32 - scopebits)));
	Lcomp->sdt.scopenhost = scope & scopemask;
	if (Lcomp->sdt.scopenhost != scope) {
		acnlogmark(lgWARN, "scope bits discarded");
	}

	Lcomp->sdt.scopenhost |= htonl((ntohl(netx_getmyip(0)) & 0xff) << (24 - scopebits));
	Lcomp->sdt.dyn_mask = (1 << (24 - scopebits)) - 1;
	Lcomp->sdt.dyn_mcast =  unmarshalU16(cid + 2) ^ unmarshalU16(cid + 14);
#endif

	Lcomp->sdt.Lchannels = NULL;
	Lcomp->sdt.expiry = expiry;
	Lcomp->sdt.membevent = membevent;
	Lcomp->sdt.flags = CF_OPEN;
	LOG_FEND(lgFCTY);
	return SUCCESS;
}
#undef FAIL
#undef SUCCESS

/**********************************************************************/
void
sdtDeregister(if_MANYCOMP(Lcomponent_t *Lcomp))
{
	if_ONECOMP(struct Lcomponent_s * const Lcomp = &localComponent;)
	Lchannel_t *Lchan;
	
	LOG_FSTART(lgFCTY);
	for (Lchan = Lcomp->sdt.Lchannels; Lchan; Lchan = Lchan->lnk.r) {
		closeChannel(Lchan);
	}
	Lcomp->sdt.flags = 0;
	releaseLcomponent(Lcomp, USEDBY_SDT);
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
int
sdt_setListener(if_MANYCOMP(Lcomponent_t *Lcomp,) chanOpen_fn *joinRx, /* void *ref, */ netx_addr_t *adhocip)
{
	LOG_FSTART(lgFCTY);
	if_ONECOMP(struct Lcomponent_s * const Lcomp = &localComponent;)
	if (Lcomp->sdt.adhoc == NULL
		&& (Lcomp->sdt.adhoc = rlpSubscribe(NULL, SDT_PROTOCOL_ID, &sdtRxAdhoc, Lcomp)) == NULL)
	{
		return -1;
	}
	if (adhocip) netxGetMyAddr(Lcomp->sdt.adhoc, adhocip);

	Lcomp->sdt.joinRx = joinRx;
	/* Lcomp->sdt.joinref = ref; */
	Lcomp->sdt.flags |= CF_LISTEN;
	LOG_FEND(lgFCTY);
	return 0;
}

/**********************************************************************/
int
sdt_clrListener(if_MANYCOMP(Lcomponent_t *Lcomp))
{
	LOG_FSTART(lgFCTY);
	if_ONECOMP(struct Lcomponent_s * const Lcomp = &localComponent;)
	Lcomp->sdt.flags &= ~CF_LISTEN;
	Lcomp->sdt.joinRx = NULL;

	rlpUnsubscribe(Lcomp->sdt.adhoc, NULL, SDT_PROTOCOL_ID);
	Lcomp->sdt.adhoc = NULL;
	LOG_FEND(lgFCTY);
	return 0;
}

/**********************************************************************/
#if CONFIG_SDT_SINGLE_CLIENT
int
sdt_addClient(if_MANYCOMP(Lcomponent_t *Lcomp,) clientRx_fn *rxfn, void *ref)
{
	if_ONECOMP(struct Lcomponent_s * const Lcomp = &localComponent;)
	Lchannel_t *Lchan;

	LOG_FSTART(lgFCTY);
	if (rxfn == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (Lcomp->sdt.client.callback) {
		errno = ENOBUFS;
		return -1;
	}
	
	Lcomp->sdt.client.callback = rxfn;
	Lcomp->sdt.client.ref = ref;

	for (Lchan = Lcomp->sdt.Lchannels; Lchan; Lchan = Lchan->lnk.r) {
		if ((Lchan->flags & CHF_NOAUTOCON) == 0
			&& Lchan->membercount)
		{
			connectAll(Lchan);
		}
	}
	LOG_FEND(lgFCTY);
	return 0;
}

void
sdt_dropClient(if_MANYCOMP(Lcomponent_t *Lcomp))
{
	if_ONECOMP(struct Lcomponent_s * const Lcomp = &localComponent;)
	Lchannel_t *Lchan;

	LOG_FSTART(lgFCTY);
	for (Lchan = Lcomp->sdt.Lchannels; Lchan; Lchan = Lchan->lnk.r) {
		disconnectAll(Lchan, SDT_REASON_NONSPEC, false);
	}
	Lcomp->sdt.client.callback = NULL; Lcomp->sdt.client.ref = NULL;
	LOG_FEND(lgFCTY);
}

#else
/* FIXME implement !CONFIG_SDT_SINGLE_CLIENT */
#endif

/**********************************************************************/
/*
Helpers for mesaage receive
*/
/**********************************************************************/
/*
Read the receive queue
All queued wrappers are stored in a single queue
*/
void
readrxqueue()
{
	uint16_t INITIALIZED(vector);
	const uint8_t *pdup;
	const uint8_t *INITIALIZED(datap);
	const uint8_t *pp;
	int datasize = 0;
	uint8_t flags;
	member_t *memb;
	struct rxwrap_s *rxp;
	uint32_t INITIALIZED(protocol);
	uint16_t assoc;
	struct sdt_client_s *clientp;

	LOG_FSTART(lgFCTY);

	while (rxqueue != NULL) {
		rxp = rxqueue->lnk.l;   /* oldest is at tail */
		dlUnlink(rxqueue, rxp, lnk);
		/* wrapper has already been checked in queuerxwrap() */
		pdup = rxp->data + OFS_WRAPPER_CB;

		while (pdup < rxp->data + rxp->length) {
			flags = *pdup;
			pp = pdup + 2;
			pdup += getpdulen(pdup);

			if (flags & VECTOR_bFLAG) {
				vector = unmarshalU16(pp); pp += 2;
			}

			if (flags & HEADER_bFLAG) {
				protocol = unmarshalU32(pp); pp += 4;
				assoc = unmarshalU16(pp); pp += 2;
			}

			if (flags & DATA_bFLAG)  {
				datap = pp; /* get pointer to start of the data */
				datasize = pdup - pp;
			}
			forEachMemb(memb, rxp->Rchan) {
				if ((vector == ALL_MEMBERS || vector == memb->loc.mid)
					&& ((clientp = findConnectedClient(memb, protocol)))
					&& clientp->callback)
				{
					(*(clientp->callback))(memb, datap, datasize, clientp->ref);
				}
			}
		}
		releaseRxbuf(rxp->rxbuf);
		free(rxp);
	}
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
queueNxt
*/
static void
queuerxwrap(Rchannel_t *Rchan, struct rxwrap_s *rxp)
{
	uint16_t vector;
	const uint8_t *pdup;
	const uint8_t *datap;
	const uint8_t *pp;
	int datasize = 0;
	uint8_t flags;
	member_t *memb;
	uint32_t protocol;
	uint16_t assoc;
	bool haveClientPDUs = false;

	LOG_FSTART(lgFCTY);

	Rchan->Tseq = rxp->Tseq;
	Rchan->Rseq = rxp->Rseq;

	/* don't queue empty wrappers */
	if (rxp->length == LEN_WRAPPER_MIN) goto dumpwrap;

	if (rxp->length < (LEN_WRAPPER_MIN + OFS_CB_PDU1DATA)) {
		acnlogmark(lgERR, "Rx length error");
		goto dumpwrap;
	}

	pdup = rxp->data + OFS_WRAPPER_CB;
	/* first PDU must have all fields */
	if ((*pdup & (FLAG_bMASK)) != (FIRST_bFLAGS)) {
		acnlogmark(lgERR, "Rx bad first PDU flags");
		goto dumpwrap;
	}

	while (pdup < (rxp->data + rxp->length)) {
		flags = *pdup;
		pp = pdup + 2;
		acnlogmark(lgDBUG, "Rx PDU at %" PRIuPTR " len %d, flags %02x", (uintptr_t)pdup, getpdulen(pdup), flags);
		pdup += getpdulen(pdup);
		dbugloc = pdup;


		if (flags & VECTOR_bFLAG) {
			vector = unmarshalU16(pp); pp += 2;
		}

		if (flags & HEADER_bFLAG) {
			protocol = unmarshalU32(pp); pp += 4;
			assoc = unmarshalU16(pp); pp += 2;
		}

		if (flags & DATA_bFLAG)  {
			datap = pp; /* get pointer to start of the data */
			if ((datasize = pdup - pp) < 0) { /* get size of the data */
				acnlogmark(lgERR, "Rx blocksize error");
				goto dumpwrap;
			}
		}
		forEachMemb(memb, Rchan) {
			if (memb->loc.mstate >= MS_JOINPEND
				&& (vector == ALL_MEMBERS || vector == memb->loc.mid))
			{
				if (assoc && assoc != memb->rem.Lchan->chanNo) {
					acnlogmark(lgERR, "Rx association error");
				} else if (protocol == SDT_PROTOCOL_ID) {
					sdtLevel2Rx(datap, datasize, memb);
				} else if (protocol == CONFIG_SDT_CLIENTPROTO && memb->connect) {
					haveClientPDUs = true;
				}
			}
		}
	}
	if (haveClientPDUs) {
		dlAddHead(rxqueue, rxp, lnk);
		LOG_FEND(lgFCTY);
		return;
	}

dumpwrap:
	releaseRxbuf(rxp->rxbuf);
	free(rxp);
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Check whether a wrapper requires any member to ACK
*/
static bool
mustack(Rchannel_t *Rchan, int32_t Rseq, const uint8_t *wrapdata)
{
	uint16_t firstMAK;
	uint16_t lastMAK;
	member_t *memb;
	int32_t    MAKpoint;

	LOG_FSTART(lgFCTY);
	firstMAK = unmarshalU16(wrapdata + OFS_WRAPPER_FIRSTMAK);
	lastMAK = unmarshalU16(wrapdata + OFS_WRAPPER_LASTMAK);
	MAKpoint = Rseq - unmarshalU16(wrapdata + OFS_WRAPPER_MAKTHR);
	
	forEachMemb(memb, Rchan) {
		if (memb->loc.mstate == MS_MEMBER
			&& memb->loc.mid >= firstMAK
			&& memb->loc.mid <= lastMAK
			&& (MAKpoint - memb->loc.lastack) >= 0)
		{
			acnlog(lgDBUG, "- %s true", __func__);
			return true;
		}
	}
	acnlog(lgDBUG, "- %s false", __func__);
	return false;
}

/**********************************************************************/
static void
killMember(member_t *memb, uint8_t reason, uint8_t event)
{
	Rchannel_t *Rchan;
	Lchannel_t *Lchan;
	txwrap_t *txwrap;

	LOG_FSTART(lgFCTY);
	cancel_timer(&memb->rem.stateTimer);
	cancel_timer(&memb->loc.expireTimer);
	/* have we got first ACKs being resent? */
	if (memb->loc.mstate == MS_MEMBER) {
		for (txwrap = firstacks; txwrap; txwrap = txwrap->st.fack.lnk.r) {
			if (txwrap->st.fack.rptTimer.userp == memb) {
				cancel_timer(&txwrap->st.fack.rptTimer);
				slUnlink(txwrap_t, firstacks, txwrap, st.fack.lnk);
				cancelWrapper(txwrap);
				break;
			}
		}
	}
	Lchan = memb->rem.Lchan;

	txwrap = NULL;

	if (memb->connect & (CX_LOC | CX_REM)) {
		if ((memb->connect & CX_LOC)) {
			txwrap = nextSDTmsg(txwrap, memb, disconnect_msg, WRAP_REL_ON);
		}
		if ((memb->connect & CX_REM)) {
			disconnecting_msg[7] = reason;
			txwrap = nextSDTmsg(txwrap, memb, disconnecting_msg, WRAP_REL_ON);
		}
		(*membLcomp(memb)->sdt.membevent)(EV_LOCDISCONNECT, Lchan, memb);
	}
	if (memb->connect & CX_SDT)
		(*membLcomp(memb)->sdt.membevent)(EV_LOCLEAVE, Lchan, memb);
	memb->connect = 0;

	if (memb->rem.mstate >= MS_JOINPEND) {
		txwrap = nextSDTmsg(txwrap, memb, leave_msg, WRAP_REL_ON | _WRAP_CLOSE);
		assert(txwrap == NULL);

		if (memb->rem.mstate == MS_MEMBER) {
			/* pretend we're acked up to date */
			updateRmembSeq(memb, Lchan->Rseq);
			Lchan->ackcount--;
		}
	}
	memb->rem.mstate = MS_NULL;

	switch (memb->loc.mstate) {
	case MS_JOINRQ:
	case MS_JOINPEND:
		sendJoinRefuse(memb, reason);
		break;
	case MS_MEMBER:
		sendLeaving(memb, reason);
		break;
	case MS_NULL:
	default:
		break;
	}
	memb->loc.mstate = MS_NULL;

	if ((Rchan = get_Rchan(memb)) != NULL) {

#if CONFIG_SINGLE_COMPONENT
		cancel_timer(&Rchan->NAKtimer);
		unlinkRchan(memb->rem.Rcomp, Rchan);
#else
		if (unlinkLmemb(Rchan, memb) == NULL) {
			cancel_timer(&Rchan->NAKtimer);
			unlinkRchan(memb->rem.Rcomp, Rchan);
			free_Rchannel(Rchan);
			Rchan = NULL;
		}
#endif
	}

	switch (event) {
	case EV_JOINFAIL:
		(*membLcomp(memb)->sdt.membevent)(event, Lchan, memb->rem.Rcomp->hd.uuid);
		break;
	case EV_NAKTIMEOUT:
	case EV_MAKTIMEOUT:
	case EV_LOSTSEQ:
		(*membLcomp(memb)->sdt.membevent)(event, Lchan, memb);
		break;
	case EV_LOCCLOSE:
	default:
		break;
	}

	if (memb->rem.Rcomp->sdt.Rchannels == NULL) {
		releaseRcomponent(memb->rem.Rcomp, USEDBY_SDT);
	}
	unlinkRmemb(Lchan, memb);
	free_member(memb);

	if (Lchan->membercount == 0) {
		cancel_timer(&Lchan->blankTimer);
		cancel_timer(&Lchan->keepalive);

		if ((Lchan->flags & (CHF_RECIPROCAL | CHF_NOCLOSE)) == CHF_RECIPROCAL) {
			acnlogmark(lgDBUG, "closing empty channel");
			closeChannel(Lchan);
		}
	}
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
static void
killRchan(Rchannel_t *Rchan, uint8_t reason, uint8_t event)
{
	member_t *memb;

	LOG_FSTART(lgFCTY);
	forEachMemb(memb, Rchan) {
		killMember(memb, reason, event);
	}
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
static void
setFullMember(member_t *memb)
{
	assert(membLcomp(memb)->sdt.membevent);

	LOG_FSTART(lgFCTY);
	memb->connect = CX_SDT;
	(*membLcomp(memb)->sdt.membevent)(EV_JOINSUCCESS, memb->rem.Lchan, memb);

#if CONFIG_SDT_SINGLE_CLIENT
	if ((memb->rem.Lchan->flags & CHF_NOAUTOCON) == 0
		&& membLcomp(memb)->sdt.client.callback)
	{
		sendConnect(memb);
	}
#else
	/* FIXME implement !CONFIG_SDT_SINGLE_CLIENT */
#endif
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
static void
initLocMember(member_t *memb, const uint8_t *joindata, rxcontext_t *rcxt)
{
	Rcomponent_t *Rcomp;
	Rchannel_t *Rchan;
	const uint8_t *bp;

	LOG_FSTART(lgFCTY);
	Rchan = get_Rchan(memb);
	Rcomp = Rchan->owner;
	
	bp = joindata + OFS_JOIN_MID;
	memb->loc.mid = unmarshalU16(bp); bp += 2;

	if (CONFIG_SINGLE_COMPONENT || Rchan->chanNo == 0) {  /* is Rchan uninitialized? */
		Rchan->chanNo = unmarshalU16(bp);
		bp += 4; /* skip recip */
		Rchan->Tseq = unmarshalSeq(bp); bp += 4;
		Rchan->Rseq = unmarshalSeq(bp); bp += 4;
		bp = unmarshalTA(bp, &Rchan->outwd_ad);

		Rchan->inwd_ad = rcxt->netx.source;

		/* subscribe here */
		if (netx_TYPE(&Rchan->outwd_ad) == SDT_ADDR_NULL)
			netx_INIT_ADDR(&Rchan->outwd_ad, netx_PORT_EPHEM, netx_GROUP_UNICAST);
		Rchan->outwd_sk = rlpSubscribe(&Rchan->outwd_ad, SDT_PROTOCOL_ID, &sdtRxRchan, NULL);
		if (Rchan->outwd_sk == NULL) {
			acnlogerror(lgERR);
		}
		linkRchan(Rcomp, Rchan);
	} else {
		bp = joindata + OFS_JOIN_TADDR;
		bp += tasizes[*bp] + 1; /* already checked for validity */
	}

#if !CONFIG_SINGLE_COMPONENT
	/* link memb to Rchan */
	linkLmemb(Rchan, memb);
	memb->loc.Lcomp = ctxtLcomp;
#endif
	/* get the rest of the data */
	bp = unmarshalParams(bp, &memb->loc.params);
	memb->loc.mstate = MS_JOINRQ;
	memb->loc.expireTimer.action = expireAction;
	memb->loc.expireTimer.userp = memb;

	/* update discovery adhoc info!! bad bad bad. why is this in an SDT packet? */
	(*membLcomp(memb)->sdt.membevent)(EV_DISCOVER, Rcomp, (void *)bp);
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
SDT Message receive functions
*/
/**********************************************************************/
/*
Join - level 1 message to adhoc address or reciprocal join address

May be cold jopin, or reciprocal in response to one from us
Could also be a repeat as this message is subject to timeout and retry
*/
static void
rx_join(const uint8_t *data, int length, rxcontext_t *rcxt)
{
	Rcomponent_t *Rcomp = NULL;
	Lchannel_t *Lchan = NULL;
	member_t *memb = NULL;
	bool repeatJoin = false;
	const uint8_t *pardata;
	uint16_t mid;
	uint16_t recip = 0;
	uint16_t chanNo;
	uint8_t refuseCode;
	Rchannel_t *Rchan = NULL;
	int tasize;

	LOG_FSTART(lgFCTY);
	pardata = data + OFS_JOIN_TADDR;
	tasize = supportedTAsize(*pardata);
	refuseCode = SDT_REASON_RESOURCES;   /* default reason */

	if (length < LEN_JOIN_MIN
		|| (tasize >= 0 && length != LEN_JOIN(tasize))) {
		/* PDU is too wrong to even send joinRefuse */
		acnlogmark(lgERR, "Rx length error");
		return;
	}

	if (tasize < 0) {
		acnlogmark(lgERR, "Rx bad address type");
		refuseCode = SDT_REASON_BAD_ADDR;
		goto joinAbort;
	}

	pardata += tasize + 1;
	chanNo = unmarshalU16(data + OFS_JOIN_CHANNO);
	mid = unmarshalU16(data + OFS_JOIN_MID);

	/* a bunch of sanity checks */
	if (chanNo == 0
		|| mid == 0
		|| mid == 0xffff
		|| pardata[OFS_PARAM_EXPIRY] < MIN_EXPIRY_TIME_s
		|| (pardata[OFS_PARAM_FLAGS] & ~PARAM_FLAG_MASK) != 0
		|| unmarshalU16(pardata + OFS_PARAM_MODULUS) == 0
	) {
		acnlogmark(lgERR, "Rx bad params");
		refuseCode = SDT_REASON_PARAMETERS;
		goto joinAbort;
	}

	acnlogmark(lgDBUG, "Rx Join chan %d, mid %d", chanNo, mid);
/*
	see how much we know already:
	Should be the lot if its a repeat join. If it's a new unsolicited
	join we may know the component and if we have multiple Lcomponents we
	may also know the channel. If it's new reciprocal we should have the
	Lchan and the member from the reciprocal information but not Rchan -
	we need to connect the member and the new Rchan
*/
	repeatJoin = (Rcomp = findRcomp(rcxt->rlp.srcCID)) /* got Rcomp already */
					&& (Rchan = findRchan(Rcomp, chanNo))              /* got Rchan already */
					&& (memb = findLmembComp(Rchan, ctxtLcomp));       /* got memb already */
	if (repeatJoin) {
		if (memb->loc.mid != mid) {   /* repeat - is the mid correct? */
			acnlogmark(lgERR, "Rx already member");
			refuseCode = SDT_REASON_ALREADY_MEMBER;
			goto joinAbort;
		}
		acnlogmark(lgINFO, "Rx repeated join in %s state", jstates[memb->loc.mstate]);
	} else {
/*
	New Join
	if we have recip must be a reciprocal join
	otherwise must be a cold join
*/
		recip = unmarshalU16(data + OFS_JOIN_RECIP);
		if (recip) {
			acnlogmark(lgDBUG, "Rx recip join");
			/* reciprocal join - trace from local side to find the member */
			/* if it came to the reciprocal address we will have Lchan */
			Lchan = (Lchannel_t *)(rcxt->rlp.handlerRef);
			/* must have Rcomp already since we sent the join */
			if (  Rcomp == NULL
				|| Lchan->chanNo != recip
				|| (memb = findRmembComp(Lchan, Rcomp)) == NULL
				|| !(memb->rem.mstate == MS_JOINRQ || memb->rem.mstate == MS_JOINPEND)
			) {
				acnlogmark(lgERR, "Rx recip join from unknown member");
				refuseCode = SDT_REASON_PARAMETERS;
				goto joinAbort;
			}

			/* make sure its not already got an Rchannel */
			if (get_Rchan(memb)) {
					acnlogmark(lgERR, "Rx recip join: already member");
					refuseCode = SDT_REASON_NONSPEC;
					goto joinAbort;
			}
#if !CONFIG_SINGLE_COMPONENT
			/* OK now setup new Rchan - it may already exist for other Lcomps */
			if (Rchan == NULL && (Rchan = new_Rchannel()) == NULL) {
				acnlogerror(lgERR);
				goto joinAbort;
			}
			memb->loc.Rchan = Rchan;   /* so that initLocMember can find Rchan */
#else
			Rchan = &memb->Rchan;
#endif
			Rchan->owner = Rcomp;
			initLocMember(memb, data, rcxt);

		} else { /* not (recip) */
/*
			New unsolicited join
			try and allocate whatever is necessary:
			- We may have Rcomp already.
			- We may have Rchan if we are not CONFIG_SINGLE_COMPONENT but
			can't have Rchan without Rcomp
			On failure we need to be careful to de-allocate only what we
			have just allocated so keep a record.
*/
			acnlogmark(lgDBUG, "Rx cold join");
			if ((ctxtLcomp->sdt.flags & CF_LISTEN) == 0 || ctxtLcomp->sdt.joinRx == NULL) {
				acnlogmark(lgINFO, "Rx refused adhoc join");
				refuseCode = SDT_REASON_NONSPEC;
				goto joinAbort;
			}

			if (Rcomp == NULL) {
				switch (findornewRcomp(rcxt->rlp.srcCID, &Rcomp)) {
				case 0:
					acnlogmark(lgWARN, "Internal consistency error");
					/* fall through */
				case 1:
					Rcomp->useflags |= USEDBY_SDT;
					break;
				case -1:
				default:
					acnlogmark(lgERR, "Can't allocate new component");
					goto joinAbort;
				}
				/* Rcomp->sdt.adhocAddr = rcxt->netx.source; */
			}

#if !CONFIG_SINGLE_COMPONENT
			if (Rchan == NULL && (Rchan = new_Rchannel()) == NULL) {
				acnlogerror(lgERR);
				goto joinAbort;
			}
#endif
			if ((memb = new_member()) == NULL) {
				acnlogerror(lgERR);
				goto joinAbort;
			}
			
#if !CONFIG_SINGLE_COMPONENT
			memb->loc.Rchan = Rchan;   /* so that initLocMember can find Rchan */
#else
			Rchan = &memb->Rchan;
#endif
			Rchan->owner = Rcomp;
			/* got all our structures - initialize them */
			initLocMember(memb, data, rcxt);

			if (createRecip(ctxtLcomp, Rcomp, memb) < 0) {
				refuseCode = SDT_REASON_NO_RECIPROCAL;
				goto joinAbort;
			}
		}  /* end of leader join */
	}

	if (sendJoinAccept(Rchan, memb) >= 0 && !repeatJoin) {

		memb->loc.mstate = MS_JOINPEND;

		if (memb->rem.mstate >= MS_JOINPEND) {
			firstACK(memb);
			memb->loc.mstate = MS_MEMBER;
			if (memb->rem.mstate == MS_MEMBER) setFullMember(memb);
		}
	}

	LOG_FEND(lgFCTY);
	return;

joinAbort:
	acnlogmark(lgERR, "Rx rxJoin fail");
	if (!repeatJoin) {

#if CONFIG_SINGLE_COMPONENT
		/* take down any local member we've created */
		if (memb) {
			unlinkRchan(Rcomp, Rchan);    /* Rchan is same as memb */
			if (recip) {
				memset(&memb->Rchan, 0, sizeof(memb->Rchan));
				memb->loc.mstate = MS_NULL;
			} else {
				free_member(memb);
				memb = NULL;
			}
		}
		if (Rcomp && Rcomp->sdt.Rchannels == NULL) {
			releaseRcomponent(Rcomp, USEDBY_SDT);
			Rcomp = NULL;
		}
#else
		if (memb) {
			unlinkLmemb(Rchan, memb);
			if (recip) {
				memb->loc.mstate = MS_NULL;
			} else {
				free_member(memb);
				memb = NULL;
			}
		}
		if (Rchan && Rchan->members == NULL) {
			unlinkRchan(Rcomp, Rchan);
			free_Rchannel(Rchan);
			Rchan = NULL;
		}
		if (Rcomp && Rcomp->sdt.Rchannels == NULL) {
			releaseRcomponent(Rcomp, USEDBY_SDT);
			Rcomp = NULL;
		}
#endif

	}
	sendJoinRefuseData(data, refuseCode, rcxt);
	LOG_FEND(lgFCTY);
	return;
}

/**********************************************************************/
/*
Join-accept - level 1 message

When original Join had SDT_ADDR_NULL, this comes from the outboud
address to be used for the channel
*/

static void
rx_joinAccept(const uint8_t *data, int length, rxcontext_t *rcxt)
{
	Lchannel_t *Lchan;
	member_t *memb;

	LOG_FSTART(lgFCTY);
	if (length != LEN_JACCEPT) {
		/* PDU is wrong length */
		acnlogmark(lgERR, "Rx length error");
		return;
	}

	/* find our channel */
	Lchan = ((Lchannel_t *)(rcxt->rlp.handlerRef));
	if (Lchan->chanNo != unmarshalU16(data + OFS_JACCEPT_CHANNO)) {
		acnlogmark(lgERR, "Rx unknown channel No");
		return;
	}

	/* find the member */
	if ((memb = findRmembMID(Lchan, unmarshalU16(data + OFS_JACCEPT_MID))) == NULL) {
		acnlogmark(lgERR, "Rx member not in channel");
		return;
	}

	/* double check */
	if (!uuidsEq(memb->rem.Rcomp->hd.uuid, rcxt->rlp.srcCID)) {
		acnlogmark(lgERR, "Rx MID and CID do not match");
		return;
	}

	/* do we have the reciprocal channel? */
	if (get_Rchan(memb) != NULL 
		&& get_Rchan(memb)->chanNo != unmarshalU16(data + OFS_JACCEPT_RECIP))
	{
		acnlogmark(lgERR, "Rx wrong reciprocal channel");
		return;
	}

	/* Do we need the downstream address? */
	if (netx_TYPE(&Lchan->outwd_ad) == SDT_ADDR_NULL) {
		Lchan->outwd_ad = rcxt->netx.source;
	}

	switch (memb->rem.mstate) {
	case MS_JOINRQ:   /* this is normal - we've sen a join */
		memb->rem.mstate = MS_JOINPEND;
		memb->rem.stateTimer.action = recipTimeoutAction;
		set_timer(&memb->rem.stateTimer, RECIPROCAL_TIMEOUT(Lchan));
		if (get_Rchan(memb) && memb->loc.mstate == MS_JOINPEND) {
			firstACK(memb);
			memb->loc.mstate = MS_MEMBER;
		}
		break;
	case MS_NULL:     /* assume something went wrong */
		killMember(memb, SDT_REASON_NONSPEC, EV_JOINFAIL);
		return;
	case MS_MEMBER:
	default:
		acnlogmark(lgERR, "Rx remote in %s state", jstates[memb->rem.mstate]);
		/* fall through */
	case MS_JOINPEND: /* this is unexpected - probably a duplicate Jaccept */
		return;
	}
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Jon refuse - level 1 message
*/

static void
rx_joinRefuse(const uint8_t *data, int length, rxcontext_t *rcxt)
{
	Lchannel_t *Lchan;
	member_t *memb;

	LOG_FSTART(lgFCTY);
	if (length != LEN_JREFUSE) {
		/* PDU is wrong length */
		acnlogmark(lgERR, "Rx length error");
		return;
	}

	/* find our channel */
	Lchan = ((Lchannel_t *)(rcxt->rlp.handlerRef));
	if (Lchan->chanNo != unmarshalU16(data + OFS_JREFUSE_CHANNO)) {
		acnlogmark(lgERR, "Rx unknown channel No");
		return;
	}

	/* find the member */
	if ((memb = findRmembMID(Lchan, unmarshalU16(data + OFS_JREFUSE_MID))) == NULL) {
		acnlogmark(lgERR, "Rx member not in channel");
		return;
	}

	/* double check */
	if (!uuidsEq(memb->rem.Rcomp->hd.uuid, rcxt->rlp.srcCID)) {
		acnlogmark(lgERR, "Rx MID and CID do not match");
		return;
	}

	if (memb->rem.mstate != MS_JOINRQ) {
		acnlogmark(lgWARN, "Rx remote in %s state", jstates[memb->rem.mstate]);
	}
	killMember(memb, SDT_REASON_NONSPEC, EV_JOINFAIL);
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Leaving - level 1 message
*/

static void
rx_leaving(const uint8_t *data, int length, rxcontext_t *rcxt)
{
	Lchannel_t *Lchan;
	member_t *memb;

	LOG_FSTART(lgFCTY);
	if (length != LEN_LEAVING) {
		/* PDU is wrong length */
		acnlogmark(lgERR, "Rx length error");
		return;
	}

	/* find our channel */
	Lchan = ((Lchannel_t *)(rcxt->rlp.handlerRef));
	if (Lchan->chanNo != unmarshalU16(data + OFS_LEAVING_CHANNO)) {
		acnlogmark(lgERR, "Rx unknown channel No");
		return;
	}

	/* find the member */
	if ((memb = findRmembMID(Lchan, unmarshalU16(data + OFS_LEAVING_MID))) == NULL) {
		acnlogmark(lgERR, "Rx member not in channel");
		return;
	}

	/* double check */
	if (!uuidsEq(memb->rem.Rcomp->hd.uuid, rcxt->rlp.srcCID)) {
		acnlogmark(lgERR, "Rx MID and CID do not match");
		return;
	}
	
	updateRmembSeq(memb, unmarshalSeq(data + OFS_LEAVING_RSEQ));
	killMember(memb, SDT_REASON_NONSPEC, EV_REMLEAVE);
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
NAK  - level 1 message
coming upstream to us as channel owner
*/

static void
rx_ownerNAK(const uint8_t *data, int length, rxcontext_t *rcxt)
{
	Lchannel_t *Lchan;
	member_t *memb;
	int32_t first;
	int32_t last;

	LOG_FSTART(lgFCTY);
	if (length != LEN_NAK) {
		/* PDU is wrong length */
		acnlogmark(lgERR, "Rx length error");
		return;
	}

	/* find our channel */
	Lchan = ((Lchannel_t *)(rcxt->rlp.handlerRef));
	if (Lchan->chanNo != unmarshalU16(data + OFS_NAK_CHANNO)) {
		acnlogmark(lgERR, "Rx unknown channel No");
		return;
	}

	/* find the member */
	if ((memb = findRmembMID(Lchan, unmarshalU16(data + OFS_LEAVING_MID))) == NULL) {
		acnlogmark(lgERR, "Rx member not in channel");
		return;
	}
	
	/* double check */
	if (!uuidsEq(memb->rem.Rcomp->hd.uuid, rcxt->rlp.srcCID)) {
		acnlogmark(lgERR, "Rx MID and CID do not match");
		return;
	}

	first = unmarshalSeq(data + OFS_NAK_FIRSTMISS);
	last = unmarshalSeq(data + OFS_NAK_LASTMISS);
	if ((last - first) < 0) {
		acnlogmark(lgERR, "Rx inbound NAK bad range");
		return;
	}

	/* are we blanking these NAKs because we've just sent them? */
	if ((first - Lchan->nakfirst) >= 0 && (last - Lchan->naklast) < 0)
		return;

	/* need to resend something */
	if (Lchan->obackwrap == NULL) {
		acnlogmark(lgERR, "Rx NAK for %" PRIu32 " - %" PRIu32 ". back-wrappers empty",
						first, last);
	} else if ((first - Lchan->obackwrap->st.sent.Rseq) < 0
		|| (last - Lchan->Rseq) > 0)
	{
		acnlogmark(lgERR, "Rx NAK for %" PRIu32 " - %" PRIu32
						". available: %" PRIu32 " - %" PRIu32,
						first, last, Lchan->obackwrap->st.sent.Rseq, Lchan->Rseq);
		/* don't just kill here because we may have already sent them */
	} else resendWrappers(Lchan, first, last);

	updateRmembSeq(memb, unmarshalSeq(data + OFS_NAK_RSEQ));
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
NAK  - level 1 message
coming outbound from another group member (to suppress NAK storms)
	16	Leader CID (member CID is in RLP)
	2	ChanNo
	2	MID
	4	RSeq
	4	FirstMissed
	4	LastMissed
*/

static void
rx_outboundNAK(const uint8_t *data, int length, rxcontext_t *rcxt)
{
	Rchannel_t *Rchan;
	int32_t first, last;

	LOG_FSTART(lgFCTY);
	if (length != LEN_NAK) {
		/* PDU is wrong length */
		acnlogmark(lgERR, "Rx length error");
		return;
	}

	if ((Rchan = findRchan(rcxt->sdt1.Rcomp, unmarshalU16(data + OFS_NAK_CHANNO))) == NULL)
		return;  /* unknown channel */
	if (Rchan->NAKstate == NS_NULL) return;  /* not NAKing ourselves - nothing to supress */

	last = unmarshalSeq(data + OFS_NAK_LASTMISS);
	first = unmarshalSeq(data + OFS_NAK_FIRSTMISS);

	if ((Rchan->lastnak - last) > 0 || (Rchan->Rseq - first) <= 0)
		return; /* ignore if we want more packets than this one */

	sendNAK(Rchan, true);
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Leave - wrapped message
*/

static void
rx_leave(const uint8_t *data UNUSED, int length, member_t *memb)
{
//   UNUSED_ARG(data)

	LOG_FSTART(lgFCTY);
	if (length != LEN_LEAVE) {   /* PDU is wrong length */
		acnlogmark(lgERR, "Rx length error");
		return;
	}
	killMember(memb, SDT_REASON_ASKED_TO_LEAVE, EV_REMLEAVE);
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
channel parameters - wrapped message
*/

static void
rx_chparams(const uint8_t *data, int length, member_t *memb)
{
	const uint8_t *bp;
	Lchannel_t *Lchan;

	LOG_FSTART(lgFCTY);
	if (length < LEN_CHPARAMS(0)
		|| length != LEN_CHPARAMS(supportedTAsize(data[OFS_CHPARAMS_TADDR])))
	{   /* PDU is wrong length */
		acnlogmark(lgERR, "Rx length error");
		return;
	}
	bp = unmarshalParams(data, &memb->loc.params);
	Lchan = memb->rem.Lchan;
	if (unicastLchan(Lchan)) {
/*
	match Lchan expiry time and nakholdoff of reciprocal but keep
	nakmaxtime minimised because we're unicast. We also don't need to add
	any multicast groups to the socket.
*/
		Lchan->params.expiry_sec = memb->loc.params.expiry_sec;
		Lchan->params.nakmaxtime = Lchan->params.nakholdoff = 0;
	}
	
	bp = unmarshalTA(bp, &get_Rchan(memb)->owner->sdt.adhocAddr);
	
	/* update discovery adhoc info!! bad bad bad. why is this in an SDT packet? */
	(*membLcomp(memb)->sdt.membevent)(EV_DISCOVER, get_Rchan(memb)->owner, (void *)bp);
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Connect - wrapped message
*/

static void
rx_connect(const uint8_t *data, int length, member_t *memb)
{
	protocolID_t proto;

	LOG_FSTART(lgFCTY);
	if (length != LEN_CONNECT) {   /* PDU is wrong length */
		acnlogmark(lgERR, "Rx length error");
		return;
	}
	if (memb->rem.mstate != MS_MEMBER) {
		acnlogmark(lgERR, "Rx no channel to reply");
		return;
	}
	proto = unmarshalU32(data + OFS_CONACCEPT_PROTO);
#if CONFIG_SDT_SINGLE_CLIENT
	if (proto != CONFIG_SDT_CLIENTPROTO) {
		sendConnrefuse(memb, proto, SDT_REASON_NO_RECIPIENT);
		return;
	}
	if ((memb->rem.Lchan->flags & CHF_NOAUTOCON) /* currently unsupported */
		|| !(memb->connect & CX_SDT))
	{
		sendConnrefuse(memb, proto, SDT_REASON_NONSPEC);
		return;
	}
	sendConnaccept(memb);
	if ((memb->connect & ~CX_SDT) == 0)
		(*membLcomp(memb)->sdt.membevent)(EV_CONNECT, memb->rem.Lchan, memb);
	memb->connect |= CX_REM;
#else
#endif 
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Connect Accept - wrapped message
*/

static void
rx_conaccept(const uint8_t *data, int length, member_t *memb)
{
	protocolID_t proto;

	LOG_FSTART(lgFCTY);
	if (length != LEN_CONACCEPT) {   /* PDU is wrong length */
		acnlogmark(lgERR, "Rx length error");
		return;
	}
	proto = unmarshalU32(data + OFS_CONACCEPT_PROTO);
#if CONFIG_SDT_SINGLE_CLIENT
	if (proto != CONFIG_SDT_CLIENTPROTO) {
		acnlogmark(lgERR, "Rx unknown protocol %u", proto);
		return;
	}
	if ((memb->connect & ~CX_SDT) == 0)
		(*membLcomp(memb)->sdt.membevent)(EV_CONNECT, memb->rem.Lchan, memb);
	memb->connect |= CX_LOC;
#else
#endif 
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Connect Refuse - wrapped message
*/

static void
rx_conrefuse(const uint8_t *data, int length, member_t *memb)
{
	protocolID_t proto;

	LOG_FSTART(lgFCTY);
	if (length != LEN_CONREFUSE) {   /* PDU is wrong length */
		acnlogmark(lgERR, "Rx length error");
		return;
	}
	proto = unmarshalU32(data + OFS_CONREFUSE_PROTO);
#if CONFIG_SDT_SINGLE_CLIENT
	if (proto != CONFIG_SDT_CLIENTPROTO) {
		acnlogmark(lgERR, "Rx unknown protocol %u", proto);
		return;
	}
	memb->connect &= ~CX_LOC;
#else
#endif 
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Disconnect - wrapped message
*/

static void
rx_disconnect(const uint8_t *data, int length, member_t *memb)
{
	protocolID_t proto;

	LOG_FSTART(lgFCTY);
	if (length != LEN_DISCONNECT) {   /* PDU is wrong length */
		acnlogmark(lgERR, "Rx length error");
		return;
	}
	proto = unmarshalU32(data + OFS_DISCONNECT_PROTO);
#if CONFIG_SDT_SINGLE_CLIENT
	if (proto != CONFIG_SDT_CLIENTPROTO) {
		acnlogmark(lgERR, "Rx unknown protocol %u", proto);
		return;
	}
	if ((memb->connect & ~CX_SDT) == CX_REM)
		(*membLcomp(memb)->sdt.membevent)(EV_REMDISCONNECT, memb->rem.Lchan, memb);
	memb->connect &= ~CX_REM;
#else
#endif 
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Disconnecting - wrapped message
*/

static void
rx_disconnecting(const uint8_t *data, int length, member_t *memb)
{
	protocolID_t proto;

	LOG_FSTART(lgFCTY);
	if (length != LEN_DISCONNECTING) {   /* PDU is wrong length */
		acnlogmark(lgERR, "Rx length error");
		return;
	}
	proto = unmarshalU32(data + OFS_DISCONNECTING_PROTO);
#if CONFIG_SDT_SINGLE_CLIENT
	if (proto != CONFIG_SDT_CLIENTPROTO) {
		acnlogmark(lgERR, "Rx unknown protocol %u", proto);
		return;
	}
	if ((memb->connect & ~CX_SDT) == CX_LOC)
		(*membLcomp(memb)->sdt.membevent)(EV_REMDISCONNECT, memb->rem.Lchan, memb);
	memb->connect &= ~CX_LOC;
#else
#endif 
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
ACK - wrapped message relating to reciprocal channel
*/

static void
rx_ack(const uint8_t *data, int length, member_t *memb)
{
	int32_t Rseq;

	LOG_FSTART(lgFCTY);
	if (length != LEN_ACK) {   /* PDU is wrong length */
		acnlogmark(lgERR, "Rx length error");
		return;
	}

/*
	are waiting for an ACK to complete Join?
	Note ACK may arrive before Join Accept.
*/
	Rseq = unmarshalSeq(data + OFS_ACK_RSEQ);
	acnlogmark(lgINFO, "Rx ACK Lchan %" PRIu16 ", mid %" PRIu16 ", Rseq %" PRIu32,
		memb->rem.Lchan->chanNo, memb->rem.mid, Rseq);

	switch (memb->rem.mstate) {
	case MS_MEMBER:
		updateRmembSeq(memb, Rseq);
		break;
	case MS_JOINPEND:
		cancel_timer(&memb->rem.stateTimer);
		memb->rem.stateTimer.action = makTimeoutAction;
		memb->rem.Rseq = Rseq;
		memb->rem.mstate = MS_MEMBER;
		memb->rem.Lchan->ackcount++;
		if (memb->loc.mstate == MS_MEMBER) setFullMember(memb);
		break;
	case MS_NULL:
	case MS_JOINRQ:
	default:
		acnlogmark(lgERR, "Rx ack from member in %s state", jstates[memb->rem.mstate]);
		return;
	}
	memb->rem.maktries = MAK_MAX_RETRIES + 1;
	acnlogmark(lgDBUG, "set MAK in %hu", Lchan_KEEPALIVE_ms(memb->rem.Lchan));
	set_timer(&memb->rem.stateTimer, timerval_ms(Lchan_KEEPALIVE_ms(memb->rem.Lchan)));
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Get sessions - adhoc level 1 message
*/

static void
rx_getSessions(const uint8_t *data UNUSED, int length, rxcontext_t *rcxt)
{
//   UNUSED_ARG(data)
	LOG_FSTART(lgFCTY);
	if (length != LEN_GETSESS) {   /* PDU is wrong length */
		acnlogmark(lgERR, "Rx length error");
		return;
	}
	sendSessions(ctxtLcomp, &rcxt->netx.source);
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Sessions - adhoc level 1 message

Response to Get Sessions
*/
#define MAXADDRLEN 128

static void
rx_sessions(const uint8_t *data, int length, rxcontext_t *rcxt)
{
	char tmpstr[MAXADDRLEN];
	const uint8_t *sp;
	const uint8_t *ep;
	uint16_t mid;
	int i;
	uint16_t count;
	uint32_t protocol;
	int j;

	LOG_FSTART(lgFCTY);
	acnlog(LOG_SESS, "Session list received from %s",
		uuid2str(rcxt->rlp.srcCID, tmpstr));
	ep = data + length;
	i = 1;
	for (sp = data; sp < ep;) {
		mid = unmarshalU16(sp);
		sp += 2;
		if (mid == 0) {
			acnlog(LOG_SESS, "  Owner channel %u", unmarshalU16(sp));
			sp += 2;
			sp = printTaddr(sp, tmpstr);
			acnlog(LOG_SESS, "    destination address: %s", tmpstr);
			if (sp == NULL) return;
			sp = printTaddr(sp, tmpstr);
			acnlog(LOG_SESS, "    source address: %s", tmpstr);
		} else {
			uuid2str(sp, tmpstr);
			sp += 16;
			acnlog(LOG_SESS, "  Member %u of channel %u from %s",
					mid, unmarshalU16(sp), tmpstr);
			sp += 2;
			sp = printTaddr(sp, tmpstr);
			acnlog(LOG_SESS, "    destination address: %s", tmpstr);
			if (sp == NULL) return;
			sp = printTaddr(sp, tmpstr);
			acnlog(LOG_SESS, "    source address: %s", tmpstr);
			acnlog(LOG_SESS, "    reciprocal channel %u", unmarshalU16(sp));
			sp += 2;
		}
		count = unmarshalU16(sp); sp += 2;
		acnlog(LOG_SESS, "    %u sessions (protocols):", count);
		for (j = 1; j <= count; ++j) {
			protocol = unmarshalU32(sp); sp += 4;
			acnlog(LOG_SESS, "    %u: %s", j, showProtocol(protocol, tmpstr));
		}
	}
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Send message functions
*/
/**********************************************************************/
/*
send a JOIN
Two cases:
  - reciprocal, initiated by incoming join - rcxt is valid and
	 we can use memb->loc and Rchan
  - leader, initiated by app - rcxt is NULL
	 memb->loc is not valid, Rchan doesn't exist
*/
static int
sendJoin(
	Lchannel_t *Lchan,
	member_t *memb
)
{
	uint8_t *bp;
	int rslt;
	Rcomponent_t *Rcomp;
	netx_addr_t *dest;
	uint8_t tatype;
	int tasize;
	uint8_t *txbuf;

	LOG_FSTART(lgFCTY);
	tatype = SDT_TA_TYPE(&Lchan->outwd_ad);
	tasize = supportedTAsize(tatype);
	assert(tasize >= 0);

	if (memb->rem.mstate > MS_JOINRQ)
		acnlogmark(lgERR, "Tx member is in %s state", jstates[memb->rem.mstate]);

	Rcomp = memb->rem.Rcomp;

	txbuf = new_txbuf(PKT_JOIN_OUT);
	if (txbuf == NULL) return -1;

	bp = txbuf + RLP_OFS_PDU1DATA + 2;  /* leave space for flags/vector */

	bp = marshalU8(bp, SDT_JOIN);
	bp = marshaluuid(bp, Rcomp->hd.uuid);
	bp = marshalU16(bp, memb->rem.mid);
	bp = marshalU16(bp, Lchan->chanNo);
	/* reciprocal channel - only set if recip join */
	bp = marshalU16(bp, (get_Rchan(memb)) ? get_Rchan(memb)->chanNo : 0);
	bp = marshalSeq(bp, Lchan->Tseq);
	bp = marshalSeq(bp, Lchan->Rseq);

	bp = marshalTA(bp, &Lchan->outwd_ad);

	bp = marshalParams(bp, &(Lchan->params));
	bp = marshalU8(bp, LchanOwner(Lchan)->sdt.expiry);

	if (get_Rchan(memb)) {
		acnlogmark(lgDBUG, "Tx sending to recip inbound");
		dest = &get_Rchan(memb)->inwd_ad;
	}
	else {
		acnlogmark(lgDBUG, "Tx sending to adhoc");
		dest = &Rcomp->sdt.adhocAddr;
	}

	rslt = sdt1_sendbuf(txbuf, bp - txbuf, 
								Lchan->inwd_sk, 
								dest,
								LchanOwner(Lchan)->hd.uuid);

	free_txbuf(txbuf, PKT_JOIN_OUT);
	LOG_FEND(lgFCTY);
	return rslt;
}

/**********************************************************************/

static int
sendJoinAccept(
	Rchannel_t *Rchan,
	member_t *memb
)
{
	uint8_t *txbuf;
	uint8_t *bp;
	int rslt;
	
	LOG_FSTART(lgFCTY);
	txbuf = new_txbuf(PKT_JACCEPT);
	if (txbuf == NULL) return -1;

	bp = txbuf + RLP_OFS_PDU1DATA + 2;  /* leave space for flags/vector */

	bp = marshalU8(bp, SDT_JOIN_ACCEPT);
	bp = marshaluuid(bp, Rchan->owner->hd.uuid);
	bp = marshalU16(bp, Rchan->chanNo);
	bp = marshalU16(bp, memb->loc.mid);
	bp = marshalSeq(bp, Rchan->Rseq);
	bp = marshalU16(bp, memb->rem.Lchan->chanNo);

	rslt = sdt1_sendbuf(txbuf, bp - txbuf, 
								Rchan->outwd_sk, 
								&Rchan->inwd_ad,
								LchanOwner(memb->rem.Lchan)->hd.uuid);

	free_txbuf(txbuf, PKT_JACCEPT);
	LOG_FEND(lgFCTY);
	return rslt;
}

/**********************************************************************/
/*
	sendJoinRefuse() context may be that of an incoming JoinRefuse or a timeout.
*/

static int
sendJoinRefuse(member_t *memb, uint8_t refuseCode)
{
	uint8_t *txbuf;
	uint8_t *bp;
	Lcomponent_t *Lcomp;
	int rslt;

	LOG_FSTART(lgFCTY);
#if CONFIG_SINGLE_COMPONENT
	Lcomp = &localComponent;
#else
	Lcomp = memb->loc.Lcomp;
#endif
	if (get_Rchan(memb) == NULL) return -1;

	txbuf = new_txbuf(PKT_JREFUSE);
	if (txbuf == NULL) return -1;

	bp = txbuf + RLP_OFS_PDU1DATA + 2;  /* leave space for flags/vector */

	bp = marshalU8(bp, SDT_JOIN_REFUSE);
	bp = marshaluuid(bp, get_Rchan(memb)->owner->hd.uuid);
	bp = marshalU16(bp, get_Rchan(memb)->chanNo);
	bp = marshalU16(bp, memb->loc.mid);
	bp = marshalSeq(bp, get_Rchan(memb)->Rseq);
	bp = marshalU8(bp, refuseCode);

	rslt = sdt1_sendbuf(txbuf, bp - txbuf, 
								membLcomp(memb)->sdt.adhoc,
								&get_Rchan(memb)->inwd_ad,
								membLcomp(memb)->hd.uuid);

	free_txbuf(txbuf, PKT_JREFUSE);
	LOG_FEND(lgFCTY);
	return rslt;
}

/**********************************************************************/
/*
	sendJoinRefuseData() takes it's content from the Join message rather
	than member and channel structures because their state or existence
	cannot be relied on. However, we do know that data is at least a
	sensible length.
*/
static int
sendJoinRefuseData(
	const uint8_t *joinmsg,
	uint8_t refuseCode,
	rxcontext_t *rcxt
)
{
	uint8_t *txbuf;
	uint8_t *bp;
	int rslt;
	
	LOG_FSTART(lgFCTY);
	txbuf = new_txbuf(PKT_JREFUSE);
	if (txbuf == NULL) return -1;

	bp = txbuf + RLP_OFS_PDU1DATA + 2;  /* leave space for flags/vector */
	bp = marshalU8(bp, SDT_JOIN_REFUSE);
	bp = marshaluuid(bp, rcxt->rlp.srcCID);
	bp = marshalBytes(bp, joinmsg + OFS_JOIN_CHANNO, 2);
	bp = marshalBytes(bp, joinmsg + OFS_JOIN_MID, 2);
	bp = marshalBytes(bp, joinmsg + OFS_JOIN_RSEQ, 4);
	bp = marshalU8(bp, refuseCode);

	rslt = sdt1_sendbuf(txbuf, bp - txbuf, 
								ctxtLcomp->sdt.adhoc, 
								&rcxt->netx.source,
								ctxtLcomp->hd.uuid);

	free_txbuf(txbuf, PKT_JREFUSE);
	LOG_FEND(lgFCTY);
	return rslt;
}

/**********************************************************************/
/*
	sendLeaving()
*/

static int
sendLeaving(member_t *memb, uint8_t reason)
{
	uint8_t *txbuf;
	uint8_t *bp;
	Lcomponent_t *Lcomp;
	int rslt;

	LOG_FSTART(lgFCTY);
#if CONFIG_SINGLE_COMPONENT
	Lcomp = &localComponent;
#else
	Lcomp = memb->loc.Lcomp;
#endif
	if (get_Rchan(memb) == NULL) return -1;

	txbuf = new_txbuf(PKT_LEAVING);
	if (txbuf == NULL) return -1;

	bp = txbuf + RLP_OFS_PDU1DATA + 2;  /* leave space for flags/vector */

	bp = marshalU8(bp, SDT_LEAVING);
	bp = marshaluuid(bp, get_Rchan(memb)->owner->hd.uuid);
	bp = marshalU16(bp, get_Rchan(memb)->chanNo);
	bp = marshalU16(bp, memb->loc.mid);
	bp = marshalSeq(bp, get_Rchan(memb)->Rseq);
	bp = marshalU8(bp, reason);

	rslt = sdt1_sendbuf(txbuf, bp - txbuf, 
								membLcomp(memb)->sdt.adhoc,
								&get_Rchan(memb)->inwd_ad,
								membLcomp(memb)->hd.uuid);

	free_txbuf(txbuf, PKT_LEAVING);
	LOG_FEND(lgFCTY);
	return rslt;
}

/**********************************************************************/
/*
Send a list  of our sessions
*/
/* sizes defined for IPv4 and CONFIG_SDT_SINGLE_CLIENT */
#define MAX_OWNER_BLOCKSIZE (4 + (1 + LEN_TA_IPV4) * 2 + 2 + 4)
#define MAX_MEMBER_BLOCKSIZE (20 + (1 + LEN_TA_IPV4) * 2 + 4 + 4)

static int
sendSessions(Lcomponent_t *Lcomp, netx_addr_t *dest)
{
	Lchannel_t *Lchan;
	uint8_t *txbuf;
	uint8_t *bp;
	uint8_t *ep;
	uint16_t sessions;
	netx_addr_t addr;
	member_t *memb;
	uint16_t mid;

	LOG_FSTART(lgFCTY);
	txbuf = new_txbuf(MAX_MTU);
	if (txbuf == NULL) return -1;
	ep = txbuf + MAX_MTU - MAX_MEMBER_BLOCKSIZE;
	bp = NULL;

	for (Lchan = Lcomp->sdt.Lchannels; Lchan; Lchan = Lchan->lnk.r) {
		sessions = 0;
		/* loop once with mid == 0 for Lchan then once for each member */
		for (mid = 0; mid <= Lchan->himid; ++mid) {
			if (bp == NULL) {
				bp = txbuf + RLP_OFS_PDU1DATA + 2;  /* leave space for flags/vector */
				bp = marshalU8(bp, SDT_SESSIONS);
			}

			if (mid == 0) {
				bp = marshalU16(bp, 0);
				bp = marshalU16(bp, Lchan->chanNo);
				bp = marshalTA(bp, &Lchan->outwd_ad);
				netxGetMyAddr(Lchan->inwd_sk, &addr);
				bp = marshalTA(bp, &addr);
				if (sessions) {
					marshalU16(bp, 1);
					marshalU32(bp, CONFIG_SDT_CLIENTPROTO);
				} else {
					marshalU16(bp, 0);
				}
			} else {
				if ((memb = findRmembMID(Lchan, mid)) == NULL) continue;
				bp = marshalU16(bp, memb->loc.mid);
				bp = marshaluuid(bp, memb->rem.Rcomp->hd.uuid);
				bp = marshalU16(bp, get_Rchan(memb)->chanNo);
				bp = marshalTA(bp, &get_Rchan(memb)->outwd_ad);
				bp = marshalTA(bp, &get_Rchan(memb)->inwd_ad);
				bp = marshalU16(bp, memb->rem.Lchan->chanNo);
				sessions |= memb->connect;
				if (memb->connect) {
					marshalU16(bp, 1);
					marshalU32(bp, CONFIG_SDT_CLIENTPROTO);
				} else {
					marshalU16(bp, 0);
				}
			}

			if (bp > ep || (Lchan->lnk.r == NULL && mid == Lchan->himid)) {
				sdt1_sendbuf(txbuf, bp - txbuf, Lcomp->sdt.adhoc, dest, Lcomp->hd.uuid);
				bp = NULL;
			}
		}
	}

	free_txbuf(txbuf, MAX_MTU);
	LOG_FEND(lgFCTY);
	return 0;
}

/**********************************************************************/
/*
Send a NAK
Only call this when timeouts have failed - construct message using latest
available data
	16	Leader CID
	2	ChanNo
	2	MID
	4	RSeq
	4	FirstMissed
	4	LastMissed

if suppress is true we do everything except actually send the packet
*/
static void
sendNAK(Rchannel_t *Rchan, bool suppress)
{
	uint8_t *txbuf;
	uint8_t *bp;
	member_t *INITIALIZED(memb);
	uint8_t maxexp;
	bool nakout;
	if_MANYCOMP(member_t *mp;)

	LOG_FSTART(lgFCTY);

#if CONFIG_SINGLE_COMPONENT
	memb = firstMemb(Rchan);
	maxexp = memb->loc.params.expiry_sec;
	nakout = (memb->loc.params.flags & NAK_OUTBOUND) != 0;
#else
	maxexp = 0;
	nakout = false;
	forEachMemb(mp, Rchan) {
		if (mp->loc.params.expiry_sec > maxexp) {
			maxexp = mp->loc.params.expiry_sec;
			memb = mp;
		}
		nakout = nakout || (mp->loc.params.flags & NAK_OUTBOUND);
	}
#endif

	Rchan->NAKstate = NS_NAKWAIT;
	Rchan->NAKtimer.action = NAKfailAction;
	set_timer(&Rchan->NAKtimer, FTIMEOUT_ms(maxexp, NAK_TIMEOUT_FACTOR));

	if (!suppress) {
		int rslt;

		txbuf = new_txbuf(PKT_NAK);

		bp = txbuf + RLP_OFS_PDU1DATA + OFS_VECTOR;
		bp = marshalU8(bp, SDT_NAK);
		bp = marshaluuid(bp, Rchan->owner->hd.uuid);
		bp = marshalU16(bp, Rchan->chanNo);
		bp = marshalU16(bp, memb->loc.mid);
		bp = marshalSeq(bp, Rchan->Rseq);
		bp = marshalSeq(bp, Rchan->Rseq + 1);
		bp = marshalSeq(bp, Rchan->lastnak);

		rslt = sdt1_sendbuf(txbuf, bp - txbuf, 
									memb->rem.Lchan->inwd_sk,
									&Rchan->inwd_ad,
									membLcomp(memb)->hd.uuid);
		if (rslt >= 0 && nakout) {
			sdt1_sendbuf(txbuf, bp - txbuf, 
									memb->rem.Lchan->inwd_sk,
									&Rchan->outwd_ad,
									membLcomp(memb)->hd.uuid);
		}

		free_txbuf(txbuf, PKT_NAK);
		acnlogmark(lgDBUG, "Tx NAK %" PRIu32 " - %" PRIu32, 
				Rchan->Rseq + 1, Rchan->lastnak);
	}
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Resend NAKed wrappers
If more than one, try to condense them into a single packet
*/
static void
resendWrappers(Lchannel_t *Lchan, int32_t first, int32_t last)
{
	txwrap_t *txwrap;
	uint8_t *txbuf = NULL;
	uint8_t *bp;
	uint8_t *wp;
	int32_t oldestavail;

	LOG_FSTART(lgFCTY);
	oldestavail = Lchan->obackwrap->st.sent.Rseq;

	for (txwrap = Lchan->obackwrap; txwrap; txwrap = txwrap->st.sent.lnk.r) {
		if ((txwrap->st.sent.Rseq - first) < 0) continue;
		if ((txwrap->st.sent.Rseq - last) > 0) break;
		if ((txwrap->st.sent.Rseq - Lchan->nakfirst) >= 0
			&& (txwrap->st.sent.Rseq - Lchan->naklast) < 0) continue;
		wp = txwrap->txbuf + RLP_OFS_PDU1DATA;
		if (txbuf && (bp + (txwrap->endp - wp)) > (txbuf + txwrap->size)) {
			/* need to flush */
			if (rlp_sendbuf(txbuf, bp - txbuf, Lchan->inwd_sk,
							&Lchan->outwd_ad, LchanOwner(Lchan)->hd.uuid) < 0
				) acnlogerror(lgERR);
			free_txbuf(txbuf, MAX_MTU);
			txbuf = NULL;
		}
		if (txbuf == NULL && (txwrap->st.sent.Rseq - last) < 0) {
			txbuf = new_txbuf(MAX_MTU);
			bp = txbuf + RLP_OFS_PDU1DATA;
		}
		acnlogmark(lgDBUG, "Tx resend %" PRIu32, txwrap->st.sent.Rseq);
		if (txbuf) {
			memcpy(bp, wp, (txwrap->endp - wp));
			marshalSeq(bp + SDT_OFS_PDU1DATA + OFS_WRAPPER_OLDEST, oldestavail);
			bp += txwrap->endp - wp;
		} else {
			bp = txwrap->txbuf + RLP_OFS_PDU1DATA;
			marshalSeq(bp + SDT_OFS_PDU1DATA + OFS_WRAPPER_OLDEST, oldestavail);
			if (rlp_sendbuf(txwrap->txbuf, txwrap->endp - txwrap->txbuf,
							Lchan->inwd_sk, &Lchan->outwd_ad,
							LchanOwner(Lchan)->hd.uuid) < 0
				) acnlogerror(lgERR);
		}
	}
	if (txbuf) {
		if (rlp_sendbuf(txbuf, bp - txbuf, Lchan->inwd_sk,
						&Lchan->outwd_ad, LchanOwner(Lchan)->hd.uuid) < 0
			) acnlogerror(lgERR);
		free_txbuf(txbuf, MAX_MTU);
	}
	if ((first - Lchan->nakfirst) < 0) Lchan->nakfirst = first;
	if ((last - Lchan->naklast) >= 0) Lchan->naklast = last + 1;
	set_timer(&Lchan->blankTimer, timerval_ms(NAK_BLANKTIME(Lchan->params.nakholdoff)));
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
First ACK for a new channel - if multicast set a re-send timer
*/
#define ALEN_ACK SDTW_AOFS_CB_PDU1DATA + SDT_OFS_PDU1DATA + LEN_ACK

static void
firstACK(member_t *memb)
{
	txwrap_t *txwrap;
	
	LOG_FSTART(lgFCTY);
	if ((txwrap = justACK(memb, true)) == NULL) {
		acnlogmark(lgNTCE, "Tx can't repeat first ACK");
		return;
	}
	inittimer(&txwrap->st.fack.rptTimer);
	txwrap->st.fack.rptTimer.action = prememberAction;
	txwrap->st.fack.rptTimer.userp = memb;
	txwrap->st.fack.t_ms = FIRSTACK_REPEAT_ms;
	slAddHead(firstacks, txwrap, st.fack.lnk);
	set_timer(&txwrap->st.fack.rptTimer, timerval_ms(FIRSTACK_REPEAT_ms));
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
static txwrap_t *
justACK(member_t *memb, bool keep)
{
	txwrap_t *txwrap;
	uint8_t *bp;
	int rslt;

	LOG_FSTART(lgFCTY);
	txwrap = startWrapper(memb->rem.Lchan, SDT_OFS_PDU1DATA + LEN_ACK,
						WRAP_REL_OFF | WRAP_NOAUTOACK);
	if (txwrap == NULL) return NULL;

	rslt = SDT_OFS_PDU1DATA + LEN_ACK;
	bp = startProtoMsg(txwrap, memb, SDT_PROTOCOL_ID, &rslt, WRAP_REPLY);
	if (bp == NULL) {
		cancelWrapper(txwrap);
		acnlogerror(lgERR);
		return NULL;
	}

	bp = marshalU16(bp, SDT_OFS_PDU1DATA + LEN_ACK + FIRST_FLAGS);
	bp = marshalU8(bp, SDT_ACK);
	bp = marshalSeq(bp, get_Rchan(memb)->Rseq);
	if (endProtoMsg(txwrap, bp) < 0) {
		cancelWrapper(txwrap);
		acnlogerror(lgERR);
		return NULL;
	}

	if (keep) ++txwrap->usecount;
	flushWrapper(txwrap, NULL);
	memb->loc.lastack = get_Rchan(memb)->Rseq;
	LOG_FEND(lgFCTY);
	return (keep) ? txwrap : NULL;
}

/**********************************************************************/
static int
emptyWrapper(Lchannel_t *Lchan, uint16_t flags)
{
	txwrap_t *txwrap;

//   LOG_FSTART(lgFCTY);
	if ((txwrap = startWrapper(Lchan, 0, flags)) == NULL)
		return -1;
//   LOG_FEND(lgFCTY);
	return flushWrapper(txwrap, NULL);
}

/**********************************************************************/
#if CONFIG_SDT_SINGLE_CLIENT
static int
connectAll(Lchannel_t *Lchan)
{
	member_t *memb;
	txwrap_t *txwrap;
	uint16_t i;

	LOG_FSTART(lgFCTY);
	txwrap = NULL;

	for (i = 0; i < Lchan->himid; ++i) {
		if ((memb = getRmemb(Lchan, i)) == NULL) continue;
		if (memb->connect == CX_SDT) {
			txwrap = nextSDTmsg(txwrap, memb, connect_msg, WRAP_REL_ON | _WRAP_RPT);
			memb->connect |= CX_LOC;
		}
	}
	if (txwrap) return flushWrapper(txwrap, NULL);
	LOG_FEND(lgFCTY);
	return 0;
}
#else
	/* FIXME implement !CONFIG_SDT_SINGLE_CLIENT */
#endif

/**********************************************************************/
#if CONFIG_SDT_SINGLE_CLIENT

static int
disconnectAll(Lchannel_t *Lchan, uint8_t reason, bool eject)
{
	member_t *memb;
	txwrap_t *txwrap;
	uint16_t wflags;
	uint16_t i;

	LOG_FSTART(lgFCTY);
	txwrap = NULL;
	wflags = WRAP_REL_ON | _WRAP_RPT;

	for (i = 0; i < Lchan->himid; ++i) {
		if ((memb = getRmemb(Lchan, i)) == NULL) continue;
		if ((memb->connect & CX_LOC)) {
			memb->connect &= ~CX_LOC;
			txwrap = nextSDTmsg(txwrap, memb, disconnect_msg, wflags);
			if (memb->connect == 0)
				(*membLcomp(memb)->sdt.membevent)(EV_LOCDISCONNECT, Lchan, memb);
		}
	}
	disconnecting_msg[7] = reason;
	wflags = WRAP_REL_ON | WRAP_REPLY;
	for (i = 0; i < Lchan->himid; ++i) {
		if ((memb = getRmemb(Lchan, i)) == NULL) continue;
		if ((memb->connect & CX_REM))
		{
			memb->connect &= ~CX_REM;
			txwrap = nextSDTmsg(txwrap, memb, disconnecting_msg, wflags);
			wflags |= _WRAP_RPT;
			if (memb->connect == 0)
				(*membLcomp(memb)->sdt.membevent)(EV_LOCDISCONNECT, memb->rem.Lchan, memb);
		}
	}
	if (eject && Lchan->membercount) {
		/* memb is still valid from last loop */
		txwrap = nextSDTmsg(txwrap, memb, disconnecting_msg, _WRAP_ALL | _WRAP_CLOSE);
	}
	if (txwrap) return flushWrapper(txwrap, NULL);
	LOG_FEND(lgFCTY);
	return 0;
}
#else
	/* FIXME implement !CONFIG_SDT_SINGLE_CLIENT */
#endif

/**********************************************************************/
/*
Wrapped message sending
*/
/**********************************************************************/
/*
Start a txwrap
*/
txwrap_t *
startWrapper(Lchannel_t *Lchan, int size, uint16_t wflags)
{
	uint8_t *txbuf;
	txwrap_t *txwrap;

	LOG_FSTART(lgFCTY);
	if (size >= (MAX_MTU - SDTW_AOFS_CB_PDU1DATA)) {
		errno = EMSGSIZE; /* EOVERFLOW ERANGE ? */
		return NULL;
	} else if (size < 0) size = DEFAULT_MTU;
	else size += SDTW_AOFS_CB_PDU1DATA;

	if ((txwrap = new_txwrap()) == NULL) return NULL;
	if ((txbuf = new_txbuf((unsigned)size)) == NULL) {
		free_txwrap(txwrap);
		return NULL;
	}
	txwrap->txbuf = txbuf;
	txwrap->size = size;
	txwrap->endp = txbuf + SDTW_AOFS_CB;
	txwrap->st.open.prevflags = wflags & (WRAP_REL_ON | WRAP_REL_OFF | WRAP_NOAUTOACK);
	txwrap->st.open.Lchan = Lchan;
	txwrap->usecount = 1;
	
	LOG_FEND(lgFCTY);
	return txwrap;
}

/**********************************************************************/
/*
Start a txwrap given a member
*/
txwrap_t *
startMemberWrapper(member_t *memb, int size, uint16_t wflags)
{
	return startWrapper(memb->rem.Lchan, size, wflags);
}

/**********************************************************************/
/*
Cancel a txwrap
*/
void
cancelWrapper(txwrap_t *txwrap)
{
	LOG_FSTART(lgFCTY);
	if (--txwrap->usecount == 0) {
		free_txbuf(txwrap->txbuf, txwrap->size);
		free_txwrap(txwrap);
	}
	LOG_FEND(lgFCTY);
}


/**********************************************************************/
/*
Initialize the next message block in the wrapper
*/

uint8_t *
startProtoMsg(txwrap_t *txwrap, member_t *memb, protocolID_t proto, int *sizep, uint16_t wflags)
{
	uint8_t *bp;
	uint16_t mid;
	uint16_t assoc;
	uint8_t pduflags;

	if (txwrap->st.open.Lchan != memb->rem.Lchan) {
		errno = ENXIO;
		return NULL;
	}

	/* These flags are cumulative */
	wflags |= (txwrap->st.open.prevflags & (WRAP_REL_ON | WRAP_REL_OFF | WRAP_NOAUTOACK));
	if ((wflags & WRAP_REL_BOTH) == WRAP_REL_BOTH
		|| (memb == WRAP_ALL_MEMBERS
			&& (wflags & WRAP_REPLY)))
	{
		errno = EINVAL; /* ENOBUFS ENOMSG */
		return NULL;
	}
	txwrap->st.open.flags = wflags;

	mid = ALL_MEMBERS;
	assoc = 0;
	if (memb != WRAP_ALL_MEMBERS) {
		mid = memb->rem.mid;
		if (wflags & WRAP_REPLY) assoc = get_Rchan(memb)->chanNo;
	}

	bp = txwrap->endp;
	assert(bp >= (txwrap->txbuf + SDTW_AOFS_CB));
	assert(proto != 0);

	pduflags = DATA_bFLAG;
	bp += OFS_VECTOR;
	if (mid != txwrap->st.open.prevmid) {
		pduflags |= VECTOR_bFLAG;
		bp = marshalU16(bp, mid);
	}
	if (proto != txwrap->st.open.prevproto || assoc != txwrap->st.open.prevassoc) {
		pduflags |= HEADER_bFLAG;
		bp = marshalU32(bp, proto);
		bp = marshalU16(bp, assoc);
	}
	marshalU8(txwrap->endp, pduflags);

	if (sizep) {
		if (*sizep >= 0 && (bp + *sizep) > (txwrap->txbuf + txwrap->size)) {
			errno = EMSGSIZE;
			return NULL;
		}
		*sizep = txwrap->txbuf + txwrap->size - bp;
	}
	return bp;
}

/**********************************************************************/
txwrap_t *
initMemberMsg(member_t *memb, protocolID_t proto, int *sizep, uint16_t wflags, uint8_t **msgp)
{
	txwrap_t *txwrap;
	int size;

	if (msgp == NULL) {
		errno = EINVAL; /* ENOBUFS ENOMSG */
		return NULL;
	}
	size = -1;
	if (sizep) size = *sizep;
	if ((txwrap = startWrapper(memb->rem.Lchan, size, wflags)) == NULL)
		return NULL;
	*msgp = startProtoMsg(txwrap, memb, proto, sizep, wflags);
	return txwrap;
}

/**********************************************************************/
/*
Repeat the previous message (and flags) to another member
*/
int
rptProtoMsg(txwrap_t *txwrap, member_t *memb)
{
	uint16_t lenflags;
	uint8_t *bp;
	uint16_t mid;
	uint16_t assoc;

	if (txwrap->st.open.Lchan != memb->rem.Lchan) {
		errno = ENXIO;
		return -1;
	}
	
	if (txwrap->endp <= (txwrap->txbuf + SDTW_AOFS_CB)
		|| (memb == WRAP_ALL_MEMBERS
			&& (txwrap->st.open.prevflags & WRAP_REPLY)))
	{
		errno = EINVAL;
		return -1;
	}

	mid = ALL_MEMBERS;
	assoc = 0;
	if (memb != WRAP_ALL_MEMBERS) {
		mid = memb->rem.mid;
		if (txwrap->st.open.prevflags & WRAP_REPLY) assoc = get_Rchan(memb)->chanNo;
	}

	lenflags = 0;
	if (mid != txwrap->st.open.prevmid) {
		lenflags += VECTOR_FLAG + 2;
	}
	if (assoc != txwrap->st.open.prevassoc) {
		lenflags += HEADER_FLAG + 6;
	}
	if ((txwrap->endp + (lenflags & LENGTH_MASK)) > (txwrap->txbuf + txwrap->size)) {
		errno = EMSGSIZE;
		return -1;
	}

	bp = txwrap->endp;
	bp = marshalU16(bp, lenflags);
	if ((lenflags & VECTOR_FLAG)) bp = marshalU16(bp, mid);
	if ((lenflags & HEADER_FLAG)) {
		bp = marshalU32(bp, txwrap->st.open.prevproto);
		bp = marshalU16(bp, assoc);
	}
	txwrap->endp = bp;
	return txwrap->txbuf + txwrap->size - bp;
}

/**********************************************************************/
/*
Endp points to the end of the PDUs you have added
*/
int
endProtoMsg(txwrap_t *txwrap, uint8_t *endp)
{
	uint16_t lenflags;
	uint8_t *bp;

	assert(endp >= txwrap->endp && endp <= (txwrap->txbuf + txwrap->size));

	bp = txwrap->endp;
	lenflags = unmarshalU8(bp) << 8;
	assert((lenflags & LENGTH_MASK) == 0); /* check for corruption */
	lenflags |= DATA_FLAG;

	lenflags += endp - bp;
	bp = marshalU16(bp, lenflags);

	if (lenflags & VECTOR_FLAG) {
		txwrap->st.open.prevmid = unmarshalU16(bp);
		bp += 2;
	}

	if (lenflags & HEADER_FLAG) {
		txwrap->st.open.prevproto = unmarshalU32(bp);
		bp += 4;
		txwrap->st.open.prevassoc = unmarshalU16(bp);
		bp += 2;
	}
	txwrap->st.open.prevflags = txwrap->st.open.flags;
	txwrap->endp = endp;
	return 0;
}

/**********************************************************************/
int
addProtoMsg(txwrap_t *txwrap, member_t *memb, protocolID_t proto,
				const uint8_t *data, int size, uint16_t wflags)
{
	uint16_t lenflags;
	uint8_t *bp;
	uint16_t mid;
	uint16_t assoc;

	acnlogmark(lgDBUG, "Tx %s size %d, code %d", __func__, size, data[2]);
	if (txwrap->st.open.Lchan != memb->rem.Lchan) {
		errno = ENXIO;
		return -1;
	}
	
	/* These flags are cumulative */
	wflags |= (txwrap->st.open.prevflags & (WRAP_REL_ON | WRAP_REL_OFF | WRAP_NOAUTOACK));
	if ((wflags & WRAP_REL_BOTH) == WRAP_REL_BOTH
		|| (memb == WRAP_ALL_MEMBERS
			&& (wflags & WRAP_REPLY)))
	{
		errno = EINVAL; /* ENOBUFS ENOMSG */
		return -1;
	}
	txwrap->st.open.flags = wflags;

	mid = ALL_MEMBERS;
	assoc = 0;
	if (memb != WRAP_ALL_MEMBERS) {
		mid = memb->rem.mid;
		if (wflags & WRAP_REPLY) assoc = get_Rchan(memb)->chanNo;
	}

	lenflags = size + 2 + DATA_FLAG;
	if (mid != txwrap->st.open.prevmid) {
		lenflags += VECTOR_FLAG + 2;
	}
	if (proto != txwrap->st.open.prevproto || assoc != txwrap->st.open.prevassoc) {
		lenflags += HEADER_FLAG + 6;
	}

	if ((txwrap->endp + (lenflags & LENGTH_MASK)) > (txwrap->txbuf + txwrap->size)) {
		errno = EMSGSIZE;
		return -1;
	}

	bp = txwrap->endp;
	bp = marshalU16(bp, lenflags);
	if ((lenflags & VECTOR_FLAG)) bp = marshalU16(bp, mid);
	if ((lenflags & HEADER_FLAG)) {
		bp = marshalU32(bp, proto);
		bp = marshalU16(bp, assoc);
	}
	bp = marshalBytes(bp, data, size);
	acnlogmark(lgDBUG, "Tx %" PRIuPTR " added", bp - txwrap->endp);
	txwrap->endp = bp;
	return txwrap->txbuf + txwrap->size - bp;
}

/**********************************************************************/
#define LEN_ACKEXTRA (OFS_CB_PDU1DATA + SDT_OFS_PDU1DATA + LEN_ACK)

int
flushWrapper(txwrap_t *txwrap, int32_t *Rseqp)
{
	uint8_t *bp;
	member_t *memb;
	uint8_t *endp;
	Lchannel_t *Lchan;
	uint8_t wraptype;
	uint32_t oldest;

	LOG_FSTART(lgFCTY);
	Lchan = txwrap->st.open.Lchan;
	if ((txwrap->st.open.prevflags & WRAP_NOAUTOACK) == 0
			&& Lchan->membercount)
	{
		uint16_t mid;
		int i;

		endp = txwrap->txbuf + txwrap->size - LEN_ACKEXTRA;
		bp = txwrap->endp;

		mid = Lchan->lastackmid;
		for (i = Lchan->himid; i > 0 && bp <= endp; --i) {
			if (++mid > Lchan->himid) mid = 1;

			if ((memb = findRmembMID(Lchan, mid))
				&& memb->loc.mstate == MS_MEMBER)
			{
				bp = marshalU16(bp, LEN_ACKEXTRA + FIRST_FLAGS);
				bp = marshalU16(bp, memb->rem.mid);
				bp = marshalU32(bp, SDT_PROTOCOL_ID);
				bp = marshalU16(bp, get_Rchan(memb)->chanNo);

				bp = marshalU16(bp, SDT_OFS_PDU1DATA + LEN_ACK + FIRST_FLAGS);
				bp = marshalU8(bp, SDT_ACK);
				bp = marshalSeq(bp, get_Rchan(memb)->Rseq);
				acnlogmark(lgDBUG, "Tx autoack MID %" PRIu16 " Rem seq %" PRIu32, mid, get_Rchan(memb)->Rseq);
				memb->loc.lastack = get_Rchan(memb)->Rseq;
			}
		}
		Lchan->lastackmid = mid;
		txwrap->endp = endp = bp;
	} else endp = txwrap->endp;

	wraptype = SDT_UNREL_WRAP;
	if ((txwrap->st.open.prevflags & WRAP_REL_ON)) {
		wraptype = SDT_REL_WRAP;
		++Lchan->Rseq;
	}

	bp = txwrap->txbuf + SDT1_OFS_LENFLG;

	bp = marshalU16(bp, (endp - bp) + FIRST_FLAGS);
	bp = marshalU8(bp, wraptype);
	bp = marshalU16(bp, Lchan->chanNo);
	Lchan->Tseq++;
	bp = marshalSeq(bp, Lchan->Tseq);
	bp = marshalSeq(bp, Lchan->Rseq);
	oldest = (Lchan->obackwrap)
					? Lchan->obackwrap->st.sent.Rseq
					: Lchan->Rseq + (wraptype == SDT_UNREL_WRAP);
	bp = marshalU32(bp, oldest);
	bp = setMAKs(bp, Lchan, txwrap->st.open.prevflags);

	acnlogmark(lgINFO, "Tx %cwrapper T=%" PRIu32 " R=%" PRIu32 " oldest=%" PRIu32,
							((wraptype == SDT_REL_WRAP) ? 'R' : 'U'),
							Lchan->Tseq, Lchan->Rseq, oldest);
	if (rlp_sendbuf(txwrap->txbuf, (endp - txwrap->txbuf),
							Lchan->inwd_sk, &Lchan->outwd_ad,
							LchanOwner(Lchan)->hd.uuid) < 0)
	{
		acnlogerror(lgERR);
	}

	if (Rseqp) *Rseqp = Lchan->Rseq;
	if (wraptype == SDT_REL_WRAP) {
		acnlogmark(lgINFO, "Tx Buffer rel wrap");
		txwrap->st.sent.Rseq = Lchan->Rseq;
		txwrap->st.sent.acks = Lchan->ackcount;
		stlAddTail(Lchan->obackwrap, Lchan->nbackwrap, txwrap, st.sent.lnk);
		++Lchan->backwraps;
	} else if (--txwrap->usecount == 0) {
		acnlogmark(lgDBUG, "Tx Discard unrel wrap");
		free_txbuf(txwrap->txbuf, txwrap->size);
		free_txwrap(txwrap);
	}
	acnlogmark(lgDBUG, "set keepalive %ums", Lchan->ka_t_ms);
	set_timer(&Lchan->keepalive, timerval_ms(Lchan->ka_t_ms));
	LOG_FEND(lgFCTY);
	return 0;
}

/**********************************************************************/
/*
*/
int
sendWrap(
	Lchannel_t *Lchan,
	member_t *memb,
	protocolID_t proto,
	const uint8_t *data,
	int size,
	uint16_t wflags
)
{
	txwrap_t *txwrap;
	int rslt;
	
	LOG_FSTART(lgFCTY);
	if ((txwrap = startWrapper(Lchan, -1, wflags)) == NULL)
		return -1;
	
	if (addProtoMsg(txwrap, memb, proto, data, size, wflags) < 0) {
		cancelWrapper(txwrap);
		acnlogerror(lgERR);
		return -1;
	}
	
	rslt = flushWrapper(txwrap, NULL);
	LOG_FEND(lgFCTY);
	return rslt;
}

/**********************************************************************/
static txwrap_t *
nextProtoMsg(
	txwrap_t *txwrap,
	member_t *memb,
	protocolID_t proto,
	const uint8_t *data,
	int size,
	uint16_t wflags
)
{
	Lchannel_t *Lchan;

	LOG_FSTART(lgFCTY);
	Lchan = memb->rem.Lchan;
	if (wflags & _WRAP_ALL) memb = WRAP_ALL_MEMBERS;

	if (txwrap && (txwrap->endp + OFS_CB_PDU1DATA + size) > (txwrap->txbuf + txwrap->size)) {
		if (flushWrapper(txwrap, NULL) < 0)
			acnlogerror(lgERR);
		txwrap = NULL;
	}
	if (txwrap == NULL) {
		if ((txwrap = startWrapper(Lchan, -1, wflags & _WRAP_MASK)) == NULL)
		{
			acnlogerror(lgERR);
			return NULL;
		}
		wflags &= ~_WRAP_RPT;
	}
	if (wflags & _WRAP_RPT) rptProtoMsg(txwrap, memb);
	else addProtoMsg(txwrap, memb, proto, data, size, wflags & _WRAP_MASK);

	if ((wflags & _WRAP_CLOSE)) {
		flushWrapper(txwrap, NULL);
		txwrap = NULL;
	}
	LOG_FEND(lgFCTY);
	return txwrap;
}

/**********************************************************************/
/*
NAK processing
*/
/**********************************************************************/
/*
Early check for whether oldest-available means we've lost sequence
*/

static int
NAKcheck(Rchannel_t *Rchan, const uint8_t *wrapdata)
{
	if (Rchan->NAKstate != NS_NULL
			&& (unmarshalSeq(wrapdata + OFS_WRAPPER_OLDEST) - Rchan->Rseq) > 1)
	{
		acnlogmark(lgERR, "Rx lost sequence");
		killRchan(Rchan, SDT_REASON_LOST_SEQUENCE, EV_LOSTSEQ);
		return -1;
	}
	return 0;
}

/**********************************************************************/
/*
T = min(NAKmaxwait, ((seqNo + MID) % NAKmodulus) * NAKholdoff),
*/
static void
NAKwrappers(Rchannel_t *Rchan)
{
	member_t *memb;
	uint32_t holdoff;
#if !CONFIG_SINGLE_COMPONENT
	uint32_t minhoff;
#endif

	LOG_FSTART(lgFCTY);
	acnlogmark(lgDBUG, "Tx NAK try %" PRIu8, Rchan->NAKtries);

#if CONFIG_SINGLE_COMPONENT
	memb = firstMemb(Rchan);
	if (memb->loc.params.nakmaxtime == 0 || memb->loc.params.nakholdoff == 0) {
		sendNAK(Rchan, false);
		LOG_FEND(lgFCTY);
		return;
	}
	holdoff = (uint32_t)Rchan->Rseq + memb->loc.mid;
	holdoff = (holdoff % memb->loc.params.nakmodulus) * memb->loc.params.nakholdoff;
	if (holdoff > memb->loc.params.nakmaxtime) holdoff = memb->loc.params.nakmaxtime;
#else
	/* calculate minimum holdoff of all local members */
	minhoff = UINT32_MAX;
	forEachMemb(memb, Rchan) {
		if (memb->loc.params.nakmaxtime == 0 || memb->loc.params.nakholdoff == 0) {
			/* holdoff is zero - send immediately */
			sendNAK(Rchan, false);
			LOG_FEND(lgFCTY);
			return;
		} else {
			holdoff = (uint32_t)Rchan->Rseq + memb->loc.mid;
			holdoff = (holdoff % memb->loc.params.nakmodulus) * memb->loc.params.nakholdoff;
			if (holdoff > memb->loc.params.nakmaxtime) holdoff = memb->loc.params.nakmaxtime;
			if (holdoff < minhoff) minhoff = holdoff;
		}
	}
	holdoff = minhoff;
#endif

	Rchan->NAKstate = NS_HOLDOFF;
	Rchan->NAKtimer.action = NAKholdoffAction;
	set_timer(&Rchan->NAKtimer, timerval_ms(holdoff));
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Update sequence numbers for members of an Lchan and dispose of any
released back-wrappers. Called on receipt of ACK or NAK or any other
confirmation of a members ACK point.
*/
/**********************************************************************/
static void
updateRmembSeq(member_t *memb, int32_t Rseq)
{
	Lchannel_t *Lchan;
	txwrap_t *txwrap;
	txwrap_t *tp;

	LOG_FSTART(lgFCTY);
	Lchan = memb->rem.Lchan;

	/* handle some special cases */
	if (memb->rem.Rseq == Rseq) return; /* already up to this point */
	
	/* decrease ack count for backwraps and delete any which are fully acked */
	for (txwrap = Lchan->obackwrap; txwrap && (Rseq - txwrap->st.sent.Rseq) >= 0; ) {
		/* for all backwraps up to Rseq */
		tp = txwrap;
		txwrap = txwrap->st.sent.lnk.r;
		if ((memb->rem.Rseq - tp->st.sent.Rseq) < 0 && --tp->st.sent.acks == 0) {
			/* member hasn't already passed this wrapper and we're the last to ack */
			while ((tp = Lchan->obackwrap) != txwrap) {
				Lchan->obackwrap = tp->st.sent.lnk.r;
				if (tp->st.sent.acks)
					acnlogmark(lgINFO, "Tx ACK anomaly - missing %d acks", tp->st.sent.acks);
				if (--tp->usecount == 0) {
					acnlogmark(lgDBUG, "Tx Free fully acked %" PRIu32, tp->st.sent.Rseq);
					free_txbuf(tp->txbuf, tp->size);
					free_txwrap(tp);
				}
				if (--Lchan->backwraps == 0) {
					assert(Lchan->obackwrap == NULL && txwrap == NULL);
					Lchan->nbackwrap = NULL;
				}
			}
		}
	}
	memb->rem.Rseq = Rseq;
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Set the MAK specification in an outgoing wrapper
*/
const uint8_t noMAK[6] = {
	0xff, 0xff,
	0xff, 0xff,
	0xff, 0xff
};

static uint8_t *
setMAKs(uint8_t *bp, Lchannel_t *Lchan, uint16_t flags)
{
	member_t *memb;

	LOG_FSTART(lgFCTY);
	if (Lchan->membercount == 0)
		return marshalBytes(bp, noMAK, sizeof(noMAK));

	if (Lchan->primakHi) {
		uint16_t mid;
		uint16_t makthr;
		uint16_t retries = 0;

		makthr = 65535;
		for (mid = Lchan->primakLo; mid <= Lchan->primakHi; ++mid) {
			if ((memb = findRmembMID(Lchan, mid))
				&& memb->rem.mstate == MS_MEMBER
				&& !is_active(&memb->rem.stateTimer))
			{
				if ((Lchan->Rseq - memb->rem.Rseq) < makthr)
					makthr = Lchan->Rseq - memb->rem.Rseq;
				--memb->rem.maktries;
				if ((MAK_MAX_RETRIES + 1 - memb->rem.maktries) > retries)
					retries = (MAK_MAX_RETRIES + 1 - memb->rem.maktries);
				set_timer(&memb->rem.stateTimer, Lchan_MAK_TIMEOUT(Lchan));
			}
			
		}
		Lchan->ka_t_ms >>= 1;
		bp = marshalU16(bp, Lchan->primakLo);
		bp = marshalU16(bp, Lchan->primakHi);
		bp = marshalU16(bp, 0);
		acnlogmark(lgDBUG, "Tx MAK (pri) %hu-%hu, %ums", Lchan->primakLo, Lchan->primakHi, Lchan->ka_t_ms);
		Lchan->primakLo = Lchan->primakHi = 0;
	} else {
		uint16_t firstmak;
		uint16_t lastmak;
		uint16_t makthr;
		int Nmaks;

		makthr = Lchan->makthr;
		firstmak = 0;
		lastmak = Lchan->lastmak;
		Nmaks = Lchan->makspan;

		while (1) {
			if (++lastmak > Lchan->himid) {
				if (firstmak) break;
				lastmak = 1;
			}
			if ((memb = findRmembMID(Lchan, lastmak)) == NULL) continue;
			if (firstmak == 0) firstmak = lastmak;
			if (--Nmaks == 0) break;
		}
		Lchan->ka_t_ms <<= 1;
		if (Lchan->ka_t_ms > (unsigned int)Lchan_KEEPALIVE_ms(Lchan))
			Lchan->ka_t_ms = (unsigned int)Lchan_KEEPALIVE_ms(Lchan);
		bp = marshalU16(bp, firstmak);
		bp = marshalU16(bp, lastmak);
		bp = marshalU16(bp, Lchan->makthr);
		acnlogmark(lgDBUG, "Tx MAK %hu-%hu, %ums", firstmak, lastmak, Lchan->ka_t_ms);
	}
	LOG_FEND(lgFCTY);
	return bp;
}

/**********************************************************************/
/*
Default values for unicast and multicast channel parameters
*/
static const chanParams_t dflt_multicastParams = {
	.expiry_sec = 20,
	.flags      = NAK_OUTBOUND,
	.nakholdoff = NAK_HOLDOFF_INTERVAL_ms,
	.nakmodulus = 16,
	.nakmaxtime = NAK_MAX_TIME_ms
};

static const chanParams_t dflt_unicastParams = {
	.expiry_sec = 20,
	.flags      = 0,
	.nakholdoff = 0,
	.nakmodulus = 1,
	.nakmaxtime = 0
};

/**********************************************************************/
/*
	Create and initialize a new channel. If params is NULL then defaults
	are used. If params is provided then these are used for the new channel
	unless the RECIPROCAL flag is set, in which case the  parameters
	supplied are assumed to be from an incoming channel and the new
	channel's parameters are matched to these.

	Returns Lchannel_t * on success, on failure NULL is returned and errno 
	set accordingly.

	Errors:
		EINVAL supplied parameters are invalid
		ENOMEM couldn't allocate a new Lchannel_t
*/
#define new_mcast(comp) ((comp)->sdt.scopenhost \
		| htonl((uint32_t)((comp)->sdt.dyn_mask \
					& (comp)->sdt.dyn_mcast++)))

Lchannel_t *
openChannel(if_MANYCOMP(Lcomponent_t *Lcomp,) chanParams_t *params, uint16_t flags)
{
	if_ONECOMP(struct Lcomponent_s * const Lcomp = &localComponent;)
	Lchannel_t *Lchan;

	LOG_FSTART(lgFCTY);
	/* first check valid arguments */
	if (params
			&& (params->expiry_sec < MIN_EXPIRY_TIME_s
				|| (params->flags & ~PARAM_FLAG_MASK)
				|| params->nakmodulus == 0))
	{
		errno = EINVAL;
		return NULL;
	}

	if ((Lchan = new_Lchannel()) == NULL) {
		acnlogerror(lgERR);
		return NULL;
	}
	/* open ephemeral socket for upstream */
	if ((Lchan->inwd_sk = rlpSubscribe(NULL, SDT_PROTOCOL_ID, &sdtRxLchan, Lchan)) == NULL) {
		acnlogerror(lgERR);
		free_Lchannel(Lchan);
		return NULL;
	}

#if !CONFIG_SINGLE_COMPONENT
	Lchan->owner = Lcomp;
#endif
	Lchan->chanNo = getNewChanNo();
	Lchan->Tseq = Lchan->Rseq = 0;
	Lchan->blankTimer.action = blanktimeAction;
	Lchan->blankTimer.userp = Lchan;
	Lchan->keepalive.action = keepaliveAction;
	Lchan->keepalive.userp = Lchan;
	Lchan->flags = flags;
	Lchan->params = (flags & CHF_UNICAST) ? dflt_unicastParams : dflt_multicastParams;
	
	if (params) {
		if ((flags & CHF_RECIPROCAL))
			Lchan->params.expiry_sec = params->expiry_sec;
		else {
			Lchan->params = *params;
			if ((flags & CHF_UNICAST))  /* override for unicaast channels */
				Lchan->params.flags = 0;
		}
	}
	Lchan->makthr = DFLT_MAKTHR;
	Lchan->makspan = DFLT_MAKSPAN;
	Lchan->ka_t_ms = Lchan_KEEPALIVE_ms(Lchan);

	if ((flags & CHF_UNICAST)) {
		netx_TYPE(&Lchan->outwd_ad) = SDT_ADDR_NULL; /* gets filled in on JoinAccept */
	} else {
#if CONFIG_JOIN_TX_GROUPS
		/* multicast - use inwd_sk to subscribe to outwd_ad */
		struct rlpsocket_s *sk;
		struct in_addr mcast;

		mcast.s_addr = new_mcast(LchanOwner(Lchan));
		acnlogmark(lgDBUG, "using multicast %s", inet_ntoa(mcast));
		netx_INIT_ADDR(&Lchan->outwd_ad, mcast.s_addr, Lchan->inwd_sk->port);
		sk = rlpSubscribe(&Lchan->outwd_ad, SDT_PROTOCOL_ID, &sdtRxLchan, Lchan);
		if (Lchan->inwd_sk != sk) {
			acnlogmark(lgWARN, "wrong socket returned");
			rlpUnsubscribe(sk, &Lchan->outwd_ad, SDT_PROTOCOL_ID);
		}
		netx_PORT(&Lchan->outwd_ad) = htons(SDT_MULTICAST_PORT); /* correct the outward port */
#else
		netx_INIT_ADDR(&Lchan->outwd_ad, new_mcast(LchanOwner(Lchan)), htons(SDT_MULTICAST_PORT));
#endif
	}
	linkLchan(Lcomp, Lchan);
	LOG_FEND(lgFCTY);
	return Lchan;
}   
/**********************************************************************/
/*
*/

void
closeChannel(Lchannel_t *Lchan)
{
	assert (Lchan);

	LOG_FSTART(lgFCTY);

	if (Lchan->membercount > 0) {
		Lchan->flags |= CHF_NOCLOSE;  /* prevent recursive callback when last member is killed */
		do {
			killMember(getRmemb(Lchan, Lchan->himid - 1), SDT_REASON_NONSPEC, EV_LOCCLOSE);
		} while (Lchan->membercount > 0);
	}

#if CONFIG_JOIN_TX_GROUPS
	if (!unicastLchan(Lchan)) {
		netx_PORT(&Lchan->outwd_ad) = Lchan->inwd_sk->port;
		rlpUnsubscribe(Lchan->inwd_sk, &Lchan->outwd_ad, SDT_PROTOCOL_ID);
	}
#endif
	rlpUnsubscribe(Lchan->inwd_sk, NULL, SDT_PROTOCOL_ID);

	unlinkLchan(LchanOwner(Lchan), Lchan);
	free_Lchannel(Lchan);
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Create a new member and add it to a channel and send a cold Join.
*/
int
addMember(Lchannel_t *Lchan, uuid_t cid, netx_addr_t *adhoc)
{
	Rcomponent_t *Rcomp;
	member_t *memb;
	
	LOG_FSTART(lgFCTY);
	assert(Lchan);

	if (findornewRcomp(cid, &Rcomp) < 0) {
		acnlogerror(lgERR);
		return -1;
	}
	Rcomp->useflags |= USEDBY_SDT;

	acnlogmark(lgDBUG, "got new Rcomp");
	if (adhoc)
		Rcomp->sdt.adhocAddr = *adhoc;

	if (Lchan->membercount > 0 && unicastLchan(Lchan)) {
		acnlogmark(lgERR, "add multiple members to unicast channel");
		errno = EADDRINUSE;
		return -1;
	}

	acnlogmark(lgDBUG, "search for existing member");
	/* is the remote already a member of this channel? */
	if (findRmembComp(Lchan, Rcomp) != NULL) {
		errno = EALREADY;
		acnlogerror(lgERR);
		return -1;
	}

	acnlogmark(lgDBUG, "creating new member");
	if ((memb = new_member()) == NULL) {
		errno = ENOMEM;
		acnlogerror(lgERR);
		return -1;
	}

	/* initialize our new member (rem half only) */
	acnlogmark(lgDBUG, "initializing new member");
	memb->rem.Lchan = Lchan;
	memb->rem.Rcomp = Rcomp;
	memb->rem.t_ms = AD_HOC_TIMEOUT_ms;
	memb->rem.Rseq = Lchan->Rseq;
	memb->rem.stateTimer.userp = memb;

	acnlogmark(lgDBUG, "linking new member to channel");
	linkRmemb(Lchan, memb); /* this assigns a MID */

	/* All OK so far - send a join */
	acnlogmark(lgDBUG, "sending join");
	if (sendJoin(Lchan, memb) < 0) {
		acnlogmark(lgDBUG, "fail: unlink");
		unlinkRmemb(Lchan, memb);
		acnlogmark(lgDBUG, "free");
		free_member(memb);
		errno = ECONNABORTED;
		acnlogerror(lgERR);
		return -1;
	}
	acnlogmark(lgDBUG, "initialize state");
	memb->rem.mstate = MS_JOINRQ;
	memb->rem.stateTimer.action = joinTimeoutAction;
	acnlogmark(lgDBUG, "set timeout");
	set_timer(&memb->rem.stateTimer, timerval_ms(memb->rem.t_ms));
	LOG_FEND(lgFCTY);
	return 0;
}

/**********************************************************************/
/*
Create a reciprocal connection (in response to a cold Join).
The local member should be initialized but the remote blank.
Pick or create a local channel (policy decision), set up the
Rmember and send Join.
*/
static int
createRecip(Lcomponent_t *Lcomp, Rcomponent_t *Rcomp, member_t *memb)
{
	Lchannel_t *Lchan;

	LOG_FSTART(lgFCTY);
	memb->rem.mstate = MS_NULL;
	Lchan = (*Lcomp->sdt.joinRx)(if_MANYCOMP(Lcomp,) &memb->loc.params);

	if (Lchan == NULL) {
		acnlogerror(lgINFO);
		return -1;
	}
	memb->rem.Lchan = Lchan;
	memb->rem.Rcomp = Rcomp;
	memb->rem.t_ms = AD_HOC_TIMEOUT_ms;
	memb->rem.Rseq = Lchan->Rseq;
	memb->rem.stateTimer.userp = memb;

	linkRmemb(Lchan, memb);
/*
	All OK so far - send a join
*/
	if (sendJoin(Lchan, memb) < 0) {
		unlinkRmemb(Lchan, memb);
		errno = ECONNABORTED;
		return -1;
	}
	memb->rem.mstate = MS_JOINRQ;
	memb->rem.stateTimer.action = joinTimeoutAction;
	set_timer(&memb->rem.stateTimer, timerval_ms(memb->rem.t_ms));
	LOG_FEND(lgFCTY);
	return 0;
}

/**********************************************************************/
Lchannel_t *
autoJoin(if_MANYCOMP(struct Lcomponent_s *Lcomp,) chanParams_t *params)
{
	Lchannel_t *Lchan;

	LOG_FSTART(lgFCTY);
	Lchan = openChannel(if_MANYCOMP(Lcomp,) params, CHF_UNICAST | CHF_RECIPROCAL);
	LOG_FEND(lgFCTY);
	return Lchan;
}

/**********************************************************************/
void
drop_member(member_t *memb, uint8_t reason)
{
	LOG_FSTART(lgFCTY);
	killMember(memb, reason, EV_LOCCLOSE);
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Timeout functions
*/
/**********************************************************************/
#define AD_HOC_MAX_TIMEOUT_ms (AD_HOC_TIMEOUT_ms << AD_HOC_RETRIES)
/*
Adhoc join has failed
*/

static void
joinTimeoutAction(acnTimer_t *timer)
{
	member_t *memb;
	Lchannel_t *Lchan;

	LOG_FSTART(lgFCTY);
	memb = container_of(timer, member_t, rem.stateTimer);

	Lchan = memb->rem.Lchan;

	if (memb->rem.t_ms > AD_HOC_MAX_TIMEOUT_ms || sendJoin(Lchan, memb) < 0) {
		killMember(memb, SDT_REASON_NONSPEC, EV_JOINFAIL);
		return;
	}
	memb->rem.t_ms <<= 1;
	set_timer(timer, timerval_ms(memb->rem.t_ms));
}

/**********************************************************************/
/*
Reciprocal Join has expired
*/
static void
recipTimeoutAction(acnTimer_t *timer)
{
	member_t *memb;

	memb = container_of(timer, member_t, rem.stateTimer);

	killMember(memb, SDT_REASON_NO_RECIPROCAL, EV_JOINFAIL);
}

/**********************************************************************/
static void
makTimeoutAction(acnTimer_t *timer)
{
	member_t *memb;
	Lchannel_t *Lchan;

	LOG_FSTART(lgFCTY);
	memb = container_of(timer, member_t, rem.stateTimer);
	Lchan = memb->rem.Lchan;

	acnlogmark(lgDBUG, "Tx maktries == %u", memb->rem.maktries);
	if (memb->rem.maktries == 0)
		killMember(memb, SDT_REASON_CHANNEL_EXPIRED, EV_MAKTIMEOUT);
	else {
		if (memb->rem.mid > Lchan->primakHi) {
			Lchan->primakHi = memb->rem.mid;
			if (Lchan->primakLo == 0) Lchan->primakLo = Lchan->primakHi;
		} else if (memb->rem.mid < Lchan->primakLo) {
			Lchan->primakLo = memb->rem.mid;
		}
		emptyWrapper(Lchan, WRAP_REL_OFF /* | WRAP_NOAUTOACK */);
	}
	LOG_FEND(lgFCTY);
}

/**********************************************************************/

static void
prememberAction(acnTimer_t *timer)
{
	txwrap_t *txwrap;
	member_t *memb;
	
	txwrap = container_of(timer, txwrap_t, st.fack.rptTimer);
	memb = (member_t *)timer->userp;

	if (memb->loc.mstate == MS_MEMBER) {
		acnlogmark(lgDBUG, "Tx Rpt 1st ACK");
		rlp_sendbuf(txwrap->txbuf, (txwrap->endp - txwrap->txbuf),
								memb->rem.Lchan->inwd_sk, &memb->rem.Lchan->outwd_ad,
								LchanOwner(memb->rem.Lchan)->hd.uuid);

		if ((txwrap->st.fack.t_ms <<= 1) <= MAXACK_REPEAT_ms) {
			set_timer(timer, timerval_ms(txwrap->st.fack.t_ms));
			return;
		}
	}
	slUnlink(txwrap_t, firstacks, txwrap, st.fack.lnk);
	cancelWrapper(txwrap);
	return;
}

/**********************************************************************/
/*
Remote channel has expired
*/
static void
expireAction(acnTimer_t *timer)
{
	Rchannel_t *Rchan;

	LOG_FSTART(lgFCTY);
	Rchan = (Rchannel_t *)(timer->userp);
/*
	Take down Rchan and reciprocal
*/
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Naking has failed
*/
static void
NAKfailAction(acnTimer_t *timer)
{
	Rchannel_t *Rchan;

	Rchan = container_of(timer, Rchannel_t, NAKtimer);

	if (Rchan->NAKtries > 0) {
		--Rchan->NAKtries;
		NAKwrappers(Rchan);
	} else {
		killRchan(Rchan, SDT_REASON_LOST_SEQUENCE, EV_NAKTIMEOUT);
	}
}

/**********************************************************************/
/*
NAK holdoff expired
*/
static void
NAKholdoffAction(acnTimer_t *timer)
{
	Rchannel_t *Rchan;

	Rchan = container_of(timer, Rchannel_t, NAKtimer);
	sendNAK(Rchan, false);
}

/**********************************************************************/
/*
Lchannel NAK blank timeout
*/
static void
blanktimeAction(acnTimer_t *timer)
{
	Lchannel_t *Lchan;

	Lchan = container_of(timer, Lchannel_t, blankTimer);

	Lchan->nakfirst = Lchan->naklast = 0;
}

/**********************************************************************/
/*

*/
static void
keepaliveAction(acnTimer_t *timer)
{
	Lchannel_t *Lchan;

	Lchan = container_of(timer, Lchannel_t, keepalive);

	LOG_FSTART(lgFCTY);
	emptyWrapper(Lchan, WRAP_REL_OFF /* | WRAP_NOAUTOACK */);
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
PDU Block processing

Have to do this at three levels
	level 1  the base level is called by RLP
	wrapper  process the contents of a wrapper - a client block
	level 2  an SDT PDU block embedded in the wrapper client block
*/
/**********************************************************************/
/*
Block receive - callback from RLP to handle level 1 messages
*/
static void
sdtRxAdhoc(const uint8_t *pdus, int blocksize, rxcontext_t *rcxt)
{
	uint8_t INITIALIZED(vector);
	const uint8_t *pdup;
	const uint8_t *datap = NULL;
	const uint8_t *pp;
	int datasize = 0;
	uint8_t flags;

	LOG_FSTART(lgFCTY);

	if(blocksize < 3) {
		acnlogmark(lgWARN, "Rx short PDU block");
		return;
	}

	/* first PDU must have all fields */
	if ((*pdus & (FLAG_bMASK)) != (FIRST_bFLAGS)) {
		acnlogmark(lgDBUG, "Rx bad first PDU flags");
		return;
	}
#if !CONFIG_SINGLE_COMPONENT
	rcxt->sdt1.Lcomp = (Lcomponent_t *)(rcxt->rlp.handlerRef);
#endif

	for (pdup = pdus; pdup < pdus + blocksize - 2;)
	{
		flags = *pdup;
		pp = pdup + 2;
		pdup += getpdulen(pdup);

		if (flags & VECTOR_bFLAG) vector = *pp++;

		if (flags & DATA_bFLAG)  {
			datap = pp; /* get pointer to start of the data */
			if ((datasize = pdup - pp) < 0) { /* get size of the data */
				acnlogmark(lgDBUG, "Rx blocksize error");
				return;
			}
		}
		switch (vector) {
		case SDT_JOIN:
			if (!uuidsEq(datap, ctxtLcomp->hd.uuid)) {
				char cidstr1[UUID_STR_SIZE];
				char cidstr2[UUID_STR_SIZE];

				acnlogmark(lgWARN, "Rx adhoc to unknown component\n"
						" my cid: %s\n"
						"request: %s\n",
						uuid2str(ctxtLcomp->hd.uuid, cidstr1),
						uuid2str(datap, cidstr2));
				break;
			}
			rx_join(datap, datasize, rcxt);
			break;
		case SDT_GET_SESSIONS:
			if (!uuidsEq(datap, ctxtLcomp->hd.uuid)) {
				acnlogmark(lgWARN, "Rx adhoc to unknown component");
				break;
			}
			rx_getSessions(datap, datasize, rcxt);
			break;
		case SDT_SESSIONS:
			if ((rcxt->sdt1.Rcomp = findRcomp(rcxt->rlp.srcCID)) == NULL) {
				acnlogmark(lgNTCE, "Rx sessions from unknown owner");
				break;
			}
			rx_sessions(datap, datasize, rcxt);
			break;

		case SDT_JOIN_ACCEPT:
		case SDT_JOIN_REFUSE:
		case SDT_LEAVING:
		case SDT_NAK:
		case SDT_REL_WRAP:
		case SDT_UNREL_WRAP:
		case SDT_CHANNEL_PARAMS:
		case SDT_LEAVE:
		case SDT_CONNECT:
		case SDT_CONNECT_ACCEPT:
		case SDT_CONNECT_REFUSE:
		case SDT_DISCONNECT:
		case SDT_DISCONNECTING:
		case SDT_ACK:
		default:
			acnlogmark(lgWARN, "Rx unrecognised adhoc PDU type %u", vector);
			break;
		}
	}
	if (pdup != pdus + blocksize)  { /* sanity check */
		acnlogmark(lgDBUG, "Rx blocksize mismatch");
	}
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
static void
sdtRxLchan(const uint8_t *pdus, int blocksize, rxcontext_t *rcxt)
{
	uint8_t INITIALIZED(vector);
	const uint8_t *pdup;
	const uint8_t *datap = NULL;
	const uint8_t *pp;
	int datasize = 0;
	uint8_t flags;

	LOG_FSTART(lgFCTY);

	if(blocksize < 3) {
		acnlogmark(lgWARN, "Rx short PDU block");
		return;
	}

	/* first PDU must have all fields */
	if ((*pdus & (FLAG_bMASK)) != (FIRST_bFLAGS)) {
		acnlogmark(lgDBUG, "Rx bad first PDU flags");
		return;
	}
#if !CONFIG_SINGLE_COMPONENT
	rcxt->sdt1.Lcomp = ((Lchannel_t *)(rcxt->rlp.handlerRef))->owner;
#endif

	for (pdup = pdus; pdup < pdus + blocksize - 2;)
	{
		flags = *pdup;
		pp = pdup + 2;
		pdup += getpdulen(pdup);

		if (flags & VECTOR_bFLAG) vector = *pp++;

		if (flags & DATA_bFLAG)  {
			datap = pp; /* get pointer to start of the data */
			if ((datasize = pdup - pp) < 0) { /* get size of the data */
				acnlogmark(lgDBUG, "Rx blocksize error");
				return;
			}
		}
		if (!uuidsEq(datap, LchanOwner(((Lchannel_t *)(rcxt->rlp.handlerRef)))->hd.uuid)) {
			acnlogmark(lgWARN, "Rx inbound to wrong component");
			break;
		}
		switch (vector) {
		case SDT_JOIN_ACCEPT:
			rx_joinAccept(datap, datasize, rcxt);
			break;
		case SDT_JOIN_REFUSE:
			rx_joinRefuse(datap, datasize, rcxt);
			break;
		case SDT_LEAVING:
			rx_leaving(datap, datasize, rcxt);
			break;
		case SDT_NAK:
			rx_ownerNAK(datap, datasize, rcxt);
			break;
/*
		Unfortunately due to SDT's broken addressing we may get adhoc messages here
*/
		case SDT_JOIN:    /* this should be a reciprocal but might be cold */
		case SDT_GET_SESSIONS:  /* shouldn't get this here!! */
			switch (vector) {
			case SDT_JOIN:
				rx_join(datap, datasize, rcxt);
				break;
			case SDT_GET_SESSIONS:
				rx_getSessions(datap, datasize, rcxt);
				break;
			default:
				break;
			}
			break;

		case SDT_SESSIONS:   /* ignore sessions if not to our true adhoc */
		case SDT_REL_WRAP:
		case SDT_UNREL_WRAP:
		case SDT_CHANNEL_PARAMS:
		case SDT_LEAVE:
		case SDT_CONNECT:
		case SDT_CONNECT_ACCEPT:
		case SDT_CONNECT_REFUSE:
		case SDT_DISCONNECT:
		case SDT_DISCONNECTING:
		case SDT_ACK:
		default:
			acnlogmark(lgWARN, "Rx unrecognised base layer PDU type %u", vector);
			break;
		}
	}
	if (pdup != pdus + blocksize)  { /* sanity check */
		acnlogmark(lgDBUG, "Rx blocksize mismatch");
	}
	LOG_FEND(lgFCTY);
}
/**********************************************************************/

#if acntestlog(lgDBUG) && 0
void showcomponents(void)
{
	int i, j;
	Rcomponent_t *Rcomp;
	char cidstr[UUID_STR_SIZE];

	fprintf(STDLOG, "Remote components\n");
	for (i = j = 0; i < hashcount(CONFIG_R_HASHBITS); ++i) {
		if ((Rcomp = remoteComps[i])) {
			fprintf(STDLOG, "at 0x%02x:\n", i);
			do {
				fprintf(STDLOG, "   %s\n", uuid2str(Rcomp->hd.uuid, cidstr));
				Rcomp = Rcomp->sdt.rlnk;
				++j;
			} while (Rcomp);
		}
	}
	fprintf(STDLOG, "total %d in %d lists\n", j, i);
}
#endif

/**********************************************************************/
static void
sdtRxRchan(const uint8_t *pdus, int blocksize, rxcontext_t *rcxt)
{
	uint8_t INITIALIZED(vector);
	const uint8_t *pdup;
	const uint8_t *datap = NULL;
	const uint8_t *pp;
	int datasize = 0;
	uint8_t flags;

	LOG_FSTART(lgFCTY);

	if(blocksize < 3) {
		acnlogmark(lgWARN, "Rx short PDU block");
		return;
	}

	/* first PDU must have all fields */
	if ((*pdus & (FLAG_bMASK)) != (FIRST_bFLAGS)) {
		acnlogmark(lgDBUG, "Rx bad first PDU flags");
		return;
	}
	for (pdup = pdus; pdup < pdus + blocksize - 2;)
	{
		flags = *pdup;
		pp = pdup + 2;
		pdup += getpdulen(pdup);

		if (flags & VECTOR_bFLAG) vector = *pp++;

		if (flags & DATA_bFLAG)  {
			datap = pp; /* get pointer to start of the data */
			if ((datasize = pdup - pp) < 0) { /* get size of the data */
				acnlogmark(lgDBUG, "Rx blocksize error");
				return;
			}
		}
		switch (vector) {
		case SDT_REL_WRAP:
		case SDT_UNREL_WRAP:
			if ((rcxt->sdt1.Rcomp = findRcomp(rcxt->rlp.srcCID)) == NULL) {
				acnlogmark(lgNTCE, "Rx wrapper from unknown source");
				break;
			}
			rx_wrapper(datap, datasize, rcxt, vector == SDT_REL_WRAP);
			break;
		case SDT_NAK:
			/*
			NAK (member) (multicast) datap matches RownerCid
			*/
			if ((rcxt->sdt1.Rcomp = findRcomp(datap)) != NULL) {
				rx_outboundNAK(datap, datasize, rcxt);
			}
#ifdef MATCH_OUTBOUND_NAK_TO_INBOUND
			{
				Lcomponent_t *Lcomp;
				Lchannel_t *Lchan;

				if ((Lcomp = findLcomp(datap)) == NULL)
					break;
				if ((Lchan = findLchan(Lcomp, unmarshalU16(datap + OFS_NAK_CHANNO))) {
#if !CONFIG_SDT_SINGLE_CLIENT
					rcxt->sdt1.Lcomp = Lcomp;
#endif
					rcxt->rlp.handlerRef = Lchan;
					rx_ownerNAK(datap, datasize, rcxt);
				}
			}
#endif
			/* don't bother to log downstream NAKs for unknown channels */
			break;

		case SDT_JOIN:
		case SDT_GET_SESSIONS:
		case SDT_SESSIONS:
		case SDT_JOIN_ACCEPT:
		case SDT_JOIN_REFUSE:
		case SDT_LEAVING:
		case SDT_CHANNEL_PARAMS:
		case SDT_LEAVE:
		case SDT_CONNECT:
		case SDT_CONNECT_ACCEPT:
		case SDT_CONNECT_REFUSE:
		case SDT_DISCONNECT:
		case SDT_DISCONNECTING:
		case SDT_ACK:
		default:
			acnlogmark(lgWARN, "Rx unrecognised base layer PDU type %u", vector);
			break;
		}
	}
	if (pdup != pdus + blocksize)  { /* sanity check */
		acnlogmark(lgDBUG, "Rx blocksize mismatch");
	}
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
static void
pendingRxWrap(Rchannel_t *Rchan, const uint8_t *data, int length)
{
	uint16_t vector;
	const uint8_t *pdup;
	const uint8_t *datap;
	const uint8_t *pp;
	int datasize = 0;
	uint8_t flags;
	member_t *memb;
	uint32_t protocol;
	uint16_t assoc;

	LOG_FSTART(lgFCTY);

	pdup = data;
	/* first PDU must have all fields */
	if ((*pdup & (FLAG_bMASK)) != (FIRST_bFLAGS)) {
		acnlogmark(lgERR, "Rx bad first PDU flags");
		return;
	}

	while (pdup < data + length) {
		flags = *pdup;
		pp = pdup + 2;
		pdup += getpdulen(pdup);

		if (flags & VECTOR_bFLAG) {
			vector = unmarshalU16(pp); pp += 2;
		}

		if (flags & HEADER_bFLAG) {
			protocol = unmarshalU32(pp); pp += 4;
			assoc = unmarshalU16(pp); pp += 2;
		}

		if (flags & DATA_bFLAG)  {
			datap = pp; /* get pointer to start of the data */
			if ((datasize = pdup - pp) < 0) { /* get size of the data */
				acnlogmark(lgERR, "Rx blocksize error");
				return;
			}
		}
		if (protocol == SDT_PROTOCOL_ID) {
			forEachMemb(memb, Rchan) {
				if (memb->loc.mstate >= MS_JOINPEND
					&& (vector == ALL_MEMBERS || vector == memb->loc.mid))
				{
					sdtLevel2Rx(datap, datasize, memb);
				}
			}
		}
	}
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Reliable or unreliable wrapper - level 1 channel message
*/

static void
rx_wrapper(const uint8_t *data, int length, rxcontext_t *rcxt, bool reliable)
{
	Rchannel_t *Rchan;
	int32_t seq;
	struct rxwrap_s *curp;
	enum mstate_e chanstate;

	member_t *memb;

	LOG_FSTART(lgFCTY);
	if (length < LEN_WRAPPER_MIN) {
		/* PDU is wrong length */
		acnlogmark(lgERR, "Rx length error");
		return;
	}
	
	Rchan = findRchan(rcxt->sdt1.Rcomp, unmarshalU16(data + OFS_WRAPPER_CHANNO));
	if (!Rchan) {
		acnlogmark(lgINFO, "Rx unknown channel %u", unmarshalU16(data + OFS_WRAPPER_CHANNO));
		return;
	}

	acnlogmark(lgDBUG, "Rx %cwrapper T=%" PRIu32 " R=%" PRIu32 " Oldest=%" PRIu32,
				(reliable ? 'R' : 'U'),
				unmarshalSeq(data + OFS_WRAPPER_TSEQ),
				unmarshalSeq(data + OFS_WRAPPER_RSEQ),
				unmarshalSeq(data + OFS_WRAPPER_OLDEST));

	if (NAKcheck(Rchan, data) < 0) return; /* check for lost sequence */

#if CONFIG_SINGLE_COMPONENT
	chanstate = firstMemb(Rchan)->loc.mstate;
#else
	chanstate = 0;
	forEachMemb(memb, Rchan) {
		if (memb->loc.mstate >= MS_JOINPEND
			&& (chanstate = memb->loc.mstate) == MS_MEMBER) break;
	}
#endif
	if (chanstate < MS_JOINPEND) {
		acnlogmark(lgDBUG, "Rx not fully joined");
		return;
	}

	seq = unmarshalSeq(data + OFS_WRAPPER_TSEQ);
	if ((seq - Rchan->Tseq) <= 0) {  /* check for repeat wrapper */
		acnlogmark(lgDBUG, "Rx repeat wrapper");
		return;
	}

	if (chanstate == MS_JOINPEND) {
		acnlogmark(lgDBUG, "Rx wrap in JoinPend");
		Rchan->Tseq = seq;
		Rchan->Rseq = unmarshalSeq(data + OFS_WRAPPER_RSEQ);

		if (length == LEN_WRAPPER_MIN) return;
		if (length < (LEN_WRAPPER_MIN + OFS_CB_PDU1DATA + SDT_OFS_PDU1DATA)) {
			acnlogmark(lgERR, "Rx length error");
			return;
		}

		pendingRxWrap(Rchan, data + OFS_WRAPPER_CB, length - OFS_WRAPPER_CB);
		return;
	}

/*
	it's a new wrapper and we're a member - we'll need to queue it either
	for immediate processing or fow handling after NAK processing, so
	create the queue entry now.
*/
	curp = acnNew(struct rxwrap_s);
	curp->Rchan = Rchan;
	curp->Tseq = seq;
	curp->Rseq = seq = unmarshalSeq(data + OFS_WRAPPER_RSEQ);
	curp->rxbuf = rcxt->netx.rcvbuf;
	curp->rxbuf->usecount += 1;
	curp->data = data;
	curp->length = length;
	curp->reliable = reliable;
	seq -= curp->reliable;

	if ((seq - Rchan->Rseq) == 0) {
		/* sequence OK (only unreliable skipped if any) */
		bool needack = false;

		forEachMemb(memb, Rchan) {
			set_timer(&memb->loc.expireTimer, timerval_s(memb->loc.params.expiry_sec));
		}
		Rchan->NAKtries = NAK_MAX_RETRIES;
		while (1) {
			needack = needack || mustack(Rchan, curp->Rseq, curp->data);
			queuerxwrap(Rchan, curp);
			if (Rchan->aheadQ == NULL) {
				/* we've caught up - cancel any NAK processing and refresh expiry */
				cancel_timer(&Rchan->NAKtimer);
				Rchan->NAKstate = NS_NULL;
				break;
			}
			curp = Rchan->aheadQ->lnk.l;
			if ((curp->Rseq - curp->reliable - Rchan->Rseq) > 0) break;

			assert((curp->Rseq - curp->reliable - Rchan->Rseq) == 0);
			dlUnlink(Rchan->aheadQ, curp, lnk);
		}
		if (needack) {
			forEachMemb(memb, Rchan) justACK(memb, false);
		}
		if (CONFIG_RX_AUTOCALL && rxqueue != NULL) readrxqueue();
	} else {
		/* queue this one for later - in order and NAK if new missing wrappers */
		rxwrap_t *rxp;

		if ((rxp = Rchan->aheadQ) == NULL) {
			Rchan->aheadQ = curp->lnk.r = curp->lnk.l = curp;
			Rchan->lastnak = seq;
		} else for (;; rxp = rxp->lnk.r) {
			if ((curp->Tseq - rxp->Tseq) == 0) {
				/* already seen this one */
				releaseRxbuf(rxp->rxbuf);
				free(curp);
				return;
			}
			if ((curp->Tseq - rxp->Tseq) > 0) {
				if ((seq - rxp->Rseq) != 0) {
					Rchan->lastnak = seq;
				}
				if (rxp == Rchan->aheadQ) Rchan->aheadQ = curp;
				dlInsertL(rxp, curp, lnk);
				break;
			}
			if (rxp->lnk.r == Rchan->aheadQ) {
				if (reliable && Rchan->lastnak == (seq + 1)) {
					Rchan->lastnak = seq;
				}
				dlInsertR(rxp, curp, lnk);
				break;
			}
		}

		if (Rchan->NAKstate == NS_NULL) {
			NAKwrappers(Rchan);
		}
	}
	LOG_FEND(lgFCTY);
}

/**********************************************************************/
/*
Level2 receive - callback from queuerxwrap or pendingRxWrap to handle
level 2 (wrapped) messages
*/

static void
sdtLevel2Rx(const uint8_t *pdus, int blocksize, member_t *memb)
{
	uint8_t INITIALIZED(vector);
	const uint8_t *pdup;
	const uint8_t *datap = NULL;
	const uint8_t *pp;
	int datasize = 0;
	uint8_t flags;

	LOG_FSTART(lgFCTY);

	if(blocksize < 3) {
		acnlogmark(lgWARN, "Rx short PDU block");
		return;
	}

	/* first PDU must have all fields */
	if ((*pdus & (FLAG_bMASK)) != (FIRST_bFLAGS)) {
		acnlogmark(lgDBUG, "Rx bad first PDU flags");
		exit(0);
		return;
	}

	for (pdup = pdus; pdup < pdus + blocksize;)
	{
		flags = *pdup;
		pp = pdup + 2;
		pdup += getpdulen(pdup);

		if (flags & VECTOR_bFLAG) vector = *pp++;

		if (flags & DATA_bFLAG)  {
			datap = pp; /* get pointer to start of the data */
			if ((datasize = pdup - pp) < 0) { /* get size of the data */
				acnlogmark(lgWARN, "Rx blocksize error");
				return;
			}
		}
		if (memb->loc.mstate == MS_MEMBER) {
			switch (vector) {
			case SDT_LEAVE:
				rx_leave(datap, datasize, memb);
				break;
			case SDT_ACK:
				rx_ack(datap, datasize, memb);
				break;
			case SDT_CHANNEL_PARAMS:
				rx_chparams(datap, datasize, memb);
				break;
			case SDT_CONNECT:
				rx_connect(datap, datasize, memb);
				break;
			case SDT_CONNECT_ACCEPT:
				rx_conaccept(datap, datasize, memb);
				break;
			case SDT_CONNECT_REFUSE:
				rx_conrefuse(datap, datasize, memb);
				break;
			case SDT_DISCONNECT:
				rx_disconnect(datap, datasize, memb);
				break;
			case SDT_DISCONNECTING:
				rx_disconnecting(datap, datasize, memb);
				break;
			default:
				acnlogmark(lgWARN, "Rx unrecognised client layer PDU type %u", vector);
				break;
			}
		} else {
			switch (vector) {
			case SDT_LEAVE:
				rx_leave(datap, datasize, memb);
				break;
			case SDT_ACK:
				rx_ack(datap, datasize, memb);
				break;
			default:
				acnlogmark(lgDBUG, "Rx client layer PDU type %u ignored in Join Pending state", vector);
				break;
			}
		}
	}
	if (pdup != pdus + blocksize)  { /* sanity check */
		acnlogmark(lgWARN, "Rx blocksize mismatch");
	}
	LOG_FEND(lgFCTY);
}




