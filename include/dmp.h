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
#ifndef __dmp_h__
#define __dmp_h__ 1
/**********************************************************************/
/*
header: dmp.h

Device Management Protocol

topic: Overview

Acacian's DMP code provides these services:
- An incoming message block parser which decodes messages, and identifies
the properties they relate to before passing them on to the application.
See <dmp_sdtRx>.
- Automatic response context (for any received message which may 
generate a response) to facilitate building and transmission of 
responses via the correct connection.
- Detection and response to all basic property address or access 
errors. e.g. addressing a non-existent property or attempt to 
subscribe to a property which does not support events.
- A suite of functions for building and transmitting message blocks.

topic: DMP Message format

All DMP messages relate to DMP addresses within a component and so all 
specify one or more addresses in a standard address format.

Certain messages must also specify data relating to each address  
which is either a value for the property at that address (e.g. 
set-property, get-property-reply), or a 
reason code specifying why a command for that address has failed. 
The presence of data and whether it is a value or a reason code is 
fixed for each specific message code.

In the standard PDU format of ACN the DMP message fields are defined as:

vector - One octet message code.
header - One octet address encoding, determines single vs range address,
			field size, single vs multiple data, absolute or relative (to 
			previously used) address.
data -	One or more fields of address only, address + data, or 
			address + reason-code as determined by the message code. 
			The number of fields is determined by the PDU length.

topic: Message support

DMP messages divide into two clear sets. Support for these is compiled
according to whether <CF_DMPCOMP_C_> (controller only), <CF_DMPCOMP__D>
(device only) or <CF_DMPCOMP_CD> (both) is configured.

Controller→Device messages:
Devices must correctly receive and handle these and it is an error for a 
controller-only component to receive any of them. The
address field in these messages refers to the receiving component.
	o DMP_GET_PROPERTY
	o DMP_SET_PROPERTY
	o DMP_SUBSCRIBE
	o DMP_UNSUBSCRIBE

Device→Controller messages:
Controllers must receive and handle these and it is an error for a 
device-only component to receive any of them. The
address field refers to the transmitting component.
	o DMP_GET_PROPERTY_REPLY
	o DMP_GET_PROPERTY_FAIL
	o DMP_SET_PROPERTY_FAIL
	o DMP_SUBSCRIBE_ACCEPT
	o DMP_SUBSCRIBE_REJECT
	o DMP_EVENT
	o DMP_SYNC_EVENT

topic: Array Properties

For any DMP property declared (in DDL) as an array, the DDL parser 
generates a single <dmpprop_s> structure which includes the 
necessary dimensional definitions. Callbacks to handle array addressed 
messages relating to these properties are passed the array address 
and are able to handle the whole array operation in one go. The 
dimensional information in the <dmpprop_s> structure may also be 
used to efficiently generate array addressed outgoing messages.

group: Macros for address types

macros:
	IS_RANGE(header) - True if the header value specifies a range address
	IS_RELADDR(header) - True if the header specifies a relative address
	IS_MULTIDATA(header) - True if there is a data value for each address
			in the range
	IS_RANGECOMMON(header) - True if there is just one value applying to
			all addresses in the range
	
*/

#define IS_RANGE(type) (((type) & DMPAD_TYPEMASK) != DMPAD_SINGLE)
#define IS_RELADDR(type) (((type) & DMPAD_R) != 0)
#define IS_MULTIDATA(type) (((type) & DMPAD_RANGE_ARRAY) != 0)
#define IS_RANGECOMMON(type) (((type) & DMPAD_TYPEMASK) == DMPAD_RANGE_SINGLE)

/*
Group: Combined message code and address modes

For many purposes
Acacian combines the message code with the address type (header) field.
These macros define all the combined message/address codes required
by Acacian's transmit code - there are other permitted values but they 
mostly have arcane use-cases or duplicate functionality.

*Note:* the choice of relative/absolute addressing is made automatically
as PDUs are constructed.

*Note:* <dmp_openpdu()> will substitute the single address version 
whenever count = 1 so multiple address versions can normally be used

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
Packet structure lengths
*/
#define DMP_BLOCK_MIN (OFS_VECTOR + DMP_VECTOR_LEN + DMP_HEADER_LEN)

/**********************************************************************/
/*
we need some partial structure pre-declarations
*/
struct Lcomponent_s;
#if CF_EPI10
struct mcastscope_s;
#endif
struct txwrap_s;
struct dmptcxt_s;

struct adspec_s {
	uint32_t addr;
	uint32_t inc;
	uint32_t count;
};

#if !CF_PROPEXT_FNS
struct dmprcxt_s;

/*
type: dmprx_fn

Callback function to handle a specific incoming message. A local 
component must include a vector of these functions to handle all 
message types it implements. See <dmp_Lcomp_s>.

|typedef int dmprx_fn(struct dmprcxt_s *rcxt, const uint8_t *data);

rcxt - receive context, includes the propery that the message 
relates to and the address specification which may include multiple 
values if the property is an array (see <Array Properties>). Also a 
pre-initialized transmit context structure for building responses 
(only if component is a device).
data - pointer to the data in the packet.

return value:
Each function must return the number of values it has 
processed up to a maximum of the value count given in the array 
address specifcation. If the callback returns less this number it 
will be called again until all values are used up. This gives the 
freedom to process array addresses individually, as a whole array or 
in blocks.

A return value of -1 indicates that a serious error has occurred and 
synchronization with the data stream is lost. All subsequent 
messages or values in the block will be skipped. However, if a 
property specific error occurs, the application should construct an 
appropriate error response and include the value as processed.
*/
typedef int dmprx_fn(struct dmprcxt_s *rcxt, const uint8_t *data);
#endif

/*
type: dmp_Lcomp_s

DMP related data for local or remote components (see <component.h>). 
Includes:

amap - Required if local component is a device (<CF_DMPCOMP__D> or 
<CF_DMPCOMP_CD>), otherwise omitted.
rxvec - A vector of <dmprx_fn>s. One for each message code. e.g.
(code)
	.rxvec = {
		[DMP_reserved0]             = NULL,
		[DMP_GET_PROPERTY]          = &dev_get_prop,
		[DMP_SET_PROPERTY]          = &dev_set_prop,
		[DMP_GET_PROPERTY_REPLY]    = &ctl_recieve_prop,
		[DMP_EVENT]                 = &ctl_recieve_prop,
		[DMP_reserved5]             = NULL,
		[DMP_reserved6]             = NULL,
		[DMP_SUBSCRIBE]             = &dev_subscribe_prop,
		[DMP_UNSUBSCRIBE]           = &dev_unsubscribe_prop,
		[DMP_GET_PROPERTY_FAIL]     = &ctl_getprop_fail,
		[DMP_SET_PROPERTY_FAIL]     = &ctl_setprop_fail,
		[DMP_reserved11]            = NULL,
		[DMP_SUBSCRIBE_ACCEPT]      = &ctl_subs_accept,
		[DMP_SUBSCRIBE_REJECT]      = &ctl_subscribe_fail,
		[DMP_reserved14]            = NULL,
		[DMP_reserved15]            = NULL,
		[DMP_reserved16]            = NULL,
		[DMP_SYNC_EVENT]            = &ctl_recieve_prop,
	},
(end code)
*/
struct dmp_Lcomp_s {
#if CF_DMPCOMP_xD && !defined(CF_DMPMAP_NAME)
	union addrmap_u *amap;
#endif
	dmprx_fn *rxvec[DMP_MAX_VECTOR];
	uint8_t flags;
};

/*
type: dmp_Rcomp_s

DMP related data for local or remote components (see <component.h>). 
Includes:

union addrmap_u *amap - Required if local component is a controller 
(<CF_DMPCOMP_C_> or <CF_DMPCOMP_CD>), otherwise omitted.
unsigned int ncxns - Number of connections we have to this component.
void *cxns[] - Array of connection identifiers (depends on DMP's 
transport, see <CF_DMP_MULTITRANSPORT>).
*/
struct dmp_Rcomp_s {
#if CF_DMPCOMP_Cx
	union addrmap_u *amap;
#endif
	unsigned int ncxns;
	void *cxns[CF_DMP_RMAXCXNS];
};

enum dmpLcompflg_e {
	ACTIVE_LDMPFLG = 1,
};

/**********************************************************************/
#if CF_DMPISONLYCLIENT
#define DMPCLIENT_P_
#define DMPCLIENT_A_
#else
#define DMPCLIENT_P_ protocolID_t DMP_PROTOCOL_ID,
#define DMPCLIENT_A_ DMP_PROTOCOL_ID,
#endif

/*
types: Connection context

DMP is concerned with connections – these structures maintain the
transport protocol relevant information including state and context
data, details of remote and local components etc.

On transmission it is possible to be accumulating transmit data in 
multiple transmit contexts at the same time. For example while 
handling a multi-address set-property message or series of messages 
a device may accumulate set-property-fail responses for some 
properties in the context of the controller's session whilst 
generating event messages in it's own event session for properties 
whose value has been successfully changed.
*/
struct dmptcxt_s {
	void *dest;  /* who to send to */
	uint32_t lastaddr;
	uint32_t nxtaddr;
	uint8_t *pdup;
	uint8_t *endp;
	uint16_t wflags;
	struct txwrap_s *txwrap;
};

struct dmprcxt_s {
	uint8_t vec;
	uint8_t hdr;
	const struct dmpprop_s *dprop;
	struct adspec_s ads;
	void *src;  /* who received from (a member_s if CF_DMPON_SDT) */
	union addrmap_u *amap;
	uint32_t lastaddr;
	dmprx_fn *rxfn;
#if CF_DMPCOMP_xD
	/* if a device most received commands are likely to need a response */
	struct dmptcxt_s rspcxt;
#endif  /* CF_DMPCOMP_xD */
};

/**********************************************************************/
/*
group: Component registration

function: dmp_register

Register a local component for DMP access.

Parameters:
	Lcomp - the component being registered.
*/
int dmp_register(ifMC(struct Lcomponent_s *Lcomp));

/**********************************************************************/
/*
group: DMP Transmit Functions

Within an SDT wrapper it is possible to send multiple DMP blocks. The
usual reaason for this is to combine messages to different session 
members, or to all members, into a single wrapper. Each block can 
therefore have a different destination provided each is within the 
same channel. The destination is embedded in the transmit context 
structure.

Within a block there may be multiple DMP PDUs. Each PDU contains a 
message code and address format, then multiple address, or address + 
data fields.

func: dmp_newblock

Allocate and open a new DMP PDU block for the given transmit context.
If a block is already open it is first closed.
*/
int dmp_newblock(struct dmptcxt_s *tcxt, int *size);
/*
func: dmp_closeblock

Close the accumulated PDU block for the transmit context.
Any opened PDU will be closed first.
*/
void dmp_closeblock(struct dmptcxt_s *tcxt);
/*
func: dmp_flushpdus

Flush (transmit) the transmit context. Any open block is closed first 
then transmitted.
*/
void dmp_flushpdus(struct dmptcxt_s *tcxt);
/*
func: dmp_abortblock

Reset the pointers for the block and forget any accumulated data.
*/
void dmp_abortblock(struct dmptcxt_s *tcxt);

/*
func: dmp_openpdu

Open a new PDU with the message type and address format given by 
vecnrange. The ads structure indicates the number of addresses etc. 
and size, the space needed.

A new block is opened if necessary.
*/
uint8_t *dmp_openpdu(struct dmptcxt_s *tcxt, uint16_t vecnrange, 
					struct adspec_s *ads, int size);

/*
func: dmp_closepdu

Close and finalize the previously opened PDU
*/
void dmp_closepdu(struct dmptcxt_s *tcxt, uint8_t *nxtp);

/*
func: dmp_closeflush

Close the current PDU then close and flush all accumulated blocks.
*/
void dmp_closeflush(struct dmptcxt_s *tcxt, uint8_t *nxtp);

/*
func: dmp_truncatepdu

Truncate and close the current PDU at a smaller size/count than 
specified when it was opened. This is useful when errors are 
encountered whilst accumulating responses for example.

count specifies the number of items successfully accumulated and
nxtp the end of the accumulated data.
*/
void dmp_truncatepdu(struct dmptcxt_s *tcxt, uint32_t count, uint8_t *nxtp);

/**********************************************************************/
/*
group: Receive functions

Receive is handled by callback functions. Application level 
callbacks are specified either in the rxvec field of <dmp_Lcomp_s>, 
or for devices may be specified on a per-property basis using DDL 
property extensions.

func: dmp_sdtRx

This is a callback function which must be passed to SDT 
[rxfn in <sdt_addClient>] to have Acacian's DMP code parse and handle 
incoming DMP. DMP is split into individual messages and addresses 
matched to properties and decoded before calling the application 
provided message handlers [rxvec in <dmp_Lcomp_s>].
*/
void dmp_sdtRx(struct member_s *memb, const uint8_t *pdus, int blocksize, void *ref);


#endif /* __dmp_h__ */
