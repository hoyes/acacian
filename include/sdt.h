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

topic: SDT API Overview

Setting up:

To use SDT a component must first register using <sdt_register>. This
initializes SDT if necessary, provides or generates a local address
for handling ad-hoc messages and tells SDT how to handle ad-hoc Join 
requests. Finally it provides a callback which SDT calls for out-of-band
events to manage the status of sessions [<membevent_e>].

Next, the component must register one or more client protocols with the
SDT layer using <sdt_addClient>. This provides SDT with a callback 
function to handle incoming wrapped messages for that protocol. For
DMP the callback should be <dmp_sdtRx>.

Locally Initiated Sessions:

To create its own sessions a component must first call <openChannel> 
which sets the session parameters and returns a Lchannel_s. It can 
then call <addMember> using information from discovery to add 
members to that channel.

Once a channel is open and has members, the 
component can transmit messages either to individual members or to 
all members together.

Remotely Initiated Sessions:

Behavior on receipt of a remote iniated Join depends on the joinRx 
argument passed to <sdt_register>. If <ADHOCJOIN_NONE> was passed such 
joins are simply refused while <ADHOCJOIN_ANY> asks SDT to accept all
requests.

The joinRx function also determines the strategy for assigning
reciprocal channels to remote initiated sessions. The default 
behavior if <ADHOCJOIN_ANY> is used is to create a new unicast 
reciprocal channel for each remote-led session. This is by far the 
most network-efficient strategy, probably reduces processing burden 
on the remote component and may also do so locally. It is good 
for most components. However, components designed for very large 
numbers of incoming connections may run into socket limitations 
or find more memory resources are required using this strategy, in 
which case a strategy which allocates one or more large multicast 
channels to act as reciprocals may work better.

Most DMP `devices` will want to use ADHOCJOIN_ANY since this is how 
they listen for connections from controllers. Limited DMP 
`controllers` may use ADHOCJOIN_NONE, but most controllers will want 
to receive events from devices for which those devices need to open 
event sessions back to them so they will either use ADHOCJOIN_ANY or 
supply a more discriminating function of their own. (ADHOCJOIN_ANY 
translates to a call to the SDT internal function <mkRecip> which 
provides a template for writing such a function or may be called 
directly after making the necessary tests.)

Connection Handling:

Once SDT is connected to a remote component via a reciprocal pair of 
channels the usual action is to send `connect` messages for the
desired client protocol(s).

For incoming `connect` messages, SDT will accept the connection for
any protocol for which it has handlers registered [<sdt_addClient>].

For locally led sessions the default is to automatically send 
`connect` for all protocols that are registered as soon as the 
channels are established. This behavior is determined by the 
<CF_SDT_MAX_CLIENT_PROTOCOLS> configuration option and by the 
<CHF_NOAUTOCON> flag passed to <openChannel> but support for both 
these is only partially implemented at present (Feb 2014).

*Note:* the SDT spec is not clear about whether the `connect, 
connect accept` exchange to establish a connection needs to be 
initiated at one particular end or at both. Acacian will accept 
connect requests from remote members irrespective of which end 
initiated the connections or whether a connection has already been 
established from either end.

Receiving:

Once a connection is open, received wrappers are processed and
queued in sequence then when the wrapper is processed, SDT unpacks 
it and passes any PDUs destined for local components and for registered
protocols to the `rxfn` handler passed to <sdt_addClient>.

Transmitting:

A number of functions are provided to allow aggregation of messages
into wrappers and packets and transmission of them.

- <startWrapper> – start a new wrapper
- <cancelWrapper> – cancel a partly constructed wrapper
- <startProtoMsg> – start building a message to one or all members
- <endProtoMsg> – complete a message
- <rptProtoMsg> – repeat the previous message to another member of the
same group
- <addProtoMsg> – add a complete message to a wrapper
- <flushWrapper> – transmit a wrapper
- <sendWrap> – create a wrapper, copy in a message and send it

topic: Discovery in SDT

SDT messages Join and Channel Parameters include ad-hoc address and 
expiry information that has belongs in Discovery and has no place in 
SDT. It is not at all clear how this is intended to be used. This is 
a design flaw and the information is unnecessary.

Acacian does not use this information but does pass the packet data 
to the application using a member-event. The demo applications 
ignore it.
*/

#ifndef __sdt_h__
#define __sdt_h__ 1

/**********************************************************************/
/*
packet offsets

SDT_OFS_PDU1DATA - data in any SDT bottom layer (length/flags and vector)
OFS_WRAPPER_CB - offset to the client block in a wrapper
*/
#define SDT_OFS_PDU1DATA     3
#define OFS_WRAPPER_CB        20

/*
Within a client block

Client block PDUs vector is dest MID, header is protocol + association
*/
#define OFS_CB_LENFLG          0
#define OFS_CB_DESTMID         OFS_VECTOR
#define OFS_CB_HEADER          OFS_VECTOR + LEN_CB_VECTOR
#define OFS_CB_PROTO           OFS_CB_HEADER
#define OFS_CB_ASSOC           OFS_CB_PROTO + 4
#define OFS_CB_PDU1DATA       10
#define LEN_CB_HEADER          6
#define LEN_CB_VECTOR          2

/*
AOFS_ is absolute offset (from start of packet)
*/
#define SDTW_AOFS_CB (RLP_OFS_PDU1DATA + SDT_OFS_PDU1DATA + OFS_WRAPPER_CB)
#define SDTW_AOFS_CB_PDU1DATA (SDTW_AOFS_CB + OFS_CB_PDU1DATA)

/************************************************************************/
/*
group: SDT Structures and associated macros.
*/
/************************************************************************/
struct Lcomponent_s;   /* declared in component.h */
struct Rcomponent_s;   /* declared in component.h */
struct member_s;
struct mcastscope_s;

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
group: SDT callback functions

func: clientRx_fn
Callback function (registered in<sdt_addClient>) to handle received 
client protocol data.

memb - the member which sent the message.
data - pointer to the data (*do not modify* the data, modify a copy if
necessary).
length - the length of the data block.
cookie - the user data pointer that was passed to <sdt_addClient>.

*/
typedef void clientRx_fn(struct member_s *memb, const uint8_t *data, int length, void *cookie);

/*
func: chanOpen_fn

Callback function (registered in <sdt_register> as `joinRx`) to handle 
unsolicited Join requests.

Lcomp - The component being requested to join (only if 
<CF_MULTI_COMPONENT>).
params - The requested parameters for the incoming channel (the
reciprocal channel will want to be compatible).

Return: The callback must return an open channel to use as a reciprocal
for the Join, or NULL to refuse the join.
*/
typedef struct Lchannel_s *chanOpen_fn(ifMC(struct Lcomponent_s *Lcomp,) struct chanParams_s *params);

/*
func: memberevent_fn

Callback function (registered in <sdt_register> as `membevent`) to 
handle out-of-band session or channel events.

event - The event that has occurred (see <membevent_e>.
object - the object (channel, member etc.) that causes the event.
info - extra data relating to the event as required.
*/
typedef void memberevent_fn(int event, void *object, void *info);

/************************************************************************/
/*
type: sdt_client_s

Client protocol handler - per local component. If there is just one 
client protocol (<CF_SDT_MAX_CLIENT_PROTOCOLS> == 1) we just have 
one of these which slots into the <sdt_Lcomp_s>, otherwise there is 
a list of them.
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
type: sdt_Lcomp_s

Local component structure for SDT. This slots into the global 
<Lcomponent_s> and holds the SDT relevant information.
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

/************************************************************************/
/*
type: sdt_Rcomp_s

Remote component structure for SDT. This slots into the global 
<Rcomponent_s> and holds the SDT relevant information.
*/

struct sdt_Rcomp_s {
	struct Rchannel_s   *Rchannels;
	netx_addr_t         adhocAddr;
};

/************************************************************************/
/*
group: Channel and member structures

Each local component may have multiple local channels and each
channel may contain many members. A single remote component may be a
member of multiple local channels and may have different state in
each.

Each remote component (there may be many) can have multiple channels
and our local component(s) may be members of several. However, one
component cannot be a member of the same channel multiple times.
*/
/************************************************************************/
/*
type: Lchannel_s

Local channel owned by one of our components
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
type: Rchannel_s

Remote channel owned by a remote copmponent
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

#define lastRmemb(Lchan) ((Lchan)->members)
#define firstRmemb(Lchan) ((Lchan)->members ? (Lchan)->members->rem.lnk.l : NULL)
/************************************************************************/
/*
type: member_s

Member structures

Conceptually a local member of a remote channel is a completely
separate entity from a remote member of a local channel and we define
two separate structures. However because of the requirement to bond
them in reciprocal pairs, we never have one without the other and it
is vital to reference from one to the other easily – we therefore
keep both in the same enclosing structure and save ourselves lots of 
links otherwise needed to track reciprocals.

Furthermore, if we have only one local component (not 
<CF_MULTI_COMPONENT>), then it `must` be the sole member of each 
remote channel that we track, so we can unite the member and Rchannel
structures. The detail of this is masked by macros to give a 
consistent API.
*/
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

/************************************************************************/
/*
macros: Moving between components, channels and members

Use these macros rather than accessing structures directly because 
they hide differences due to <CF_MULTI_COMPONENT> and implementation 
changes.

ctxtLcomp(rcxt) - get the local component from receive context.
LchanOwner(Lchan) - get the owner (local component) of a local channel.
membLcomp(memb) - get the local component from a member
get_Rchan(memb) - get the remote channel from a member
forEachMemb(memb, Rchan) - iterate over the members of a remote channel
firstMemb(Rchan) - get the first member of a remote channel
membRcomp(memb) - get the remote component from a member
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
type: txwrap_s

Outgoing wrappers. Sent reliable wrappers must be kept until verified. They 
are stored in a doubly linked list with newest added at the head and 
oldest removed from the tail.
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
type: rxwrap_s

Incoming wrappers. One packet (<rxbuf>) may contain multiple wrappers so
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

/*
group: SDT initialization and session management

func: sdt_register

Lcomp - the component registering (only if <CF_MULTI_COMPONENT>).
membevent - call back [<memberevent_fn>] for handling session and 
membership events.
adhocip - in/out parameter specifying local address and/or port. If 
NULL RLP defaults to ephemeral port and any local interface. If 
adhocip is explicitly specified as ephemeral port 
(netx_PORT_EPHEM) and any local interface (INADDR_ANY) then 
adhocip gets filled in with the actual port used which can then be 
advertised for discovery [.
joinRx - Application supplied callback function [<chanOpen_fn>] to handle ad-hoc 
join requests. Alternatively specify ADHOCJOIN_NONE (refuse all 
requests) or ADHOCJOIN_ANY (accept all requests).

*See also:* <rlpSubscribe>
*/
int sdt_register(ifMC(struct Lcomponent_s *Lcomp,) 
		memberevent_fn *membevent, netx_addr_t *adhocip, 
		chanOpen_fn *joinRx);

/*
func: sdt_deregister

De-register a component from SDT.
Lcomp - the component to de-register (only if <CF_MULTI_COMPONENT>).
*/
void sdt_deregister(ifMC(struct Lcomponent_s *Lcomp));

/*
func: openChannel

Create and initialize a new channel.

Lcomp - the component registering (only if <CF_MULTI_COMPONENT>).
flags - see <Channel Flags>.
params - if params is NULL then defaults are used. If params is 
provided then these are used for the new channel unless the 
RECIPROCAL flag is set, in which case the  parameters supplied are 
assumed to be from an incoming channel and the new channel's 
parameters are matched to these.

Returns:
struct Lchannel_s * on success, on failure NULL is returned and errno 
set accordingly.

Errors:
	EINVAL supplied parameters are invalid
	ENOMEM couldn't allocate a new struct Lchannel_s
*/
struct Lchannel_s *openChannel(ifMC(struct Lcomponent_s *Lcomp,) uint16_t flags, struct chanParams_s *params);

/*
func: closeChannel

Close a channel removing all members if necessary.

Lchan - the channel to close
*/
void closeChannel(struct Lchannel_s *Lchan);

/*
func: mkRecip

Create a reciprocal channel. This function is normally invoked 
automatically when <ADHOCJOIN_ANY> is passed to <sdt_register>, however
it may be called after vetting incoming Join requests to implement
Join policy.

Lcomp - the local component (only if <CF_MULTI_COMPONENT>).
params - the channel parameters from the incoming Join request.
*/
extern struct Lchannel_s *mkRecip(ifMC(struct Lcomponent_s *Lcomp,) struct chanParams_s *params);

/*
macros: Ad hoc Join handlers

These are special values for the joinRx argument to <sdt_register>. 
They only affect ‘cold’ Joins received unsolicited from remote 
components. ‘warm’ or reciprocal Join requests creating reciprocal 
channels for locally initiated sessions are always accepted.

ADHOCJOIN_NONE - Refuse all ‘cold’ Join requests.
ADHOCJOIN_ANY - Accept all ‘cold’ Join requests.
*/
#define ADHOCJOIN_NONE NULL
#define ADHOCJOIN_ANY (&mkRecip)

/*
func: addMember

Create a new member and add it to a channel and send a cold Join.

Note: Completion of the add-member process is asynchronous. A 
successful return status from addMember indicates only that the 
member structure was successfully created and the Join request sent. 
The status of the complete process is received by callbacks to 
*membevent* which was passed to <sdt_register>. On success a 
EV_JOINSUCCESS message is passed (with the new member structure). On 
failure EV_JOINFAIL will be sent.
*/
extern int addMember(struct Lchannel_s *Lchan, struct Rcomponent_s *Rcomp);

/*
func: drop_member

Drop a member from both its channels. Disconnects all protocols as 
necessary then sends `Leave` in the Local channel, `Leaaving` in the 
remote channel. reason is sent as the reason code in the 
disconnecting and leaving messages.
*/
void drop_member(struct member_s *memb, uint8_t reason);

/*
func: sdt_addClient

Register a client protocol handler with SDT. If acacian has been 
compiled with <CF_SDT_MAX_CLIENT_PROTOCOLS> == 1 (the default) then
the protocol is not passed in the call but the call is still necessary
to pass the receive function and initialize the structures.

Lcomp - the local component (only if <CF_MULTI_COMPONENT>).
protocol - the protocol ID being registered (only if 
<CF_SDT_MAX_CLIENT_PROTOCOLS> > 1)
rxfn - callback function to handle successfully received data for 
cookie - Application data pointer which gets passed to rxfn.
*/
int sdt_addClient(ifMC(struct Lcomponent_s *Lcomp,)
		ifSDT_MP(protocolID_t protocol,) clientRx_fn *rxfn, void *cookie);

/*
func: sdt_dropClient

De-register a client protocol from the SDT layer.
*/
void sdt_dropClient(ifMC(struct Lcomponent_s *Lcomp));

/*
group: SDT transmit functions
*/
/*
func: startWrapper


Create and initialize a txwrap.

Returns:

New txwrap on success, NULL on error.

Errors:
	EMSGSIZE - size is too big
*/
struct txwrap_s *startWrapper(int size);

/*
func: startMemberWrapper

*/
struct txwrap_s *startMemberWrapper(struct member_s *memb, int size, uint16_t wflags);

/*
func: cancelWrapper

Cancel a txwrap
*/
void cancelWrapper(struct txwrap_s *txwrap);

/*
func: startProtoMsg

Initialize the next message block in the wrapper

returns:
	Pointer to place to put the data
*/
uint8_t *startProtoMsg(struct txwrap_s **txwrapp, void *dest,
								protocolID_t proto, uint16_t wflags, int *sizep);

/*
func: endProtoMsg

Close a message after adding data

endp - points to the end of the PDUs you have added
*/
int endProtoMsg(struct txwrap_s *txwrap, uint8_t *endp);

/*
func: rptProtoMsg

Repeat the previous message to another member of the same group.

Returns:
The remaining space (positive) on success, or -1 on error.
*/
int rptProtoMsg(struct txwrap_s **txwrapp, struct member_s *memb, uint16_t wflags);

/*
func: addProtoMsg

*/
int addProtoMsg(struct txwrap_s **txwrapp, struct member_s *memb, protocolID_t proto,
								uint16_t wflags, const uint8_t *data, int size);

/*
func: _flushWrapper

*/
int _flushWrapper(struct txwrap_s *txwrap, int32_t *Rseqp);

/*
func: flushWrapper

*/
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

/*
func: sendWrap

*/
int sendWrap(struct member_s *memb, protocolID_t proto,
					uint16_t wflags, const uint8_t *data, int size);
/*
macros: wrapper flags

When accumulating messages in a wrapper, some require reliable 
transmission. Of those that do not most don't care whether the 
wrapper is reliable or not. However, there are a few messages that 
may `require` that the wrapper be sent unreliably and these cannot 
be mixed in the same wrapper as messages that `require` reliability.

WRAP_REL_DONTCARE - Message doesn't care whether wrapper is reliable 
or not.
WRAP_REL_ON - Message requires reliable wrapper.
WRAP_REL_OFF - Message requires unreliable wrapper.
WRAP_REPLY - Message is a reply (SDT sets the association field 
appropriately).
WRAP_ALL_MEMBERS - Message is to all members of the channel.
WRAP_NOAUTOACK - Do not add background ACKs to the wrapper.
WRAP_NOAUTOFLUSH - Do not automatically flush full or incompatible 
wrappers.
*/
#define WRAP_REL_DONTCARE  0x0000
#define WRAP_REL_ON        0x0001
#define WRAP_REL_OFF       0x0002
#define WRAP_REPLY         0x0004
#define WRAP_ALL_MEMBERS   0x0008
#define WRAP_NOAUTOACK     0x0010
#define WRAP_NOAUTOFLUSH   0x0020

/*
enum: Channel Flags

	CHF_UNICAST - unicast channel
	CHF_RECIPROCAL - is reciprocal for remote initiated channel
	CHF_NOAUTOCON - do not automatically issue connect requests
	CHF_NOCLOSE - do not automatically close channel when last member is removed
*/
enum Lchan_flg {
	CHF_UNICAST =1,
	CHF_RECIPROCAL = 2,
	CHF_NOAUTOCON = 4,
	CHF_NOCLOSE = 8,
};
/**********************************************************************/
/*
type: membevent_e

Out of band SDT events as passed to membevent callback [<sdt_register>].
The event callback is passed two arguments in addition to the event: 
`object` and `info` [<memberevent_fn>]. The values of these depend on 
the message.

	EV_RCONNECT - Remote initiated connect succeeded. object=Local 
	channel, info=member.

	EV_LCONNECT - Local initiated connect succeeded. object=Local 
	channel, info=member.

	EV_DISCOVER - Discovery information [<Discovery in SDT>]. object 
	= Remote component, info=pointer to packet data.

	EV_JOINFAIL - An attempt to join a remote component to a local 
	channel has failed. object=Local channel, info=Remote 
	component.

	EV_JOINSUCCESS - Join succeded. object=Local channel, info=
	member. This event occurs whoever initiated the join, once both 
	local and remote channel are Joined.

	EV_LOCCLOSE - used internally, never passed to membevent

	EV_LOCDISCONNECT - A connection that was locally initiated has 
	disconnected. object=Local channel, info=member.

	EV_LOCDISCONNECTING - A connection that was remotely initiated has 
	disconnected. object=Local channel, info=member.

	EV_LOCLEAVE - member is being asked to leave the channel. object 
	= Local channel, info=member.

	EV_LOSTSEQ - Channel pair is being closed because we've lost 
	sequence in the remote channel. object=Local channel, info=
	member.

	EV_MAKTIMEOUT - Channel pair is being closed because the remote 
	did not Ack in the local channel when requested. object=Local 
	channel, info=member.

	EV_NAKTIMEOUT - Channel pair is being closed because NAK for lost 
	wrappers failed. object=Local channel, info=member.

	EV_REMDISCONNECT - Remote has sent a `disconnect` message. object=
	Local channel, info=member.

	EV_REMDISCONNECTING - Remote has sent a `disconnecting` message. 
	object=Local channel, info=member.

	EV_REMLEAVE - used internally, never passed to membevent

	EV_CONNECTFAIL - A locally initiated connect request was refused. 
	object=Local channel, info=member.
*/
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

#endif
