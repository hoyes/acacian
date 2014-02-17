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
header: sdt.h

Implementation of SDT (Session Data Transport) protocol
*/

#ifndef __sdt_h__
#define __sdt_h__ 1

/**********************************************************************/
/*
Packet structure lengths
*/
#define SDT1_OFS_LENFLG RLP_OFS_PDU1DATA

/* Transport address lengths - add 1 for type byte */
#define LEN_TA_NULL    0
#define LEN_TA_IPV4    6
#define LEN_TA_IPV6    18

extern const unsigned short tasizes[];

#define MAX_TA_TYPE ARRAYSIZE(tasizes)
#define getTAsize(x) ((x) < MAX_TA_TYPE ? tasizes[x] : -1)

#if CF_NET_IPV6
#define HAVE_IPV6 1
#define HAVE_IPV4 1
#elif CF_NET_IPV4
#define HAVE_IPV6 0
#define HAVE_IPV4 1
#else
#define HAVE_IPV6 0
#define HAVE_IPV4 0
#endif

#define supportedTAsize(x) (\
	((x) == SDT_ADDR_NULL) ? LEN_TA_NULL :\
	((HAVE_IPV4) && (x) == SDT_ADDR_IPV4) ? LEN_TA_IPV4 :\
	((HAVE_IPV6) && (x) == SDT_ADDR_IPV6) ? LEN_TA_IPV6 :\
	-1)

#define tatypeSupported(tatype) (\
	(tatype) == SDT_ADDR_NULL\
	|| (HAVE_IPV4 && (tatype) == SDT_ADDR_IPV4)\
	|| (HAVE_IPV6 && (tatype) == SDT_ADDR_IPV6))

#define OFS_TA_PORT  1
#define OFS_TA_ADDR  3
/*
Max length of Transport address may be different for outgoing and
incoming packets as we only support a subset outgoing whilst we have to
allow for anything incoming.
*/
#define LEN_TA_OUT_MAX   (HAVE_IPV6 ? LEN_TA_IPV6 : HAVE_IPV4 ? LEN_TA_IPV4 : LEN_TA_NULL)
#define LEN_TA_IN_MAX   (LEN_TA_IPV6)

/* Session parameter block */
#define OFS_PARAM_EXPIRY   0
#define OFS_PARAM_FLAGS    1
#define OFS_PARAM_HOLDOFF  2
#define OFS_PARAM_MODULUS  4
#define OFS_PARAM_MAXTIME  6
#define LEN_PARAM          8

/* any SDT header (length/flags and vector) */
#define SDT_OFS_PDU1DATA     3
#define SDT1_PACKET_MIN   (RLP_OVERHEAD + SDT_OFS_PDU1DATA)

/************************************************************************/
/*
Message PDU lengths and offsets
all are data only - add SDT_OFS_PDU1DATA for vector and length/flags
*/
#define OFS_JOIN_CID     0
#define OFS_JOIN_MID    16
#define OFS_JOIN_CHANNO 18
#define OFS_JOIN_RECIP  20
#define OFS_JOIN_TSEQ   22
#define OFS_JOIN_RSEQ   26
#define OFS_JOIN_TADDR  30
#define OFS_JOIN_PARAMS(tasize)  (31 + (tasize))
#define OFS_JOIN_EXPIRY(tasize)  (31 + LEN_PARAM + (tasize))
#define LEN_JOIN(tasize)         (31 + LEN_PARAM + 1 + (tasize))
#define LEN_JOIN_MIN             LEN_JOIN(0)
#define LEN_JOIN_OUT_MAX             LEN_JOIN(LEN_TA_OUT_MAX)
#define PKT_JOIN_OUT   LEN_JOIN_OUT_MAX + RLP_OFS_PDU1DATA + SDT_OFS_PDU1DATA

/* offset from start of parameters to adhoc expiry */
#define OFS_JOINPAR_EXPIRY     LEN_PARAM

#define OFS_GETSESS_CID        0
#define LEN_GETSESS           16
#define PKT_GETSESS           LEN_GETSESS + RLP_OFS_PDU1DATA + SDT_OFS_PDU1DATA

#define OFS_SESSIONS_DATA      0

#define OFS_WRAPPER_CHANNO     0
#define OFS_WRAPPER_TSEQ       2
#define OFS_WRAPPER_RSEQ       6
#define OFS_WRAPPER_OLDEST    10
#define OFS_WRAPPER_FIRSTMAK  14
#define OFS_WRAPPER_LASTMAK   16
#define OFS_WRAPPER_MAKTHR    18
#define OFS_WRAPPER_CB        20
#define LEN_WRAPPER_MIN       OFS_WRAPPER_CB
#define PKT_WRAPPER_MIN       LEN_WRAPPER_MIN + RLP_OFS_PDU1DATA + SDT_OFS_PDU1DATA

#define OFS_JREFUSE_LEADCID    0
#define OFS_JREFUSE_CHANNO    16
#define OFS_JREFUSE_MID       18
#define OFS_JREFUSE_RSEQ      20
#define OFS_JREFUSE_REASON    24
#define LEN_JREFUSE           OFS_JREFUSE_REASON + 1
#define PKT_JREFUSE           LEN_JREFUSE + RLP_OFS_PDU1DATA + SDT_OFS_PDU1DATA

#define OFS_JACCEPT_LEADCID    0
#define OFS_JACCEPT_CHANNO    16
#define OFS_JACCEPT_MID       18
#define OFS_JACCEPT_RSEQ      20
#define OFS_JACCEPT_RECIP     24
#define LEN_JACCEPT           OFS_JACCEPT_RECIP + 2
#define PKT_JACCEPT           LEN_JACCEPT + RLP_OFS_PDU1DATA + SDT_OFS_PDU1DATA

#define OFS_LEAVING_LEADCID    0
#define OFS_LEAVING_CHANNO    16
#define OFS_LEAVING_MID       18
#define OFS_LEAVING_RSEQ      20
#define OFS_LEAVING_REASON    24
#define LEN_LEAVING           OFS_LEAVING_REASON + 1
#define PKT_LEAVING           LEN_LEAVING + RLP_OFS_PDU1DATA + SDT_OFS_PDU1DATA

#define OFS_NAK_LEADCID        0
#define OFS_NAK_CHANNO        16
#define OFS_NAK_MID           18
#define OFS_NAK_RSEQ          20
#define OFS_NAK_FIRSTMISS     24
#define OFS_NAK_LASTMISS      28
#define LEN_NAK               OFS_NAK_LASTMISS + 4
#define PKT_NAK               LEN_NAK + RLP_OFS_PDU1DATA + SDT_OFS_PDU1DATA

#define OFS_CHPARAMS_PARAMS          0
#define OFS_CHPARAMS_TADDR           LEN_PARAM
#define OFS_CHPARAMS_EXPIRY(tasize)  (LEN_PARAM + 1 + (tasize))
#define LEN_CHPARAMS(tasize)   OFS_CHPARAMS_EXPIRY(tasize) + 1

#define LEN_LEAVE              0

#define OFS_CONNECT_PROTO      0
#define LEN_CONNECT            4

#define OFS_DISCONNECT_PROTO   0
#define LEN_DISCONNECT         4

#define OFS_ACK_RSEQ           0
#define LEN_ACK                4
#define PKT_ACK                RLP_OFS_PDU1DATA \
									  + SDT_OFS_PDU1DATA \
									  + LEN_WRAPPER_MIN \
									  + OFS_CB_PDU1DATA \
									  + SDT_OFS_PDU1DATA \
									  + LEN_ACK

#define OFS_CONACCEPT_PROTO    0
#define LEN_CONACCEPT          4

#define OFS_CONREFUSE_PROTO    0
#define OFS_CONREFUSE_REASON   4
#define LEN_CONREFUSE          5

#define OFS_DISCONNECTING_PROTO    0
#define OFS_DISCONNECTING_REASON   4
#define LEN_DISCONNECTING          5

/* client block PDUs vector is dest MID */
#define OFS_CB_LENFLG          0
#define OFS_CB_DESTMID         OFS_VECTOR
#define OFS_CB_HEADER          OFS_VECTOR + LEN_CB_VECTOR
#define OFS_CB_PROTO           OFS_CB_HEADER
#define OFS_CB_ASSOC           OFS_CB_PROTO + 4
#define OFS_CB_PDU1DATA       10
#define LEN_CB_HEADER          6
#define LEN_CB_VECTOR          2

/* AOFS_ is absolute offset (from start of packet) */
#define SDTW_AOFS_CB (RLP_OFS_PDU1DATA + SDT_OFS_PDU1DATA + OFS_WRAPPER_CB)
#define SDTW_AOFS_CB_PDU1DATA (SDTW_AOFS_CB + OFS_CB_PDU1DATA)

/************************************************************************/
/*
Some helper structures relating to information in the packet
*/
/*
transport layer address - generic
*/
typedef uint8_t taddr_t[LEN_TA_OUT_MAX];

/*
Sequence numbers are signed to allow easy wrap-around calculations
*/
#define unmarshalSeq(x)  unmarshal32(x)
#define marshalSeq(ptr, x)  marshalU32((ptr), (uint32_t)(x))

/************************************************************************/
/*
group: SDT Structures and associated macros.
*/
/************************************************************************/
struct Lcomponent_s;   /* declared in component.h */
struct Rcomponent_s;   /* declared in component.h */
struct member_s;

/************************************************************************/
/*
type: struct chanParams_s

Channel Parameters – applied to each local channel and separately to
each local member of a remote channel.
*/
struct chanParams_s {
	uint8_t   expiry_sec;
	uint8_t   flags;
	uint16_t  nakholdoff;
	uint16_t  nakmodulus;
	uint16_t  nakmaxtime;
};

/*
types: SDT callback prototypes

clientRx_fn - handler for received client protocol data.
chanOpen_fn - handle incoming ad-hoc join messages [<sdt_register>].
memberevent_fn - handle out-of-band events e.g. EV_JOINSUCCESS, EV_LOSTSEQ
 see <membevent_e>
*/
typedef void clientRx_fn(struct member_s *memb, const uint8_t *data, int length, void *cookie);
typedef struct Lchannel_s *chanOpen_fn(ifMC(struct Lcomponent_s *Lcomp,) struct chanParams_s *params);
typedef void memberevent_fn(int event, void *object, void *info);

/************************************************************************/
/*
Client protocol handler - per local component
*/
struct sdt_client_s {
#if CF_SDT_MAX_CLIENT_PROTOCOLS > 1
	slLink(struct sdt_client_s, lnk);
	protocolID_t   protocol;
#endif
	clientRx_fn    *callback;
	void           *ref;
};

/*
Macros to interface to client structures
*/

#if CF_SDT_MAX_CLIENT_PROTOCOLS == 1
#define clientProto(clientp) CF_SDT_CLIENTPROTO
#define BADPROTO(proto) ((proto) != SDT_PROTOCOL_ID && (proto) != CF_SDT_CLIENTPROTO)
#else
#define clientProto(clientp) (clientp)->protocol
#define BADPROTO(proto) ((proto) == 0)
#endif

/************************************************************************/
/*
Local component structures

- If only one local component (CF_MULTI_COMPONENT) we keep the
details in a special  local component structure with only one global
instance so we never need to search for it.
- With multiple local components we have to deal with communication
between them and any local one may also be a remote one. However, we
keep two separate structures for local and remote reference.
*/

struct sdt_Lcomp_s {
	struct rlpsocket_s   *adhoc;
	chanOpen_fn          *joinRx;
	/* void                 *joinref; */
	memberevent_fn       *membevent;

	struct Lchannel_s    *Lchannels;
	uint8_t              flags;
	uint16_t             lastChanNo;
#if CF_SDT_MAX_CLIENT_PROTOCOLS == 1
	struct sdt_client_s  client;
#else
	struct sdt_client_s  *clients;
#endif
};

/*
Local component flags
*/

enum Lcomp_f {
	LCF_OPEN =1,
	LCF_LISTEN = 2
};

/************************************************************************/
/*
Remote component
*/

struct sdt_Rcomp_s {
	struct Rchannel_s   *Rchannels;
	netx_addr_t         adhocAddr;
};

/************************************************************************/
/*
	Channel and member structures

	Each local component may have multiple local channels and each
	channel may contain many members. A single remote component may be a
	member of multiple local channels and may have different state in
	each.

	Each remote component (there may be many) can have multiple channels
	and our local components may be members of several. However, one
	component cannot be a member of the same channel multiple times.

	Addresses for Local channels
	We can ensure the inbound address is always our adhoc address in
	current implementation but outbound address must be stored in
	channel.

	Addresses for remote channels
	We have no control over the remote inbound address and need to store
	it. We can ensure  that the outbound port is always SDT_MULTICAST so
	we just store the group address for remote channels.
*/
/************************************************************************/
/*
Local channel - owned by one of our components
*/

struct Lchannel_s {
	slLink(struct Lchannel_s, lnk);
#if CF_MULTI_COMPONENT
	struct Lcomponent_s         *owner;
#endif
	struct rlpsocket_s   *inwd_sk;
	netx_addr_t          outwd_ad;
	uint16_t             chanNo;
	uint16_t             membercount;
	uint16_t             himid;      /* MIDs start at 1. This is the highest assigned */
	uint16_t             lastmak;    /* MID of last MAK sent (normal cycle) */
	uint16_t             makthr;     /* MAK threshold (normal cycle) */
	uint16_t             makspan;    /* max no of members to MAK per wrapper */
	uint16_t             primakLo;   /* low MID of priority MAK range */
	uint16_t             primakHi;   /* high MID of priority MAK range */
	uint16_t             lastackmid;
	uint16_t             flags;
	uint16_t             backwraps;
	uint16_t             ackcount;   /* number of acks to expect for each wrapper */
	uint16_t             membspace;  /* index into mssizes[] */
	int32_t              Tseq;
	int32_t              Rseq;
	int32_t              nakfirst;
	int32_t              naklast;
	struct txwrap_s      *obackwrap; /* pointer to head of queue */
	struct txwrap_s      *nbackwrap; /* pointer to tail of queue */
	union Rmemb_u {
		struct member_s      *one;
		struct member_s      **many;
	}                    members;
	struct chanParams_s         params;    /* unless we want to assign each member its own? */
	acnTimer_t           blankTimer;
	acnTimer_t           keepalive;
	unsigned int         ka_t_ms;
#if CF_SDT_MAX_CLIENT_PROTOCOLS > 1
	protocolID_t         protocols[CF_SDT_MAX_CLIENT_PROTOCOLS];
#endif
};

/************************************************************************/
/*
Remote channel - owned by a remote copmponent
*/

struct Rchannel_s {
	slLink(struct Rchannel_s, lnk);
	struct Rcomponent_s *owner;
	netx_addr_t         inwd_ad;
	netx_addr_t         outwd_ad;  /* need to keep the outward address for downstream NAKs */
	struct rlpsocket_s  *outwd_sk;
	struct rxwrap_s     *aheadQ;
	acnTimer_t          NAKtimer;
	int32_t             Tseq;
	int32_t             Rseq;
	int32_t             lastnak;
	uint16_t            chanNo;
	uint8_t             NAKstate;
	uint8_t             NAKtries;
#if CF_MULTI_COMPONENT
	struct member_s            *members;
#endif
};

/************************************************************************/
/*
	Member structures

	Conceptually a local member of a remote channel is a completely
	separate entity from a remote member of a local channel and we define
	two separate structures. However because of the requirement to bond
	them in reciprocal pairs, we never have one without the other and it
	is vital to reference from one to the other easily - we therefore
	keep both in the same structure and save ourselves lots of links
	otherwise needed to track reciprocals.

	Furthermore, if we have only one local component
	(not CF_MULTI_COMPONENT), then it must be the sole member of each
	remote channel that we track, so we can unite the member and Rchannel
	structures.
*/
#define lastRmemb(Lchan) ((Lchan)->members)
#define firstRmemb(Lchan) ((Lchan)->members ? (Lchan)->members->rem.lnk.l : NULL)

struct member_s {
#if !CF_MULTI_COMPONENT
	struct Rchannel_s Rchan;
#endif
	struct loc_member_s{
#if CF_MULTI_COMPONENT
		slLink(struct member_s, lnk);
		struct Lcomponent_s       *Lcomp;
		struct Rchannel_s         *Rchan;
#endif
		int32_t            lastack;
		struct chanParams_s       params;
		acnTimer_t         expireTimer;
		uint16_t           mid;
		uint8_t            mstate;
	} loc;
	struct rem_member_s {
//      dlLink(struct member_s, lnk);
		struct Rcomponent_s *Rcomp;
		struct Lchannel_s          *Lchan;
		int32_t             Rseq;  /* the last Rseq acked */
		acnTimer_t          stateTimer;
		uint16_t            t_ms;
		uint16_t            mid;
		uint8_t             mstate;
		uint8_t             maktries;
	} rem;
#if CF_SDT_MAX_CLIENT_PROTOCOLS == 1
	uint8_t                connect;
#else
/* FIXME implement multiprotocol support */
#endif
};

enum connect_e {
	CX_LOCINIT = 1,
	CX_SDT = 2,
	CX_CLIENTLOC = 4,
	CX_CLIENTREM = 8,
};

/* member states */
enum mstate_e {
	MS_NULL = 0,
	MS_JOINRQ,  /* we've sent at least one join but not received a response */
	MS_JOINPEND,
	MS_MEMBER
};

/* member states */
enum NAKstate_e {
	NS_NULL = 0,
	NS_SUPPRESS,
	NS_HOLDOFF,
	NS_NAKWAIT
};

/************************************************************************/
/*
macros for single component simplification
*/
#if CF_MULTI_COMPONENT
//#define ctxtLcomp (rcxt->rlp.Lcomp)
#define ctxtLcomp(cx) ((cx)->Lcomp)
#define LchanOwner(Lchannelp) ((Lchannelp)->owner)
#define membLcomp(memb) ((memb)->loc.Lcomp)
#define get_Rchan(memb) ((memb)->loc.Rchan)
#define forEachMemb(memb, Rchan) for ((memb) = (Rchan)->members; (memb); (memb) = (memb)->loc.lnk.r)
#define firstMemb(Rchan) ((Rchan)->members)

#else
#define ctxtLcomp(cx) (&localComponent)
#define LchanOwner(Lchannelp) (&localComponent)
#define membLcomp(memb) (&localComponent)
#define get_Rchan(memb) ((memb)->Rchan.owner ? (&memb->Rchan) : NULL)
//#define forEachMemb(memb, Rchan) (memb = (struct member_s *)(Rchan));
#define forEachMemb(memb, Rchan) for ((memb = (struct member_s *)(Rchan));memb;memb = NULL)
#define firstMemb(Rchan) ((struct member_s *)(Rchan))
#endif

#define membRcomp(memb) ((memb)->rem.Rcomp)

/************************************************************************/
/*
Wrapper control - outgoing wrappers Sent wrappers are stored in a doubly
linked list with newest added at the head and oldest removed from the
tail.
*/
struct txwrap_s {
	uint8_t        *txbuf;
	uint8_t        *endp;
	unsigned int   size;
	unsigned int   usecount;
	union {
		struct open_s {
			struct Lchannel_s  *Lchan;
			uint32_t    prevproto;
			uint16_t    prevmid;
			uint16_t    prevassoc;
			uint16_t    prevflags;
			uint8_t    *prevdata;
			int         prevdlen;
			uint16_t    flags;
		} open;
		struct sent_s {
			slLink(struct txwrap_s, lnk);
			int32_t     Rseq;
			int         acks;
		} sent;
		struct fack_s {
			slLink(struct txwrap_s, lnk);
			acnTimer_t  rptTimer;
			int         t_ms;
		} fack;
	} st;
};

#define lastBackwrap(Lchan) ((Lchan)->nbackwrap)
#define firstBackwrap(Lchan) ((Lchan)->obackwrap)
/************************************************************************/
/*
Incoming packets - one packet (rxbuf) can contain multiple wrappers so
for each wrapper we create a tracking structure rxwrap_s. These are
stored before initial processing (if they arrive out of order) and when
queued for application, in a doubly linked list in sequence order with
newest at the head and oldest at the tail.
*/

struct rxwrap_s {
	dlLink(struct rxwrap_s, lnk);
	struct rxbuf_s     *rxbuf;
//	struct netx_context_s netx;
	struct Rchannel_s  *Rchan;
	const uint8_t      *data;
	int32_t            Tseq;
	int32_t            Rseq;
	int                length;
	bool               reliable;
};

/************************************************************************/
/*
Prototypes
*/
struct mcastscope_s;

int sdt_register(ifMC(struct Lcomponent_s *Lcomp,) 
		memberevent_fn *membevent, netx_addr_t *adhocip, 
		chanOpen_fn *joinRx);

void sdt_deregister(ifMC(struct Lcomponent_s *Lcomp));

struct Lchannel_s *openChannel(ifMC(struct Lcomponent_s *Lcomp,) uint16_t flags, struct chanParams_s *params);

void closeChannel(struct Lchannel_s *Lchan);

extern struct Lchannel_s *autoJoin(ifMC(struct Lcomponent_s *Lcomp,) struct chanParams_s *params);
#define ADHOCJOIN_NONE NULL
#define ADHOCJOIN_ANY (&autoJoin)

extern int addMember(struct Lchannel_s *Lchan, struct Rcomponent_s *Rcomp);

void drop_member(struct member_s *memb, uint8_t reason);

int sdt_addClient(ifMC(struct Lcomponent_s *Lcomp,) clientRx_fn *rxfn, void *ref);

void sdt_dropClient(ifMC(struct Lcomponent_s *Lcomp));

struct txwrap_s *startWrapper(int size);

struct txwrap_s *startMemberWrapper(struct member_s *memb, int size, uint16_t wflags);

void cancelWrapper(struct txwrap_s *txwrap);

uint8_t *startProtoMsg(struct txwrap_s **txwrapp, void *dest,
								protocolID_t proto, uint16_t wflags, int *sizep);

int endProtoMsg(struct txwrap_s *txwrap, uint8_t *endp);

int rptProtoMsg(struct txwrap_s **txwrapp, struct member_s *memb, uint16_t wflags);

int addProtoMsg(struct txwrap_s **txwrapp, struct member_s *memb, protocolID_t proto,
								uint16_t wflags, const uint8_t *data, int size);

int _flushWrapper(struct txwrap_s *txwrap, int32_t *Rseqp);

static inline int flushWrapper(struct txwrap_s **txwrapp)
{
	int rslt = 0;

	if (txwrapp == NULL) return -1;
	if (*txwrapp) {
		rslt = _flushWrapper(*txwrapp, NULL);
		*txwrapp = NULL;
	}
	return rslt;
}

int sendWrap(struct member_s *memb, protocolID_t proto,
					uint16_t wflags, const uint8_t *data, int size);
/*
wrapper flags
*/
#define WRAP_REL_DONTCARE  0x0000
#define WRAP_REL_ON        0x0001
#define WRAP_REL_OFF       0x0002
#define WRAP_REPLY         0x0004
#define WRAP_ALL_MEMBERS   0x0008
#define WRAP_NOAUTOACK     0x0010
#define WRAP_NOAUTOFLUSH   0x0020

#define WHOLE_WRAP_FLAGS  (WRAP_REL_ON | WRAP_REL_OFF | WRAP_NOAUTOACK)

/* Some flags are mutually exclusive */
#define WRAP_REL_ERR(flags) (((flags) & (WRAP_REL_ON | WRAP_REL_OFF)) == (WRAP_REL_ON | WRAP_REL_OFF))
#define WRAP_ALL_ERR(flags) (((flags) & (WRAP_REPLY | WRAP_ALL_MEMBERS)) == (WRAP_REPLY | WRAP_ALL_MEMBERS))
#define WRAP_FLAG_ERR(flags) (WRAP_REL_ERR(flags) || WRAP_ALL_ERR(flags))

/*
enum: Channel Flags

	CHF_UNICAST - unicast channel
	CHF_RECIPROCAL - is reciprocal for remote initiated channel
	CHF_NOAUTOCON - do not automatically inssue connect requests
	CHF_NOCLOSE - do not automatically close channel when last member is removed
*/
enum Lchan_flg {
	CHF_UNICAST =1,
	CHF_RECIPROCAL = 2,
	CHF_NOAUTOCON = 4,
	CHF_NOCLOSE = 8,
};

/*
Reason codes for App
*/

enum appReason_e {
	APP_REASON_ADHOC_TIMEOUT = 1,
	APP_REASON_NO_RECIPROCAL
};

enum membevent_e {
	EV_RCONNECT,
	EV_LCONNECT,
	EV_DISCOVER,
	EV_JOINFAIL,
	EV_JOINSUCCESS,
	EV_LOCCLOSE,
	EV_LOCDISCONNECT,
	EV_LOCDISCONNECTING,
	EV_LOCLEAVE,
	EV_LOSTSEQ,
	EV_MAKTIMEOUT,
	EV_NAKTIMEOUT,
	EV_REMDISCONNECT,
	EV_REMDISCONNECTING,
	EV_REMLEAVE,
	EV_CONNECTFAIL,
};

#define FIRSTACK_REPEAT_ms 85
#define MAXACK_REPEAT_ms   (2 * FIRSTACK_REPEAT_ms)

#define DEFAULT_DISCOVERY_EXPIRE 5

#endif
