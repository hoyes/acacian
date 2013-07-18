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
	IS_SINGLEDATA(header) - True if there is just one value applying to
			all addresses in the range
	
*/
#ifndef __dmp_h__
#define __dmp_h__ 1

#define IS_RANGE(type) (((type) & DMPAD_TYPEMASK) != DMPAD_SINGLE)
#define IS_RELADDR(type) (((type) & DMPAD_R) != 0)
#define IS_MULTIDATA(type) (((type) & DMPAD_RANGE_ARRAY) != 0)
#define IS_SINGLEDATA(type) (((type) & DMPAD_RANGE_ARRAY) != 0)

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
*/
#if ACNCFG_SDT
#define cxn_s member_s
#define cxnLcomp membLcomp
#define cxnRcomp(cxn) ((cxn)->rem.Rcomp)
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
struct dmp_group_s;

typedef const uint8_t *dmprx_fn(struct dmptxcxt_s *cxtp,
						const struct prop_s *prop,
						uint32_t addr, int32_t inc, uint32_t nprops,
						const uint8_t * datap, bool dmany);

typedef void dmp_cxnev_fn(struct Rcomponent_s *Rcomp, 
						struct dmp_group_s *group, 
						unsigned int event);

/*
struct: dmp_Lcomp_s
Local component DMP layer structure
*/
struct dmp_Lcomp_s {
#if ACNCFG_DMP_DEVICE && !defined(ACNCFG_DMPMAP_NAME)
	/*pointer: map*/
	struct addrmap_s *map;
#endif
	dmprx_fn *rxvec[DMP_MAX_VECTOR + 1];
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
	struct addrmap_s *map;
#endif
	unsigned int ncxns;
	struct dmp_cxn_s *cxns[ACNCFG_DMP_RMAXCXNS];
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
	struct cxn_s **cxns;  /* who to send to */
	int ncxns;
	uint32_t lastaddr;
	uint32_t addr;
	uint32_t inc;
	uint32_t count;
	uint8_t *pdup;
	struct txwrap_s *txwrap;
};

#if ACNCFG_DMP_MULTITRANSPORT
enum dmp_cxn_e {
	cxn_unknown,
	cxn_sdt,
	cxn_tcp,
};
#endif  /* ACNCFG_DMP_MULTITRANSPORT */

struct dmp_cxn_s {
#if ACNCFG_DMP_MULTITRANSPORT
	enum dmp_cxn_e type;
#endif  /* ACNCFG_DMP_MULTITRANSPORT */
	struct {
		uint32_t lastaddr;
		uint32_t addr;
		uint32_t inc;
		uint32_t count;
		uint8_t *pdup;
		struct txwrap_s *txwrap;
	} tx;
	struct {
	#if !ACNCFG_DMPMAP_NONE && !defined(ACNCFG_DMPMAP_NAME)
		struct addrmap_s *amap;
	#endif
		uint32_t lastaddr;
		uint32_t addr;
		uint32_t inc;
		uint32_t count;
	#if ACNCFG_DMP_DEVICE
		struct dmptxcxt_s *rspcxt;
	#endif
	} rx;
#if 0 /* for now */
	union {
#if ACNCFG_DMPON_SDT
		struct dmpcxn_sdt_s sdt;
#endif
#if ACNCFG_DMPON_TCP
		struct dmpcxn_tcp_s tcp;
#endif
	} tp;
#endif
};

#if ACNCFG_DMP_MULTITRANSPORT
struct dmp_group_s {
	enum dmptransport_e type;
#if ACNCFG_DMPON_SDT
	struct Lchannel_s *sdt;
#endif
};
#elif ACNCFG_DMPON_SDT
#define dmp_group_s Lchannel_s
#endif

struct cxnGpParam_s {
	int flags;
#if ACNCFG_DMP_MULTITRANSPORT
	dmp_cxn_e type;
#endif
#if ACNCFG_DMPON_SDT
	
#endif
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
/*
group: Transmit functions

func: dmp_openpdu
Open a Protocol Data Unit

Creates a buffer if necessary, sets up the PDU header and returns
a pointer to the area to write data.

parameters:
	tcxt - The transmit context structure
	vecnrange - The DMP message type (high byte) and address type (low byte)
	addr - Starting address
	inc - Address increment
	maxcnt - Maximum address count for this message. The actual count may be
				reduced from this value when the PDU is
				closed, for example because
				an exception has occurred whilst accumulating values
*/
uint8_t *dmp_openpdu(struct dmptxcxt_s *tcxt, uint16_t vecnrange,
					uint32_t addr, uint32_t inc, uint32_t maxcnt);
/*
func: dmp_closepdu
Close a completed PDU

parameters:
	tcxt - The transmit context structure
	count - The number of addresses
	nxtp - Pointer to the end of the data
*/
void dmp_closepdu(struct dmptxcxt_s *tcxt, uint32_t count, uint8_t *nxtp);
void dmp_flushpdus(struct dmptxcxt_s *tcxt);

/**********************************************************************/
/*
Receive functions
*/
extern void dmpsdtrx(struct cxn_s *cxn, const uint8_t *pdus, int blocksize, void *ref);


/*
Receive functions
These must be provided by the application
*/
#if ACNCFG_DMP_DEVICE
void          rx_getprop(struct dmptxcxt_s *cxtp, PROP_P_ uint32_t addr, 
								int32_t inc, uint32_t nprops);

const uint8_t *rx_setprop(struct dmptxcxt_s *cxtp, PROP_P_ uint32_t addr, 
								int32_t inc, uint32_t nprops, const uint8_t * pp, bool dmany);

void        rx_subscribe(struct dmptxcxt_s *cxtp, PROP_P_ uint32_t addr, 
								int32_t inc, uint32_t nprops);

void      rx_unsubscribe(PROP_P_ uint32_t addr, 
								int32_t inc, uint32_t nprops);

#endif
#if ACNCFG_DMP_CONTROLLER
uint8_t  *rx_getpreply(PROP_P_ uint32_t addr, 
								int32_t inc, uint32_t nprops, const uint8_t * pp, bool dmany);

uint8_t      *rx_event(PROP_P_ uint32_t addr, 
								int32_t inc, uint32_t nprops, const uint8_t * pp, bool dmany);

uint8_t   *rx_getpfail(PROP_P_ uint32_t addr, 
								int32_t inc, uint32_t nprops, const uint8_t * pp, bool dmany);

uint8_t   *rx_setpfail(PROP_P_ uint32_t addr, 
								int32_t inc, uint32_t nprops, const uint8_t * pp, bool dmany);

void     rx_subsaccept(PROP_P_ uint32_t addr, 
								int32_t inc, uint32_t nprops);

uint8_t *rx_subsreject(PROP_P_ uint32_t addr, 
								int32_t inc, uint32_t nprops, const uint8_t * pp, bool dmany);

uint8_t  *rx_syncevent(PROP_P_ uint32_t addr, 
								int32_t inc, uint32_t nprops, const uint8_t * pp, bool dmany);
#endif

#endif /* __dmp_h__ */
