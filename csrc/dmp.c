/**********************************************************************/
/*

	Copyright (c) 2011, Philip Nye, Engineering Arts (UK) philip@engarts.com
	All rights reserved.

	Author: Philip Nye

	$Id$

#tabs=3t
*/
/**********************************************************************/

#include "acn.h"

/**********************************************************************/
/*
Prototypes
*/


/**********************************************************************/
#define lgFCTY LOG_DMP

/**********************************************************************/
/*
function: dmp_register

Register a local component for DMP access

Parameters:
	Lcomp - the component being registered

	listenaddr - listenaddr is both an input and an output 
	parameter; if the address is set as ANY (use macro 
	addrsetANY(addrp) ) or the port as netx_PORT_EPHEM, then the 
	actual port and address used will be filled in and can then be 
	advertised for discovery. If adhocaddr is NULL then the 
	transport layer (SDT, TCP etc.) will not accept unsolicited 
	joins to this component. Setting listenaddr to specific local 
	addresses rather than ANY may not be supported by lower layers.
*/

int
dmp_register(
	ifMC(struct Lcomponent_s *Lcomp)
)
{
	LOG_FSTART();
#if !ACNCFG_MULTI_COMPONENT
	struct Lcomponent_s *Lcomp = &localComponent;
#endif

	if ((Lcomp->dmp.flags & ACTIVE_LDMPFLG)) {  /* already in use */
		errno = EADDRNOTAVAIL;
		return -1;
	}
	Lcomp->dmp.flags |= ACTIVE_LDMPFLG;
	LOG_FEND();
	return 0;
}

/**********************************************************************/
#define DMPCX_UNICAST 1
#define DMPCX_ADD_TO_GROUP 2
#define DMPCX_FLAGERR(f) (((f) & (DMPCX_ADD_TO_GROUP | DMPCX_UNICAST)) == (DMPCX_ADD_TO_GROUP | DMPCX_UNICAST))
/**********************************************************************/

/**********************************************************************/
/*
group: Transmit functions
*/
/**********************************************************************/
void
dmp_newblock(struct dmptcxt_s *tcxt)
{
	int size;

	LOG_FSTART();
	assert(tcxt);
	if (tcxt->pdup) dmp_closeblock(tcxt);

#if ACNCFG_DMPON_SDT
	size = -1;

	tcxt->pdup = startProtoMsg(&tcxt->txwrap, tcxt->dest, DMP_PROTOCOL_ID, tcxt->wflags, &size);
	tcxt->endp = tcxt->pdup + size;
	tcxt->lastaddr = 0;
#endif
	LOG_FEND();
}

/**********************************************************************/
void
dmp_closeblock(struct dmptcxt_s *tcxt)
{
	LOG_FSTART();
#if ACNCFG_DMPON_SDT
	endProtoMsg(tcxt->txwrap, tcxt->pdup);
	tcxt->pdup = NULL;
#endif
	LOG_FEND();
}

/**********************************************************************/
void
dmp_flushpdus(struct dmptcxt_s *tcxt)
{
	LOG_FSTART();
#if ACNCFG_DMPON_SDT
	if (tcxt->pdup) dmp_closeblock(tcxt);
	flushWrapper(&tcxt->txwrap);
#endif
	LOG_FEND();
}

/**********************************************************************/
/*
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


/*
func: dmp_openpdu

Open a new PDU - starting a buffer if necessary. Save start address 
and inc but don't fill in until close time. 

arguments:
tcxt - Transmit context structure

vecnrange  - contains both the vector (high byte) and the range type 
(low byte). The range type sets only the type of range address (not 
size or relative fields) but gets ignored if count is 1

addr, inc - the property address and increment

maxcnt - indicates only the anticipated maximum value for count. 
Incremental processing after the PDU is opened might reduce that 
number.

sizep - size pointer. If *sizep is positive it represents the size 
of the PDU data.
*/

#define DMP_OFS_HEADER (OFS_VECTOR + DMP_VECTOR_LEN)
#define DMP_OFS_DATA (DMP_OFS_HEADER + DMP_HEADER_LEN)
#define DMP_SDT_MAXDATA (MAX_MTU - SDTW_AOFS_CB_PDU1DATA - DMP_OFS_DATA)

uint8_t *
dmp_openpdu(struct dmptcxt_s *tcxt, uint16_t vecnrange, struct adspec_s *ads, int size)
{
	uint8_t *dp;

	LOG_FSTART();
	assert(tcxt);

	if (tcxt->pdup == NULL) dmp_newblock(tcxt);

	tcxt->nxtaddr = ads->addr + ads->inc * (ads->count - 1);
	//tcxt->count = ads->count;

	if (ads->count == 1) {
		if (tcxt->pdup + size + 4 > tcxt->endp) {
			if (size + 4 > DMP_SDT_MAXDATA) {
				errno = EMSGSIZE;
				return NULL;
			}
			dmp_newblock(tcxt);
		}
		dp = tcxt->pdup + OFS_VECTOR;

		vecnrange &= ~DMPAD_TYPEMASK;   /* force single address */
		if (ads->addr - tcxt->lastaddr < ads->addr) {
			/* use relative address if it may give smaller value */
			ads->addr = ads->addr - tcxt->lastaddr;
			vecnrange |= DMPAD_R;
		}
		if (ads->addr < 0x100) {	
			dp = marshalU16(dp, vecnrange | DMPAD_1BYTE);
			dp = marshalU8(dp, ads->addr);
		} else if (ads->addr < 0x10000) {
			dp = marshalU16(dp, vecnrange | DMPAD_2BYTE);
			dp = marshalU16(dp, ads->addr);
		} else {
			dp = marshalU16(dp, vecnrange | DMPAD_4BYTE);
			dp = marshalU32(dp, ads->addr);
		}
	} else {
		uint32_t aai;

		if (tcxt->pdup + size + 12 > tcxt->endp) {
			if (size + 12 > DMP_SDT_MAXDATA) {
				errno = EMSGSIZE;
				return NULL;
			}
			dmp_newblock(tcxt);
		}
		dp = tcxt->pdup + OFS_VECTOR;
		if (ads->addr - tcxt->lastaddr < ads->addr) {
			/* use relative address if it may give smaller value */
			ads->addr = ads->addr - tcxt->lastaddr;
			vecnrange |= DMPAD_R;
		}
		aai = ads->addr | ads->inc | ads->count;

		if (aai < 0x100) {
			dp = marshalU16(dp, vecnrange | DMPAD_1BYTE);
			dp = marshalU8(dp, ads->addr);
			dp = marshalU8(dp, ads->inc);
			dp = marshalU8(dp, ads->count);
		} else if (aai < 0x10000) {
			dp = marshalU16(dp, vecnrange | DMPAD_2BYTE);
			dp = marshalU16(dp, ads->addr);
			dp = marshalU16(dp, ads->inc);
			dp = marshalU16(dp, ads->count);
		} else {
			dp = marshalU16(dp, vecnrange | DMPAD_4BYTE);
			dp = marshalU32(dp, ads->addr);
			dp = marshalU32(dp, ads->inc);
			dp = marshalU32(dp, ads->count);
		}
	}

	LOG_FEND();
	return dp;
}

/**********************************************************************/
/*
func: dmp_closepdu

Close a completed PDU

tcxt is the transmit context structure, nxtp is a pointer to the end 
of the data

Don't bother to check for vector and header repeats because they are
both only one byte and probably do not repeat consistently in DMP.
*/
void
dmp_closepdu(struct dmptcxt_s *tcxt, uint8_t *nxtp)
{
	LOG_FSTART();
	assert(tcxt && tcxt->pdup && nxtp <= tcxt->txwrap->txbuf + tcxt->txwrap->size);

	marshalU16(tcxt->pdup, ((nxtp - tcxt->pdup) + FIRST_FLAGS));
	tcxt->lastaddr = tcxt->nxtaddr;

	//acnlogmark(lgDBUG, "close PDU size %u", nxtp - tcxt->pdup);
	tcxt->pdup = nxtp;

	LOG_FEND();
}

/**********************************************************************/
/*
func: dmp_truncatepdu

Truncate the count field of a previously started PDU. Useful if 
progress through an array generating responses encounters an error. 
If nxtp is not NULL the PDU is closed.
*/

void
dmp_truncatepdu(struct dmptcxt_s *tcxt, uint32_t count, uint8_t *nxtp)
{
	LOG_FSTART();
	assert(tcxt 
			&& tcxt->pdup
			/* ensure we've a range */
			&& IS_RANGE(tcxt->pdup[DMP_OFS_HEADER])
			/* no overflow (always passes if nxtp is NULL) */
			&& nxtp <= tcxt->txwrap->txbuf + tcxt->txwrap->size
	);

	switch (tcxt->pdup[DMP_OFS_HEADER] & DMPAD_SIZEMASK) {
	case DMPAD_1BYTE:
		assert(count < 0x100);
		marshalU8(tcxt->pdup + DMP_OFS_DATA + 2, count);
		break;
	case DMPAD_2BYTE:
		assert(count < 0x10000);
		marshalU16(tcxt->pdup + DMP_OFS_DATA + 4, count);
		break;
	default:
		marshalU32(tcxt->pdup + DMP_OFS_DATA + 8, count);
		break;
	}
	if (nxtp) {
		marshalU16(tcxt->pdup, ((nxtp - tcxt->pdup) + FIRST_FLAGS));
		tcxt->lastaddr = tcxt->nxtaddr;
		tcxt->pdup = nxtp;
	}
	LOG_FEND();
}
/**********************************************************************/
/*
macros: vecflags bits

ctltodev - Vector is controller to device (otherwise device to controller)
propdata - Address is followed by property data
rcdata - Address is followed by result code data

Other vecflags are access bits which have the same values as pflg 
bits. All DMP vectors relate to one access mode out of pflg(read), 
pflg(write) or pflg(event) and for the vector to be applicable, the 
corresponding access bit must be set on the property. e.g. if 
vecflag pflg(event) is set, then the addressed property must support 
events.

haspdata(vector) - Macro to test whether vector expects property data.
hasrcdata(vector) - Macro to test whether vector expects result code data.
hasnodata(vector) - Macro to test whether vector has no data (address only).
canaccess(vector, dmpprop) - Macro to test whether the property access
bits allow this vector.
*/

#define ctltodev 0x8000
#define propdata 0x4000
#define rcdata 0x2000

#define accessmask (pflg(read) | pflg(write) | pflg(event))

#define haspdata(cmd) ((vecflags[cmd] & propdata) != 0)
#define hasrcdata(cmd) ((vecflags[cmd] & rcdata) != 0)
#define hasnodata(cmd) ((vecflags[cmd] & (rcdata | propdata)) == 0)
#define canaccess(cmd, prop) ((vecflags[cmd] & accessmask & getflags(prop)) != 0)

/*
var: vecflags

Array, one entry per vector, of flags specifying flags for each 
possible vector.

Note:
All valid vectors have non-zero flags. Intermediate /reserved/ 
vectors in the array can be detected as in-valid because their flags 
are all zero.
*/
static const unsigned int vecflags[DMP_MAX_VECTOR + 1] = {
#if ACNCFG_DMP_DEVICE
	[DMP_GET_PROPERTY]       = pflg(read) | ctltodev,
	[DMP_SET_PROPERTY]       = pflg(write) | propdata | ctltodev,
	[DMP_SUBSCRIBE]          = pflg(event) | ctltodev,
	[DMP_UNSUBSCRIBE]        = pflg(event) | ctltodev,
#endif
#if ACNCFG_DMP_CONTROLLER
	[DMP_GET_PROPERTY_REPLY] = pflg(read) | propdata,
	[DMP_EVENT]              = pflg(event) | propdata,
	[DMP_GET_PROPERTY_FAIL]  = pflg(read) | rcdata,
	[DMP_SET_PROPERTY_FAIL]  = pflg(write) | rcdata,
	[DMP_SUBSCRIBE_ACCEPT]   = pflg(event),
	[DMP_SUBSCRIBE_REJECT]   = pflg(event) | rcdata,
	[DMP_SYNC_EVENT]         = pflg(event) | propdata,
#endif
};

/*
var: failrsp

For vectors which may generate failure responses, these are the
corresponding response vectors.

note:
Only some ctltodev vectors generate responses. A controller never 
generates a failure response.
*/
#if ACNCFG_DMP_DEVICE
const uint16_t failrsp[] = {
	[DMP_GET_PROPERTY]       = (DMP_GET_PROPERTY_FAIL << 8) | DMPAD_RANGE_SINGLE,
	[DMP_SET_PROPERTY]       = (DMP_SET_PROPERTY_FAIL << 8) | DMPAD_RANGE_SINGLE,
	[DMP_SUBSCRIBE]          = (DMP_SUBSCRIBE_REJECT << 8) | DMPAD_RANGE_SINGLE,
	[DMP_UNSUBSCRIBE]        = 0,
#if ACNCFG_DMP_CONTROLLER
	[DMP_GET_PROPERTY_REPLY] = 0,
	[DMP_EVENT]              = 0,
	[DMP_GET_PROPERTY_FAIL]  = 0,
	[DMP_SET_PROPERTY_FAIL]  = 0,
	[DMP_SUBSCRIBE_ACCEPT]   = 0,
	[DMP_SUBSCRIBE_REJECT]   = 0,
	[DMP_SYNC_EVENT]         = 0,
#endif
};
#endif

const uint8_t badaccess[] = {
#if ACNCFG_DMP_DEVICE
	[DMP_GET_PROPERTY]       = DMPRC_NOREAD,
	[DMP_SET_PROPERTY]       = DMPRC_NOWRITE,
#if ACNCFG_DMP_NOEVENTS
	[DMP_SUBSCRIBE]          = DMPRC_NOSUBSCRIBE,
#else
	[DMP_SUBSCRIBE]          = DMPRC_NOEVENT,
#endif
	[DMP_UNSUBSCRIBE]        = 0,
#endif
#if ACNCFG_DMP_CONTROLLER
	[DMP_GET_PROPERTY_REPLY] = 0,
	[DMP_EVENT]              = 0,
	[DMP_GET_PROPERTY_FAIL]  = 0,
	[DMP_SET_PROPERTY_FAIL]  = 0,
	[DMP_SUBSCRIBE_ACCEPT]   = 0,
	[DMP_SUBSCRIBE_REJECT]   = 0,
	[DMP_SYNC_EVENT]         = 0,
#endif
};

/**********************************************************************/
#if ACNCFG_DMP_CONTROLLER
/*
func: rx_ctlvec

Handle a single controller cmd. Call this with a single command vector 
and a single address type (header) and a single address or 
address-range. Address and data (if vector takes data) are at datap.
*/

static const uint8_t *
rx_ctlvec(struct dmprcxt_s *rcxt, uint8_t vec, uint8_t header, const uint8_t *datap)
{
	uint8_t *txp;
	const struct dmpprop_s *dprop;
	union addrmap_u *amap;
	struct adspec_s ads;
	const uint8_t *dp;

	LOG_FSTART();

	dp = datap;

	/* determine requested address size in bytes */
	if (IS_RANGE(header)) {
		switch(header & DMPAD_SIZEMASK) {
		case DMPAD_1BYTE:
			ads.addr = unmarshalU8(dp); dp += 1;
			ads.inc = unmarshalU8(dp); dp += 1;
			ads.count = unmarshalU8(dp); dp += 1;
			break;
		case DMPAD_2BYTE:
			ads.addr = unmarshalU16(dp); dp += 2;
			ads.inc = unmarshalU16(dp); dp += 2;
			ads.count = unmarshalU16(dp); dp += 2;
			break;
		case DMPAD_4BYTE:
			ads.addr = unmarshalU32(dp); dp += 4;
			ads.inc = unmarshalU32(dp); dp += 4;
			ads.count = unmarshalU32(dp); dp += 4;
			break;
		default :
			acnlogmark(lgWARN,"Address length not valid..");
			return NULL;
		}
	} else {
		ads.inc = 0;
		ads.count = 1;
		switch(header & DMPAD_SIZEMASK) {
		case DMPAD_1BYTE:
			ads.addr = unmarshalU8(dp); dp += 1;
			break;
		case DMPAD_2BYTE:
			ads.addr = unmarshalU16(dp); dp += 2;
			break;
		case DMPAD_4BYTE:
			ads.addr = unmarshalU32(dp); dp += 4;
			break;
		default :
			acnlogmark(lgWARN,"Address length not valid..");
			return NULL;
		}
	}
	if (ads.count == 0) return dp;

	if (IS_RELADDR(header)) ads.addr += rcxt->lastaddr;
	rcxt->lastaddr = ads.addr + (ads.count - 1) * ads.inc;

	//acnlogmark(lgDBUG, "dmpcmd %02x, pdat.addr=%u, inc=%d, count=%u", vec, pdat.addr, ads.inc, ads.count);

	/*
	We go through the range - no responses allowed for controller vectors
	*/
	amap = rcxt->amap;

	while (ads.count > 0) {
		int32_t nprops;

		dprop = addr_to_prop(amap, addr);
		if (dprop == NULL) {
			/* dproperty not in map */
			acnlogmark(lgWARN, "Address %u does not match map", ads.addr);
			if (haspdata(vec)) return NULL;  /* lost sync */
			nprops = 1;
			if (hasrcdata(vec) && ads.count == 1 || IS_MULTIDATA(header)) ++dp;
		} else {
			/* call the appropriate function */
			if (haspdata(vec)) {
				nprops = (*(dmprxd_fn *)rcxt->rxfn)(rcxt, dprop, 
								&ads, dp, IS_MULTIDATA(header));
				if (nprops < 0) return NULL;
				if (IS_MULTIDATA(header)) {
					if (dprop->flags & pflg(vsize)) {
						int i;
						
						for (i = nprops; i--;) dp += unmarshalU16(dp);
					} else {
						dp += nprops * dprop->size;
					}
				} else if (nprops == ads.count) {
					if (dprop->flags & pflg(vsize))
						dp += unmarshalU16(dp);
					else
						dp += dprop->size;
				}
			} else if (hasrcdata(vec)) {
				nprops = (*(dmprxd_fn *)rcxt->rxfn)(rcxt, dprop, 
								&ads, dp, IS_MULTIDATA(header));
				if (nprops < 0) return NULL;
				if (IS_MULTIDATA(header)) dp += nprops;
				else if (nprops == ads.count) dp += 1;
			} else {
				nprops = (*rcxt->rxfn)(rcxt, dprop, &ads);
				if (nprops < 0) return NULL;
			}
		}
		ads.count -= nprops;
		ads.addr += nprops * ads.inc;
	}
	LOG_FEND();
	return dp;
}
#endif  /* ACNCFG_DMP_CONTROLLER */

/**********************************************************************/
#if ACNCFG_DMP_DEVICE
/*
func: rx_devvec

Handle a single device cmd. Call this with a single command vector 
and a single address type (header) and a single address or 
address-range. Address and data (if vector takes data) are at datap.
*/

static const uint8_t *
rx_devvec(struct dmprcxt_s *rcxt, uint8_t vec, uint8_t header, const uint8_t *datap)
{
	uint8_t *txp;
	const struct dmpprop_s *dprop;
	union addrmap_u *amap;
	struct dmptcxt_s *rspcxt = &rcxt->rspcxt;
	uint32_t addr, inc, count;
	const uint8_t *dp;

	LOG_FSTART();

	dp = datap;

	/* determine requested address size in bytes */
	if (IS_RANGE(header)) {
		switch(header & DMPAD_SIZEMASK) {
		case DMPAD_1BYTE:
			addr = unmarshalU8(dp); dp += 1;
			inc = unmarshalU8(dp); dp += 1;
			count = unmarshalU8(dp); dp += 1;
			break;
		case DMPAD_2BYTE:
			addr = unmarshalU16(dp); dp += 2;
			inc = unmarshalU16(dp); dp += 2;
			count = unmarshalU16(dp); dp += 2;
			break;
		case DMPAD_4BYTE:
			addr = unmarshalU32(dp); dp += 4;
			inc = unmarshalU32(dp); dp += 4;
			count = unmarshalU32(dp); dp += 4;
			break;
		default :
			acnlogmark(lgWARN,"Address length not valid..");
			return NULL;
		}
	} else {
		inc = 0;
		count = 1;
		switch(header & DMPAD_SIZEMASK) {
		case DMPAD_1BYTE:
			addr = unmarshalU8(dp); dp += 1;
			break;
		case DMPAD_2BYTE:
			addr = unmarshalU16(dp); dp += 2;
			break;
		case DMPAD_4BYTE:
			addr = unmarshalU32(dp); dp += 4;
			break;
		default :
			acnlogmark(lgWARN,"Address length not valid..");
			return NULL;
		}
	}
	if (count == 0) return dp;

	if (IS_RELADDR(header)) addr += rcxt->lastaddr;
	rcxt->lastaddr = addr + (count - 1) * inc;

	//acnlogmark(lgDBUG, "dmpcmd %02x, addr=%u, inc=%d, count=%u", vec, addr, inc, count);

	/*
	Go through the range - possibly building responses.
	*/
	amap = rcxt->amap;

	while (count > 0) {
		int32_t nprops;

		dprop = addr_to_prop(amap, addr);
		if (dprop == NULL) {
			/* dproperty not in map */
			acnlogmark(lgWARN, "Address %u does not match map", addr);
			if (failrsp[vec]) {
				struct adspec_s ads = {addr,1,1};

				txp = dmp_openpdu(rspcxt, failrsp[vec], &ads, 1);
				*txp++ = DMPRC_NOSUCHPROP;
				dmp_closepdu(rspcxt, txp);
			}
			if (vec == DMP_SET_PROPERTY) return NULL;  /* lost sync */
			nprops = 1;
		} else if (!canaccess(vec, dprop)) {
			acnlogmark(lgNTCE, "Access violation: address %u", addr);
			/* access error */
			if (failrsp[vec]) {
				/* FIXME: Should respond with range for all addresses 
				matching this property */
				struct adspec_s ads = {addr,1,1};

				txp = dmp_openpdu(rspcxt, failrsp[vec], &ads, 1);
				*txp++ = badaccess[vec];
				dmp_closepdu(rspcxt, txp);
			}
			if (vec == DMP_SET_PROPERTY && (IS_MULTIDATA(header) || count == 1)) {
				if (dprop->flags & pflg(vsize))
					dp += unmarshalU16(dp);
				else
					dp += dprop->size;
			}
			nprops = 1;
		} else {
			dmprx_fn * INITIALIZED(rxfn);
			struct adspec_s ads = {addr,inc,count};

			/* call the appropriate function */
#if ACNCFG_PROPEXT_FNS
			switch(vec) {
			case DMP_GET_PROPERTY:
				rxfn = dprop->fn_getprop;
				break;
			case DMP_SET_PROPERTY:
				rxfn = dprop->fn_setprop;
				break;
			case DMP_SUBSCRIBE:
				rxfn = dprop->fn_subscribe;
				break;
			case DMP_UNSUBSCRIBE:
				rxfn = dprop->fn_unsubscribe;
				break;				
			}
			if (rxfn == NULL)
#endif
			{
				rxfn = rcxt->rxfn;
			}
			if (vec == DMP_SET_PROPERTY) {
				nprops = (*(dmprxd_fn *)rxfn)(rcxt, dprop, &ads, dp, IS_MULTIDATA(header));
				if (nprops < 0) return NULL;
				if (IS_MULTIDATA(header)) {
					if (dprop->flags & pflg(vsize)) {
						int i;
						
						for (i = nprops; i--;) dp += unmarshalU16(dp);
					} else {
						dp += nprops * dprop->size;
					}
				} else if (nprops == count) {
					if (dprop->flags & pflg(vsize))
						dp += unmarshalU16(dp);
					else
						dp += dprop->size;
				}
			} else {
				nprops = (*rxfn)(rcxt, dprop, &ads);
				if (nprops < 0) return NULL;
			}
		}
		count -= nprops;
		addr += nprops * inc;
	}
	LOG_FEND();
	return dp;
}

#endif  /* ACNCFG_DMP_DEVICE */
/**********************************************************************/
#if ACNCFG_DMPON_SDT
void
dmp_sdtRx(struct member_s *memb, const uint8_t *pdus, int blocksize, void *ref)
{
	uint8_t INITIALIZED(header);
	const uint8_t *INITIALIZED(datap);
	int INITIALIZED(datasize);
	const uint8_t *pdup;
	const uint8_t *pp;
	const uint8_t *endp;
	uint8_t flags;
	uint8_t INITIALIZED(cmd);
	struct dmprcxt_s rcxt;

	LOG_FSTART();
	if (blocksize < DMP_BLOCK_MIN) {
		/* PDU is wrong length */
		acnlogmark(lgWARN, "Rx short PDU block (length %d)", blocksize);
		return;
	}
	/* first PDU must have all fields */
	if ((*pdus & (FLAG_bMASK)) != (FIRST_bFLAGS)) {
		acnlogmark(lgDBUG, "Rx bad first PDU flags");
		return;
	}

	memset(&rcxt, 0, sizeof(rcxt));
	rcxt.src = memb;
#if ACNCFG_DMP_DEVICE
	rcxt.rspcxt.dest = memb;
	rcxt.rspcxt.wflags = WRAP_REL_ON | WRAP_REPLY;
#endif

	for (pdup = pdus; pdup < pdus + blocksize - 2;)
	{
		flags = *pdup;
		pp = pdup + 2;
		pdup += getpdulen(pdup);   /* point to next PDU or end of block */

		if (flags & VECTOR_bFLAG) cmd = *pp++;
		if (flags & HEADER_bFLAG) header = *pp++;

		if (
				cmd > DMP_MAX_VECTOR 
			|| vecflags[cmd] == 0
			|| ((vecflags[cmd] & (rcdata | propdata)) == 0
					&& IS_MULTIDATA(header))
		) {
			acnlogmark(lgERR, "Bad DMP message %u or header %02x", cmd, header);
			continue;
		}

		if (flags & DATA_bFLAG) {
			datap = pp; /* get pointer to start of the data */
			datasize = pdup - pp;
		} else {
			pp = datap;
		}

		rcxt.rxfn = membLcomp(memb)->dmp.rxvec[cmd];
		assert(rcxt.rxfn != NULL);

#if ACNCFG_DMP_DEVICE && ACNCFG_DMP_CONTROLLER
		if (vecflags[cmd] & ctltodev) {
			rcxt.amap = membLcomp(memb)->dmp.amap;
			/*
			All commands have similar format based on header and only 
			differ in the number of data items for each address, so work 
			through addresses calling appropriate function.
			*/
			for (endp = pp + datasize; pp < endp; ) {
				pp = rx_devvec(&rcxt, cmd, header, pp);
				if (pp == NULL) break;	/* serious error */
			}
		} else {
			rcxt.amap = membRcomp(memb)->dmp.amap;
			for (endp = pp + datasize; pp < endp; ) {
				pp = rx_ctlvec(&rcxt, cmd, header, pp);
				if (pp == NULL) break;	/* serious error */
			}
		}
#elif ACNCFG_DMP_DEVICE
		rcxt.amap = membLcomp(memb)->dmp.amap;
		for (endp = pp + datasize; pp < endp; ) {
			pp = rx_devvec(&rcxt, cmd, header, pp);
			if (pp == NULL) break;	/* serious error */
		}
#else  /* must be ACNCFG_DMP_CONTROLLER */
		rcxt.amap = membRcomp(memb)->dmp.amap;
		for (endp = pp + datasize; pp < endp; ) {
			pp = rx_ctlvec(&rcxt, cmd, header, pp);
			if (pp == NULL) break;	/* serious error */
		}
#endif  /* ACNCFG_DMP_CONTROLLER */
	}
#if ACNCFG_DMP_DEVICE
	/* If processing has created PDUs to transmit then flush them */
	dmp_flushpdus(&rcxt.rspcxt);
#endif

	if (pdup != pdus + blocksize)  { /* sanity check */
		acnlogmark(lgNTCE, "Rx blocksize mismatch");
	}
	LOG_FEND();
}

#endif  /* ACNCFG_DMPON_SDT */
