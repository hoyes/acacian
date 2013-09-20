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

void dmp_inform(int event, void *object, void *info);

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
	ifMC(struct Lcomponent_s *Lcomp,)
	netx_addr_t *listenaddr
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
#if ACNCFG_DMPON_SDT
	if (sdtRegister(ifMC(Lcomp,) &dmp_inform) < 0) 
		return -1;
	
	if (listenaddr && sdt_setListener(ifMC(Lcomp,) &autoJoin, listenaddr) < 0)
		goto fail1;

	if (sdt_addClient(ifMC(Lcomp,) &dmpsdtrx, NULL) < 0) {
		if (listenaddr) sdt_clrListener(ifMC(Lcomp));
fail1:
		sdtDeregister(ifMC(Lcomp));
		return -1;
	}
#endif

	Lcomp->dmp.flags |= ACTIVE_LDMPFLG;
	LOG_FEND();
	return 0;
}

/**********************************************************************/
#define DMPCX_UNICAST 1
#define DMPCX_ADD_TO_GROUP 2
#define DMPCX_FLAGERR(f) (((f) & (DMPCX_ADD_TO_GROUP | DMPCX_UNICAST)) == (DMPCX_ADD_TO_GROUP | DMPCX_UNICAST))
/**********************************************************************/
#if ACNCFG_DMPON_SDT
/*

*/
int
dmpConnectRq_sdt(
	ifMC(struct Lcomponent_s *Lcomp,)
	struct Rcomponent_s *Rcomp,   /* need only be initialized with UUID */
	netx_addr_t *connectaddr,
	unsigned int flags,
	void *a
)
{
	struct Lchannel_s *Lchan;

	LOG_FSTART();
	if (DMPCX_FLAGERR(flags)) return -1;
	if (flags & DMPCX_ADD_TO_GROUP) {
		Lchan = ((struct member_s *)a)->rem.Lchan;
	} else {
		uint16_t chflags;
		
		chflags = (flags & DMPCX_UNICAST) ? CHF_UNICAST : 0;
		Lchan = openChannel(ifMC(Lcomp,) ((struct chanParams_s *)a), chflags);
		
		if (Lchan == NULL) return -1;
	}
#if ACNCFG_DMP_MULTITRANSPORT
/* FIXME */
#error Not implemented yet
#else
	LOG_FEND();
	return addMember(Lchan, Rcomp, connectaddr);
#endif
}
#endif  /* ACNCFG_DMPON_SDT */

/**********************************************************************/

/*
int dmp_accept()
{
	return 0;
}

int dmp_connect(remote, callback)
{
	return 0;
}
*/

void
dmp_inform(int event, void *object, void *info)
{
	struct Lcomponent_s *Lcomp;
	struct cxn_s *cxn;

	LOG_FSTART();
	switch (event) {
	EV_RCONNECT:  /* object = Lchan, info = memb */
	EV_LCONNECT:  /* object = Lchan, info = memb */
	EV_REMDISCONNECT:  /* object = Lchan, info = memb */
	EV_LOCDISCONNECT:  /* object = Lchan, info = memb */
		Lcomp = LchanOwner((struct Lchannel_s *)object);
#if ACNCFG_DMPON_SDT && !ACNCFG_DMP_MULTITRANSPORT
		cxn = (struct cxn_s *)info;
#endif

		Lcomp->dmp.cxnev(cxn, (event == EV_RCONNECT || event == EV_LCONNECT));
		break;

	EV_DISCOVER:  /* object = Rcomp, info = discover data in packet */
		break;
	EV_JOINSUCCESS:  /* object = Lchan, info = memb */
		break;

	EV_JOINFAIL:  /* object = Lchan, info = memb->rem.Rcomp */
		break;
	EV_LOCCLOSE:  /* object = , info =  */
		break;
	EV_LOCLEAVE:  /* object = Lchan, info = memb */
		break;
	EV_LOSTSEQ:  /* object = Lchan, info = memb */
		break;
	EV_MAKTIMEOUT:  /* object = Lchan, info = memb */
		break;
	EV_NAKTIMEOUT:  /* object = Lchan, info = memb */
		break;
	EV_REMLEAVE:  /* object = , info =  */
		break;
	default:
		break;
	}
	LOG_FEND();
}

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

	tcxt->pdup = startProtoMsg(&tcxt->txwrap, tcxt->cxn, DMP_PROTOCOL_ID, tcxt->wflags, &size);
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
dmp_openpdu(struct dmptcxt_s *tcxt, uint16_t vecnrange, uint32_t addr, 
				uint32_t inc, uint32_t count, int size)
{
	int datastart;
	struct txwrap_s *txwrap;
	uint8_t *dp;

	LOG_FSTART();
	assert(tcxt);

	if (tcxt->pdup == NULL) dmp_newblock(tcxt);

	tcxt->nxtaddr = addr + inc * (count - 1);
	//tcxt->count = count;

	if (count == 1) {
		if (tcxt->pdup + size + 4 > tcxt->endp) {
			if (size + 4 > DMP_SDT_MAXDATA) {
				errno = EMSGSIZE;
				return NULL;
			}
			dmp_newblock(tcxt);
		}
		dp = tcxt->pdup + OFS_VECTOR;

		vecnrange &= ~DMPAD_TYPEMASK;   /* force single address */
		if (addr - tcxt->lastaddr < addr) {
			/* use relative address if it may give smaller value */
			addr = addr - tcxt->lastaddr;
			vecnrange |= DMPAD_R;
		}
		if (addr < 0x100) {	
			dp = marshalU16(dp, vecnrange | DMPAD_1BYTE);
			dp = marshalU8(dp, addr);
		} else if (addr < 0x10000) {
			dp = marshalU16(dp, vecnrange | DMPAD_2BYTE);
			dp = marshalU16(dp, addr);
		} else {
			dp = marshalU16(dp, vecnrange | DMPAD_4BYTE);
			dp = marshalU32(dp, addr);
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
		if (addr - tcxt->lastaddr < addr) {
			/* use relative address if it may give smaller value */
			addr = addr - tcxt->lastaddr;
			vecnrange |= DMPAD_R;
		}
		aai = addr | inc | count;

		if (aai < 0x100) {
			dp = marshalU16(dp, vecnrange | DMPAD_1BYTE);
			dp = marshalU8(dp, addr);
			dp = marshalU8(dp, inc);
			dp = marshalU8(dp, count);
		} else if (aai < 0x10000) {
			dp = marshalU16(dp, vecnrange | DMPAD_2BYTE);
			dp = marshalU16(dp, addr);
			dp = marshalU16(dp, inc);
			dp = marshalU16(dp, count);
		} else {
			dp = marshalU16(dp, vecnrange | DMPAD_4BYTE);
			dp = marshalU32(dp, addr);
			dp = marshalU32(dp, inc);
			dp = marshalU32(dp, count);
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
*/

#define needrsp 0x8000
#define propdata 0x4000
#define rcdata 0x2000
#define localdev 0x1000

const unsigned int cmdflags[DMP_MAX_VECTOR + 1] = {
#if ACNCFG_DMP_DEVICE
	[DMP_GET_PROPERTY]       = pflg(read) | needrsp | localdev,
	[DMP_SET_PROPERTY]       = pflg(write) | needrsp | propdata | localdev,
	[DMP_SUBSCRIBE]          = pflg(event) | needrsp | localdev,
	[DMP_UNSUBSCRIBE]        = pflg(event) | localdev,
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

#define accessmask (pflg(read) | pflg(write) | pflg(event))

#define haspdata(cmd) ((cmdflags[cmd] & propdata) != 0)
#define hasrcdata(cmd) ((cmdflags[cmd] & rcdata) != 0)
#define canaccess(cmd, prop) ((cmdflags[cmd] & accessmask & getflags(prop)) != 0)

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
/*
func: rx_dmpcmd

Handle a single cmd block. Call this with a single command vector 
and a single address range (possibly followed by data) at datap. The 
address type is given by header
*/

static const uint8_t *
rx_dmpcmd(struct dmprcxt_s *rcxt, uint8_t cmd, uint8_t header, const uint8_t *datap)
{
	uint8_t *INITIALIZED(txp);
	const struct dmpprop_s *dprop;
	union addrmap_u *amap;
#if ACNCFG_DMP_DEVICE
	struct dmptcxt_s *rspcxt = &rcxt->rspcxt;
	int size = -1;
#endif
	struct dmppdata_s pdat;
	uint32_t icount;

	LOG_FSTART();

	pdat.inc = 0;
	icount = pdat.count = 1;
	pdat.data = datap;

	/* determine requested address size in bytes */
	switch(header & DMPAD_SIZEMASK) {
	case DMPAD_1BYTE:
		pdat.addr = unmarshalU8(pdat.data); pdat.data += 1;
		if (IS_RANGE(header)) {
			pdat.inc = unmarshalU8(pdat.data); pdat.data += 1;
			icount = pdat.count = unmarshalU8(pdat.data); pdat.data += 1;
		}
		break;
	case DMPAD_2BYTE:
		pdat.addr = unmarshalU16(pdat.data); pdat.data += 2;
		if (IS_RANGE(header)) {
			pdat.inc = unmarshalU16(pdat.data); pdat.data += 2;
			icount = pdat.count = unmarshalU16(pdat.data); pdat.data += 2;
		}
		break;
	case DMPAD_4BYTE:
		pdat.addr = unmarshalU32(pdat.data); pdat.data += 4;
		if (IS_RANGE(header)) {
			pdat.inc = unmarshalU32(pdat.data); pdat.data += 4;
			icount = pdat.count = unmarshalU32(pdat.data); pdat.data += 4;
		}
		break;
	default :
		acnlogmark(lgWARN,"Address length not valid..");
		return NULL;
	}
	if (pdat.count == 0) return pdat.data;

	if (IS_RELADDR(header)) pdat.addr += rcxt->lastaddr;
	rcxt->lastaddr = pdat.addr + (pdat.count - 1) * pdat.inc;

	//acnlogmark(lgDBUG, "dmpcmd %02x, pdat.addr=%u, inc=%d, count=%u", cmd, pdat.addr, inc, count);

	/*
	We go through the range - possibly building responses.
	*/
	amap = rcxt->amap;

	while (pdat.count > 0) {
		dprop = addr_to_prop(amap, pdat.addr);
		if (dprop == NULL) {
			
			/* dproperty not in map */
			acnlogmark(lgWARN, "Address %u does not match map", pdat.addr);
			if (haspdata(cmd)) return NULL;
#if ACNCFG_DMP_DEVICE
			/*
			Device->controller messages never generate a response so
			none of this is relevant unless we have a device
			*/
			if (failrsp[cmd]) {
				txp = dmp_openpdu(rspcxt, failrsp[cmd], pdat.addr, 1, 1, 1);
				*txp++ = DMPRC_NOSUCHPROP;
				dmp_closepdu(rspcxt, txp);
			}
			if (hasrcdata(cmd) && IS_MULTIDATA(header)) {
				pdat.data += 1;
			}
			--pdat.count;
		} else if (!canaccess(cmd, dprop)) {
			acnlogmark(lgNTCE, "Access violation: address %u", pdat.addr);
			/* access error */
			if (failrsp[cmd]) {
				int size;

				size = 1;
				txp = dmp_openpdu(rspcxt, failrsp[cmd], pdat.addr, 1, 1, 1);
				*txp++ = badaccess[cmd];
				dmp_closepdu(rspcxt, txp);
			}
			if (haspdata(cmd)) {
				/* need to skip the data */
				if ((getflags(dprop) & pflg(vsize)))
					pdat.data += unmarshalU16(pdat.data);
				else {
					pdat.data += getsize(dprop);
				}
			}
			if (--pdat.count == 0 && IS_SINGLEDATA(header) && hasrcdata(cmd)) {
				pdat.data += 1;
			}
#else
			--pdat.count;
#endif  /* ACNCFG_DMP_DEVICE */
		} else {
			/* call the appropriate function */
			dmprx_fn *rxfn;

			rxfn = cxnLcomp(rcxt->cxn)->dmp.rxvec[cmd];
			assert(rxfn != NULL);
			(*rxfn)(ifDMP_D(rspcxt,) dprop, &pdat, IS_MULTIDATA(header));
		}
	}
	LOG_FEND();
	return pdat.data;
}

/************************************************************************/
#if ACNCFG_DMPON_SDT
void
dmpsdtrx(struct cxn_s *cxn, const uint8_t *pdus, int blocksize, void *ref)
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
	rcxt.cxn = cxn;
#if ACNCFG_DMP_DEVICE
	rcxt.rspcxt.cxn = cxn;
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
			|| cmdflags[cmd] == 0
			|| ((cmdflags[cmd] & (rcdata | propdata)) == 0
					&& IS_MULTIDATA(header))
		) {
			acnlogmark(lgERR, "Bad DMP message %u or header %02x", cmd, header);
			continue;
		}
#if ACNCFG_DMP_DEVICE && ACNCFG_DMP_CONTROLLER
		if (cmdflags[cmd] & localdev) 
			rcxt.amap = cxnLcomp(cxn)->dmp.amap;
		else
			rcxt.amap = cxnRcomp(cxn)->dmp.amap;
#elif ACNCFG_DMP_CONTROLLER
		rcxt.amap = cxnRcomp(cxn)->dmp.amap;
#else /* must be device */
		rcxt.amap = cxnLcomp(cxn)->dmp.amap;
#endif

		if (flags & DATA_bFLAG) {
			datap = pp; /* get pointer to start of the data */
			if ((datasize = pdup - pp) < 0) { /* get size of the data */
				acnlogmark(lgWARN, "Rx blocksize error");
				return;
			}
		}
		
		/*
		All commands have similar format based on header and only 
		differ in the number of data items for each address, so work 
		through addresses calling appropriate function.
		*/
		for (pp = datap, endp = datap + datasize; pp < endp; ) {
			pp = rx_dmpcmd(&rcxt, cmd, header, pp);
			if (pp == NULL) break;	/* serious error */
		}
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
