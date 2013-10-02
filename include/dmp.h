/**********************************************************************/
/*

   Copyright (c) 2011, Philip Nye, Engineering Arts (UK) philip@engarts.com
   All rights reserved.

   Author: Philip Nye

*/
/**********************************************************************/
/*
#tabs=3t
*/
/*
file: dmp.h

Device Management Protocol

*Note:* This implementation is incomplete (April 2012). Functions
to send and receive messages are implemented but the code still needs
functions for establishing connections and out-of-band connection status
are needed. A number of prototypes for a suggested API are given
at the end.

Section: DMP Message format
All DMP messages relate to DMP addresses within a component and so specify
one or more addresses.

Certain messages must also specify data relating to each address 
which is either a value for the property at that address, or a 
reason code specifying why a command for that address has failed. 
The presence of data and whether it is a value or a reason code is 
fixed for each specific message code.

DMP uses the standard PDU format of ACN in which each PDU has three fields.

For DMP the message fields aree defined as:

vector - One octet message code.
header - One octet address encoding, determines single vs range address,
			field size, single vs multiple data, absolute or relative (to 
			previously used) address.
data -	One or more address fields or address+data fields as determined
			by the message code. The number of fields is determined by the 
			PDU length.

group: Macros for address types

macros:
	IS_RANGE(header) - True if the header value specifies a range address
	IS_RELADDR(header) - True if the header specifies a relative address
	IS_MULTIDATA(header) - True if there is a data value for each address
			in the range
	IS_RANGECOMMON(header) - True if there is just one value applying to
			all addresses in the range
	
*/
#ifndef __dmp_h__
#define __dmp_h__ 1

#define IS_RANGE(type) (((type) & DMPAD_TYPEMASK) != DMPAD_SINGLE)
#define IS_RELADDR(type) (((type) & DMPAD_R) != 0)
#define IS_MULTIDATA(type) (((type) & DMPAD_RANGE_ARRAY) != 0)
#define IS_RANGECOMMON(type) (((type) & DMPAD_TYPEMASK) == DMPAD_RANGE_SINGLE)

/*
Group: Combined message code and address modes

For many purposes
eaACN combines the message code with the address type (header) field.
These macros define all the combined message/address codes required
by eaACN's transmit code - there are other permitted values but they 
mostly have arcane use-cases or duplicate functionality.

*Note:* the choice of relative/absolute addressing is made automatically
as PDUs are constructed.

*Note:* dmp_openpdu() will substitute the single address version 
whenever count == 1 so multiple address versions can normally be used

Macros: Getting Property Values
	PDU_GETPROP_ONE   - Get Property, single address (no values)
	PDU_GETPROP_MANY  - Get Property, range address (no values)
	PDU_GPREPLY_ONE   - Get property reply, single address + value
	PDU_GPREPLY_MANY  - Get property reply, range address + one value per
								address
	PDU_GPFAIL_ONE    - Get Property Fail, single address + reason
	PDU_GPFAIL_MANY   - Get Property Fail, range address + one reason per address
	PDU_GPFAIL_COMMON - Get Property Fail, range address + common reason

Macros: Setting Property Values
	PDU_SETPROP_ONE   - Set Property, single address + value
	PDU_SETPROP_MANY  - Set Property, range address + one value per address
	PDU_SETPROP_COMMON - Set property, range address + common value
	PDU_SPFAIL_ONE    - Set Property Fail, single address + reason
	PDU_SPFAIL_MANY   - Set Property Fail, range address + one reason per address
	PDU_SPFAIL_COMMON - Set Property Fail, range address + common reason

Macros: Events
	PDU_EVENT_ONE     - Event, single address + value
	PDU_EVENT_MANY    - Event, range address + one value per address
	PDU_SYNCEV_ONE    - Sync Event, single address + value
	PDU_SYNCEV_MANY   - Sync Event, range address + one value per address

	PDU_SUBS_ONE      - Subscribe, single address (no values)
	PDU_SUBS_MANY     - Subscribe, range address (no values)
	PDU_SUBOK_ONE     - Subscribe Accept, single address (no values)
	PDU_SUBOK_MANY    - Subscribe Accept, range address (no values)
	PDU_SUBREJ_ONE    - Subscribe Reject, single address + reason
	PDU_SUBREJ_MANY   - Subscribe Reject, range address + one reason per address
	PDU_SUBREJ_COMMON - Subscribe Reject, range address + common reason

	PDU_USUBS_ONE     - Unsubscribe, single address (no values)
	PDU_USUBS_MANY    - Unsubscribe, range address (no values)
*/

#define PDU_GETPROP_ONE      ((DMP_GET_PROPERTY << 8) | DMPAD_SINGLE)
#define PDU_SETPROP_ONE      ((DMP_SET_PROPERTY << 8) | DMPAD_SINGLE)
#define PDU_GPREPLY_ONE      ((DMP_GET_PROPERTY_REPLY << 8) | DMPAD_SINGLE)
#define PDU_EVENT_ONE        ((DMP_EVENT << 8) | DMPAD_SINGLE)
#define PDU_SUBS_ONE         ((DMP_SUBSCRIBE << 8) | DMPAD_SINGLE)
#define PDU_USUBS_ONE        ((DMP_UNSUBSCRIBE << 8) | DMPAD_SINGLE)
#define PDU_GPFAIL_ONE       ((DMP_GET_PROPERTY_FAIL << 8) | DMPAD_SINGLE)
#define PDU_SPFAIL_ONE       ((DMP_SET_PROPERTY_FAIL << 8) | DMPAD_SINGLE)
#define PDU_SUBOK_ONE        ((DMP_SUBSCRIBE_ACCEPT << 8) | DMPAD_SINGLE)
#define PDU_SUBREJ_ONE       ((DMP_SUBSCRIBE_REJECT << 8) | DMPAD_SINGLE)
#define PDU_SYNCEV_ONE       ((DMP_SYNC_EVENT << 8) | DMPAD_SINGLE)

#define PDU_GETPROP_MANY     ((DMP_GET_PROPERTY << 8) | DMPAD_RANGE_NODATA)
#define PDU_SETPROP_MANY     ((DMP_SET_PROPERTY << 8) | DMPAD_RANGE_STRUCT)
#define PDU_SETPROP_COMMON   ((DMP_SET_PROPERTY << 8) | DMPAD_RANGE_SINGLE)
#define PDU_GPREPLY_MANY     ((DMP_GET_PROPERTY_REPLY << 8) | DMPAD_RANGE_STRUCT)
#define PDU_EVENT_MANY       ((DMP_EVENT << 8) | DMPAD_RANGE_STRUCT)
#define PDU_SUBS_MANY        ((DMP_SUBSCRIBE << 8) | DMPAD_RANGE_NODATA)
#define PDU_USUBS_MANY       ((DMP_UNSUBSCRIBE << 8) | DMPAD_RANGE_NODATA)
#define PDU_GPFAIL_MANY      ((DMP_GET_PROPERTY_FAIL << 8) | DMPAD_RANGE_STRUCT)
#define PDU_GPFAIL_COMMON    ((DMP_GET_PROPERTY_FAIL << 8) | DMPAD_RANGE_SINGLE)
#define PDU_SPFAIL_MANY      ((DMP_SET_PROPERTY_FAIL << 8) | DMPAD_RANGE_STRUCT)
#define PDU_SPFAIL_COMMON    ((DMP_SET_PROPERTY_FAIL << 8) | DMPAD_RANGE_SINGLE)
#define PDU_SUBOK_MANY       ((DMP_SUBSCRIBE_ACCEPT << 8) | DMPAD_RANGE_STRUCT)
#define PDU_SUBREJ_MANY      ((DMP_SUBSCRIBE_REJECT << 8) | DMPAD_RANGE_STRUCT)
#define PDU_SUBREJ_COMMON    ((DMP_SUBSCRIBE_REJECT << 8) | DMPAD_RANGE_SINGLE)
#define PDU_SYNCEV_MANY      ((DMP_SYNC_EVENT << 8) | DMPAD_RANGE_STRUCT)

/**********************************************************************/
/*
Section: Connections
DMP components communicate through *connections* which depend on the 
transport it is using

macros: Connections

Macros to abstract DMP connections

DMP uses connection as an absraction whose definition depends on the 
transport layer. For single transport implementations such as DMP on SDT
or DMP on TCP, this macro is typically just defined to whatever structure
the transport layer expects. For an implementation which supports multiple
transports it will need to be more complex.

cxn_s - connection structure
cxnLcomp(cxn) - get the local component for the connection
cxnRcomp(cxn) - get the remote component for the connection
cxngp_s - groupsconnections together with a set of common parameters 
and allows group messaging to all members. Where transport supports 
it this uses true group messaging, for transports which are unicast 
only, code should emulated group messaging by copying the message to 
each member
cxnpars_s - parameters for a connection group
*/

#if ACNCFG_SDT
#define cxn_s member_s
#define cxnLcomp membLcomp
#define cxnRcomp(cxn) ((cxn)->rem.Rcomp)
#define cxngp_s Lchannel_s
#define cxnpars_s chanParams_s
#define getcxngp(cxn) ((cxn)->rem.Lchan)

static inline struct cxngp_s *
new_cxngp(ifMC(struct Lcomponent_s *Lcomp,) uint16_t flags,
						struct cxnpars_s *params)
{
	return openChannel(ifMC(Lcomp,) flags, params);
}

static inline int
dmp_connectRq(struct cxngp_s *cxngp, struct Rcomponent_s *Rcomp)
{
	return addMember(cxngp, Rcomp);
}

#endif

/**********************************************************************/
/*
Packet structure lengths
*/
#define DMP_BLOCK_MIN (OFS_VECTOR + DMP_VECTOR_LEN + DMP_HEADER_LEN)

/**********************************************************************/
/*
Section: Components
DMP related data for local or remote components

we need some partial structure pre-declarations
*/
struct Lcomponent_s;
#if ACNCFG_EPI10
struct mcastscope_s;
#endif
struct cxn_s;
struct txwrap_s;
struct dmptcxt_s;
struct cxngp_s;

struct adspec_s {
	uint32_t addr;
	uint32_t inc;
	uint32_t count;
};

#if !ACNCFG_PROPEXT_FNS
struct dmprcxt_s;

typedef int dmprx_fn(struct dmprcxt_s *rcxt,
						const struct dmpprop_s *dprop,
						struct adspec_s *ads);

typedef int dmprxd_fn(struct dmprcxt_s *rcxt,
						const struct dmpprop_s *dprop,
						struct adspec_s *ads,
						const uint8_t *data,
						bool dmany);
#endif

typedef void dmp_cxnev_fn(struct cxn_s *cxn, bool connect);

/*
struct: dmp_Lcomp_s
Local component DMP layer structure
*/
struct dmp_Lcomp_s {
#if ACNCFG_DMP_DEVICE && !defined(ACNCFG_DMPMAP_NAME)
	/*pointer: map*/
	union addrmap_u *amap;
#endif
	dmprx_fn *rxvec[DMP_MAX_VECTOR];
	dmp_cxnev_fn *cxnev;
	uint8_t flags;
};

enum dmpLcompflg_e {
	ACTIVE_LDMPFLG = 1,
};

/*
struct: dmp_Rcomp_s
Remote component DMP layer structure
*/
struct dmp_Rcomp_s {
#if ACNCFG_DMP_CONTROLLER
	/*pointer: map*/
	union addrmap_u *amap;
#endif
	unsigned int ncxns;
	struct cxn_s *cxns[ACNCFG_DMP_RMAXCXNS];
};

/**********************************************************************/
#if ACNCFG_DMPISONLYCLIENT
#define DMPCLIENT_P_
#define DMPCLIENT_A_
#else
#define DMPCLIENT_P_ protocolID_t DMP_PROTOCOL_ID,
#define DMPCLIENT_A_ DMP_PROTOCOL_ID,
#endif

/*
Section: Connections

DMP is concerned with connections - these structures maintain the
transport protocol relevant information including state and context
data, details of remote and local components etc.
*/
struct dmptcxt_s {
	union {
		struct cxn_s *cxn;  /* who to send to */
		struct cxngp_s *cxngp;
	} dest;
	uint32_t lastaddr;
	uint32_t nxtaddr;
	uint8_t *pdup;
	uint8_t *endp;
	uint16_t wflags;
	struct txwrap_s *txwrap;
};

struct dmprcxt_s {
	struct cxn_s *cxn;  /* who received from */
	union addrmap_u *amap;
	uint32_t lastaddr;
	dmprx_fn *rxfn;
#if ACNCFG_DMP_DEVICE
	/* if a device most received commands are likely to need a response */
	struct dmptcxt_s rspcxt;
#endif  /* ACNCFG_DMP_DEVICE */
};

/**********************************************************************/
/*
Section: DMP Application layer functions

func: dmp_register
Start DMP and register a component for communication

Register a local component with DMP. If this is the first registration
this automatically starts DMP and 
all supporting layers as necessary.

This function also registers with lower layers so needs to be passed
any data that those layters need. Most of this is within the component
structure.
*/
int dmp_register(ifMC(struct Lcomponent_s *Lcomp,) netx_addr_t *listenaddr);

/**********************************************************************/
void dmp_inform(int event, void *object, void *info);

void dmp_closeblock(struct dmptcxt_s *tcxt);
void dmp_flushpdus(struct dmptcxt_s *tcxt);
void dmp_newblock(struct dmptcxt_s *tcxt);
uint8_t *dmp_openpdu(struct dmptcxt_s *tcxt, uint16_t vecnrange, 
					struct adspec_s *ads, int size);
void dmp_closepdu(struct dmptcxt_s *tcxt, uint8_t *nxtp);
void dmp_truncatepdu(struct dmptcxt_s *tcxt, uint32_t count, uint8_t *nxtp);

/**********************************************************************/
/*
receive functions
*/
void dmpsdtrx(struct cxn_s *cxn, const uint8_t *pdus, int blocksize, void *ref);


#endif /* __dmp_h__ */
