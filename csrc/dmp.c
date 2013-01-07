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
/* pre-declare some structures so we can make pointers to them */
struct dmpcxt_s;

/**********************************************************************/
/*
Prototypes
*/

void dmp_inform(int event, void *object, void *info);

/**********************************************************************/
#define lgFCTY LOG_DMP

/**********************************************************************/
#if CONFIG_EPI10
#define A_SCOPE , pscope
#else
#define A_SCOPE
#endif

/*
Note adhocaddr is both an input and an output parameter - the 
address can be set as ANY (use macro addrsetANY(addrp)] or the port 
can be set as netx_PORT_EPHEM, and the actual port and address used 
will be filled in and can then be advertised for discovery.
*/

int
dmp_register(
	Lcomponent_t *Lcomp
#if CONFIG_SDT
	, uint8_t expiry
	, netx_addr_t *adhocaddr
#endif
#if CONFIG_EPI10
	, struct mcastscope_s *pscope
#endif
)
{
	if ((Lcomp->useflags & USEDBY_DMP)) {  /* already in use */
		errno = EADDRNOTAVAIL;
		return -1;
	}
	
#if CONFIG_SDT
	if (sdtRegister(Lcomp, expiry, &dmp_inform A_SCOPE) < 0) 
		return -1;
	
	if (adhocaddr && sdt_setListener(Lcomp, &autoJoin, adhocaddr) < 0)
		return -1;

	if (sdt_addClient(Lcomp, &dmpsdtrx, NULL) < 0)
		return -1;
#endif

	Lcomp->useflags |= USEDBY_DMP;
	return 0;
}

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
	
}

/**********************************************************************/
void
dmp_flushpdus(struct dmptxcxt_s *tcxt)
{
	txwrap_t *txwrap;

	LOG_FSTART();
	txwrap = tcxt->txwrap;
	if (txwrap == NULL) return;
	tcxt->txwrap = NULL;
#if CONFIG_SDT
	endProtoMsg(txwrap, tcxt->pdup);
	flushWrapper(txwrap, NULL);
#endif
	LOG_FEND();
}

/**********************************************************************/
/*
Open a new PDU - starting a buffer if necessary. Save start address 
and inc but don't fill in until close time. vecnrange contains both 
the vector (high byte) and the range type (low byte). The range type 
sets only the type of range address (not size or relative fields) 
but gets ignored if count is 1
maxcnt indicates only the anticipated maximum value for count - 
incremental processing after the PDU is opened might reduce that 
number.
*/

#define DMP_OFS_HEADER (OFS_VECTOR + DMP_VECTOR_LEN)
#define DMP_OFS_DATA (DMP_OFS_HEADER + DMP_HEADER_LEN)

#if CONFIG_SDT
#define WRAPTYPE_A_ WRAP_REL_ON | WRAP_REPLY,
#endif

uint8_t *
dmp_openpdu(struct dmptxcxt_s *tcxt, uint16_t vecnrange, uint32_t addr, 
				uint32_t inc, uint32_t maxcnt)
{
	int datastart;
	struct txwrap_s *txwrap;

	LOG_FSTART();
	if (tcxt->lastaddr && addr >= tcxt->lastaddr) {
		/* use relative address if it may give smaller value */
		addr -= tcxt->lastaddr;
		vecnrange |= DMPAD_R;
	}
	//acnlogmark(lgDBUG, "cmd=%u, range=%02x, addr=%u, inc=%d, count=%u", (vecnrange >> 8) & 0xff, vecnrange & 0xff, addr, inc, maxcnt);

	if (maxcnt == 1) {
		vecnrange &= ~DMPAD_TYPEMASK;   /* force single address */
		if (addr < 256) {
			vecnrange |= DMPAD_1BYTE;
			datastart = DMP_OFS_DATA + 1;
		} else if (addr < 65536) {
			vecnrange |= DMPAD_2BYTE;
			datastart = DMP_OFS_DATA + 2;
		} else {
			vecnrange |= DMPAD_4BYTE;
			datastart = DMP_OFS_DATA + 4;
		}
	} else {
		uint32_t aai = addr | inc | maxcnt;

		if (aai < 256) {
			vecnrange |= DMPAD_1BYTE;
			datastart = DMP_OFS_DATA + 3;
		} else if (aai < 65536) {
			vecnrange |= DMPAD_2BYTE;
			datastart = DMP_OFS_DATA + 6;
		} else {
			vecnrange |= DMPAD_4BYTE;
			datastart = DMP_OFS_DATA + 12;
		}
	}
	if ((txwrap = tcxt->txwrap) != NULL && 
				(tcxt->pdup + datastart + maxcnt) > txwrap->endp) {
		acnlogmark(lgDBUG, "flush full wrapper");
		dmp_flushpdus(tcxt);
	}
	if ((txwrap = tcxt->txwrap) == NULL) {
		tcxt->txwrap = initMemberMsg(tcxt->cxn, DMP_PROTOCOL_ID,
						NULL, WRAPTYPE_A_ &tcxt->pdup);
		tcxt->lastaddr = 0;
	}
	marshalU16(tcxt->pdup + OFS_VECTOR, vecnrange);
	tcxt->addr = addr;
	tcxt->inc = inc;
	LOG_FEND();
	return tcxt->pdup + datastart;
}

/**********************************************************************/
/*
Close a previously opened PDU.
count is the actual number of PDUs to be entered into the header (if 
0, the PDU is dumped) and nxtp is a pointer to the end of the current 
PDU.
*/

void
dmp_closepdu(struct dmptxcxt_s *tcxt, uint32_t count, uint8_t *nxtp)
{
	uint8_t tatype;
	uint8_t *tp;

	LOG_FSTART();
	assert(nxtp <= tcxt->txwrap->endp);
	if (count > 0) {
		marshalU16(tcxt->pdup, ((nxtp - tcxt->pdup) + FIRST_FLAGS));
		tatype = tcxt->pdup[DMP_OFS_HEADER];
		//acnlogmark(lgDBUG, "type=%02x, addr=%u, inc=%d, count=%u", tatype, tcxt->addr, tcxt->inc, count);

		tp = tcxt->pdup + DMP_OFS_DATA;
		switch (tatype & DMPAD_SIZEMASK) {
		case (DMPAD_1BYTE):
			tp = marshalU8(tp, tcxt->addr);
			if (IS_RANGE(tatype)) {
				tp = marshalU8(tp, tcxt->inc);
				tp = marshalU8(tp, count);
			}
			break;
		case (DMPAD_2BYTE):
			marshalU16(tp, tcxt->addr);
			if (IS_RANGE(tatype)) {
				tp = marshalU16(tp, tcxt->inc);
				tp = marshalU16(tp, count);
			}
			break;
		case (DMPAD_4BYTE):
			marshalU32(tp, tcxt->addr);
			if (IS_RANGE(tatype)) {
				tp = marshalU32(tp, tcxt->inc);
				tp = marshalU32(tp, count);
			}
			break;
		}
		if ((tatype & DMPAD_R))
			tcxt->lastaddr += tcxt->addr;
		else
			tcxt->lastaddr = tcxt->addr;

		tcxt->lastaddr += tcxt->inc * (count - 1);

		//acnlogmark(lgDBUG, "close PDU size %u", nxtp - tcxt->pdup);
		tcxt->pdup = nxtp;
	} else {
		acnlogmark(lgDBUG, "zero count"); 
	}

	LOG_FEND();
}

/**********************************************************************/
/*
rx_dmpcmd() handles a single cmd block.
*/

#define needrsp 0x8000
#define propdata 0x4000
#define rcdata 0x2000
#define localdev 0x1000

const unsigned int cmdflags[] = {
#if CONFIG_DMP_DEVICE
	[DMP_GET_PROPERTY]       = pflg_read | needrsp | localdev,
	[DMP_SET_PROPERTY]       = pflg_write | needrsp | propdata | localdev,
	[DMP_SUBSCRIBE]          = pflg_event | needrsp | localdev,
	[DMP_UNSUBSCRIBE]        = pflg_event | localdev,
#endif
#if CONFIG_DMP_CONTROLLER
	[DMP_GET_PROPERTY_REPLY] = pflg_read | propdata,
	[DMP_EVENT]              = pflg_event | propdata,
	[DMP_GET_PROPERTY_FAIL]  = pflg_read | rcdata,
	[DMP_SET_PROPERTY_FAIL]  = pflg_write | rcdata,
	[DMP_SUBSCRIBE_ACCEPT]   = pflg_event,
	[DMP_SUBSCRIBE_REJECT]   = pflg_event | rcdata,
	[DMP_SYNC_EVENT]         = pflg_event | propdata,
#endif
};

#define accessmask (pflg_read | pflg_write | pflg_event)

#define haspdata(cmd) ((cmdflags[cmd] & propdata) != 0)
#define hasrcdata(cmd) ((cmdflags[cmd] & rcdata) != 0)
#define canaccess(cmd, prop) ((cmdflags[cmd] & accessmask & getflags(prop)) != 0)

#if CONFIG_DMP_DEVICE
const uint16_t failrsp[] = {
	[DMP_GET_PROPERTY]       = (DMP_GET_PROPERTY_FAIL << 8) | DMPAD_RANGE_SINGLE,
	[DMP_SET_PROPERTY]       = (DMP_SET_PROPERTY_FAIL << 8) | DMPAD_RANGE_SINGLE,
	[DMP_SUBSCRIBE]          = (DMP_SUBSCRIBE_REJECT << 8) | DMPAD_RANGE_SINGLE,
	[DMP_UNSUBSCRIBE]        = 0,
#if CONFIG_DMP_CONTROLLER
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
#if CONFIG_DMP_DEVICE
	[DMP_GET_PROPERTY]       = DMPRC_NOREAD,
	[DMP_SET_PROPERTY]       = DMPRC_NOWRITE,
#if CONFIG_DMP_NOEVENTS
	[DMP_SUBSCRIBE]          = DMPRC_NOSUBSCRIBE,
#else
	[DMP_SUBSCRIBE]          = DMPRC_NOEVENT,
#endif
	[DMP_UNSUBSCRIBE]        = 0,
#endif
#if CONFIG_DMP_CONTROLLER
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
Call this with a single command vector and a single address range 
(possibly followed by data) at datap. The address type is given by 
header
*/

static const uint8_t *
rx_dmpcmd(struct dmprxcxt_s *rcxt, uint8_t cmd, uint8_t header, const uint8_t *datap)
{
	uint8_t *INITIALIZED(txp);
#if !CONFIG_DMPMAP_NONE
	const struct prop_s *prop;
	uint32_t minad, maxad;
	addrfind_t *map;
	int maplen;
#endif
	uint32_t addr;
	int32_t inc;
	int32_t count;
	const uint8_t *pp;
#if CONFIG_DMP_DEVICE
	struct dmptxcxt_s *rspcxt = rcxt->rspcxt;
#endif

	LOG_FSTART();

	inc = 0; count = 1;
	pp = datap;

	/* determine requested address size in bytes */
	switch(header & DMPAD_SIZEMASK) {
	case DMPAD_1BYTE:
		addr = unmarshalU8(pp); pp += 1;
		if (IS_RANGE(header)) {
			inc = unmarshalU8(pp); pp += 1;
			count = unmarshalU8(pp); pp += 1;
		}
		break;
	case DMPAD_2BYTE:
		addr = unmarshalU16(pp); pp += 2;
		if (IS_RANGE(header)) {
			inc = unmarshalU16(pp); pp += 2;
			count = unmarshalU16(pp); pp += 2;
		}
		break;
	case DMPAD_4BYTE:
		addr = unmarshalU32(pp); pp += 4;
		if (IS_RANGE(header)) {
			inc = unmarshalU32(pp); pp += 4;
			count = unmarshalU32(pp); pp += 4;
		}
		break;
	default :
		acnlogmark(lgWARN,"Address length not valid..");
		return NULL;
	}
	if (count == 0) return pp;

	if (IS_RELADDR(header)) addr += rcxt->lastaddr;
	rcxt->lastaddr = addr + (count - 1) * inc;

	//acnlogmark(lgDBUG, "dmpcmd %02x, addr=%u, inc=%d, count=%u", cmd, addr, inc, count);

#if CONFIG_DMPMAP_NONE
#define nprops count
#else /* !CONFIG_DMPMAP_NONE */

#if defined(CONFIG_DMPMAP_NAME)
	map = CONFIG_DMPMAP_NAME;
	maplen = ARRAYSIZE(CONFIG_DMPMAP_NAME);
#else
	map = rcxt->amap->map;
	maplen = rcxt->amap->h.count;
#endif

#if CONFIG_DMPMAP_INDEX
	minad = 0;
	maxad = maplen - 1;
#else  /* CONFIG_DMPMAP_SEARCH */
	minad = map->adlo;
	maxad = map[maplen - 1].adhi;
#endif
	/*
	Test whether the address range intersects the map at all.
	A rigorous test is not easy (since high values for inc can 
	produce patterns of addresses which repeatedly wrap round and 
	might intersect genuine properties in some places) so we do a 
	rough test and respond DMPRC_UNSPECIFIED if we don't like it.
	Tests: is count > our address span?
	       is abs(inc) > our address span?
	*/
	if (count >= (maxad - minad)
		|| (uint32_t)(inc + (maxad - minad))
			>= (uint32_t)(2 * (maxad - minad))
	) {
		/*
		can't deal with too many properties  or very large +/-increments
		*/
#if CONFIG_DMP_DEVICE
		if (failrsp[cmd]) {
			txp = dmp_openpdu(rspcxt, failrsp[cmd], addr, inc, count);
			*txp++ = DMPRC_UNSPECIFIED;
			dmp_closepdu(rspcxt, count, txp);
		}
		if (hasrcdata(cmd))	/* each reason code is 1 byte */
			pp += IS_MULTIDATA(header) ? count : 1;
		else
#endif
		{
			if (haspdata(cmd)) pp = NULL;
		}
		acnlogmark(lgWARN, "Address range too complex minad=%u, maxad=%u",
																		minad, maxad);
		return pp;
	}
	/*
	We go through the range - possibly building responses.
	*/
	while (count > 0) {
		unsigned int nprops;

		nprops = count;
		prop = findaddr(map, maplen, addr, inc, &nprops);
		if (prop == NULL) {
			/* property not in map */
			acnlogmark(lgWARN, "Address range [%u, %d, %u] does not match map", addr, count, inc);
#if !CONFIG_DMP_DEVICE
			if (haspdata(cmd)) return NULL;
#else
			if (failrsp[cmd]) {
				/*
				if the command has data we have lost sync because we 
				don't know its size, so reject the whole range
				*/
				if (haspdata(cmd)) nprops = count;
				txp = dmp_openpdu(rspcxt, failrsp[cmd], addr, inc, nprops);
				*txp++ = DMPRC_NOSUCHPROP;
				dmp_closepdu(rspcxt, nprops, txp);
			}
			if (haspdata(cmd)) return NULL;
			if (hasrcdata(cmd))
				pp += IS_MULTIDATA(header) ? nprops : 1;
		} else if (!canaccess(cmd, prop)) {
			acnlogmark(lgNTCE, "Access violation addr=%u", addr);
			/* access error */
			if (failrsp[cmd]) {
				txp = dmp_openpdu(rspcxt, failrsp[cmd], addr, inc, nprops);
				*txp++ = badaccess[cmd];
				dmp_closepdu(rspcxt, nprops, txp);
			}
			if (haspdata(cmd)) {
				if ((getflags(prop) & pflg_vsize) == 0)
					pp += nprops * getsize(prop);
				else {
					int i;
					for (i = 0; i < nprops; ++i)
						pp += unmarshalU16(pp);
				}
			}
#endif
		} else {
#endif  /* CONFIG_DMPMAP_NONE */
			/* call the appropriate function */
			switch (cmd) {
#if CONFIG_DMP_DEVICE
			case DMP_GET_PROPERTY:
				rx_getprop(rspcxt, PROP_A_ addr, inc, nprops);
				break;
			case DMP_SET_PROPERTY:
				pp = rx_setprop(rspcxt, PROP_A_ addr, inc, nprops, pp, IS_MULTIDATA(header));
				break;
			case DMP_SUBSCRIBE:
				rx_subscribe(rspcxt, PROP_A_ addr, inc, nprops);
				break;
			case DMP_UNSUBSCRIBE:
				rx_unsubscribe(PROP_A_ addr, inc, nprops);
				break;
#endif
#if CONFIG_DMP_CONTROLLER
			case DMP_GET_PROPERTY_REPLY:
				pp = rx_getpreply(PROP_A_ addr, inc, nprops, pp, IS_MULTIDATA(header));
				break;
			case DMP_EVENT:
				pp = rx_event(PROP_A_ addr, inc, nprops, pp, IS_MULTIDATA(header));
				break;
			case DMP_GET_PROPERTY_FAIL:
				pp = rx_getpfail(PROP_A_ addr, inc, nprops, pp, IS_MULTIDATA(header));
				break;
			case DMP_SET_PROPERTY_FAIL:
				pp = rx_setpfail(PROP_A_ addr, inc, nprops, pp, IS_MULTIDATA(header));
				break;
			case DMP_SUBSCRIBE_ACCEPT:
				rx_subsaccept(PROP_A_ addr, inc, nprops);
				break;
			case DMP_SUBSCRIBE_REJECT:
				pp = rx_subsreject(PROP_A_ addr, inc, nprops, pp, IS_MULTIDATA(header));
				break;
			case DMP_SYNC_EVENT:
				pp = rx_syncevent(PROP_A_ addr, inc, nprops, pp, IS_MULTIDATA(header));
				break;
#endif
			default:
				assert(false);
				break;
			}
#if !CONFIG_DMPMAP_NONE
		}
		if (pp == NULL) return pp;
		count -= nprops;
		addr += nprops * inc;
	}
#else /* CONFIG_DMPMAP_NONE */
#undef nprops
#endif
	LOG_FEND();
	return pp;
}

/************************************************************************/
#if CONFIG_SDT
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
	struct dmprxcxt_s rxcxt;
#if CONFIG_DMP_DEVICE
	struct dmptxcxt_s rspcxt;
#endif

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

	rxcxt.lastaddr = 0;
	/* rxcxt.cxn = cxn; */
#if CONFIG_DMP_DEVICE
	rxcxt.rspcxt = &rspcxt;
	rspcxt.txwrap = NULL;
	rspcxt.lastaddr = 0;
	rspcxt.cxn = cxn;
#endif

	for (pdup = pdus; pdup < pdus + blocksize - 2;)
	{
		flags = *pdup;
		pp = pdup + 2;
		pdup += getpdulen(pdup);   /* point to next PDU or end of block */

		if (flags & VECTOR_bFLAG) cmd = *pp++;
		if (flags & HEADER_bFLAG) header = *pp++;

		if (
				cmd >= ARRAYSIZE(cmdflags) 
			|| cmdflags[cmd] == 0
			|| ((cmdflags[cmd] & (rcdata | propdata)) == 0
					&& IS_MULTIDATA(header))
		) {
			acnlogmark(lgERR, "Bad DMP message %u or header %02x", cmd, header);
			continue;
		}
#if !CONFIG_DMPMAP_NONE && !defined(CONFIG_DMPMAP_NAME)
#if CONFIG_DMP_DEVICE && CONFIG_DMP_CONTROLLER
		if (cmdflags[cmd] & localdev) 
			rxcxt.amap = cxnLcomp(cxn)->dmp.map;
		else
			rxcxt.amap = cxnRcomp(cxn)->dmp.map;
#elif CONFIG_DMP_CONTROLLER
		rxcxt.amap = cxnRcomp(cxn)->dmp.map;
#else /* must be device */
		rxcxt.amap = cxnLcomp(cxn)->dmp.map;
#endif
#endif	/* !CONFIG_DMPMAP_NONE */

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
			pp = rx_dmpcmd(&rxcxt, cmd, header, pp);
			if (pp == NULL) break;	/* serious error */
		}
	}
#if CONFIG_DMP_DEVICE
	/* If processing has created PDUs to transmit then flush them */
	dmp_flushpdus(&rspcxt);
#endif

	if (pdup != pdus + blocksize)  { /* sanity check */
		acnlogmark(lgNTCE, "Rx blocksize mismatch");
	}
	LOG_FEND();
}

#endif  /* CONFIG_SDT */
