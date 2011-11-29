/* vi: set sw=3 ts=3: */
/**********************************************************************/
/*
`
	Copyright (c) 2011, Philip Nye, Engineering Arts (UK) philip@engarts.com
	All rights reserved.

	Author: Philip Nye

	$Id$

*/
/**********************************************************************/

#include "acncommon.h"
#include "acnlog.h"
#include "marshal.h"
#include "netxface.h"
#include "sdt.h"
#include "dmp.h"
#include "propmap.h"

#include "warpxlib.h"

/**********************************************************************/
/* pre-declare some structures so we can make pointers to them */
struct dmpcxt_s;

/**********************************************************************/
/*
Prototypes
*/

extern void dmprx(struct member_s *memb, const uint8_t *data, int length, void *cookie);

#if CONFIG_DMP_DEVICE
static int rx_getprop(struct dmpcxt_s *cxtp, uint8_t header, const uint8_t *datap);
static int rx_setprop(struct dmpcxt_s *cxtp, uint8_t header, const uint8_t *datap);

/*
static void rx_subscribe(struct member_s *memb, uint8_t atype, const uint8_t *datap, int datasize);
static void rx_unsubscribe(struct member_s *memb, uint8_t atype, const uint8_t *datap, int datasize);
*/
//static void (uint8_t atype, const uint8_t *datap, int datasize);
#endif

#if CONFIG_DMP_CONTROLLER
/*
static void rx_getpropreply(struct member_s *memb, uint8_t atype, const uint8_t *datap, int datasize);
static void rx_syncevent(struct member_s *memb, uint8_t atype, const uint8_t *datap, int datasize);
static void rx_event(struct member_s *memb, uint8_t atype, const uint8_t *datap, int datasize);
static void rx_getpropfail(struct member_s *memb, uint8_t atype, const uint8_t *datap, int datasize);
static void rx_setpropfail(struct member_s *memb, uint8_t atype, const uint8_t *datap, int datasize);
static void rx_subscribeaccept(struct member_s *memb, uint8_t atype, const uint8_t *datap, int datasize);
static void rx_subscribereject(struct member_s *memb, uint8_t atype, const uint8_t *datap, int datasize);
*/
#endif

/**********************************************************************/
#undef CONFIG_LOGLEVEL
#define CONFIG_LOGLEVEL LOG_DEBUG

#define lgFCTY LOG_DMP

/**********************************************************************/
/*
To avoid passing very large numbers of arguments to some functions 
we pass this DMP context structure which contains most variables 
relevant to processing the current packet.
*/

struct dmpcxt_s {
	member_t *memb;  /* the member from whom we received the message(s) */
	struct propmap_s *ppmap;
	struct {/* items relating to received PDU block we are processing */	
		uint32_t lastaddr;
		uint32_t addr;
		uint32_t inc;
		uint32_t count;
	} rx;
	struct {/* items relating to transmit PDU block we are constructing */
		uint32_t lastaddr;
		uint32_t addr;
		uint32_t inc;
		uint32_t count;
		uint8_t *pdup;
		uint8_t *endp;
		txwrap_t *txwrap;
	} tx;
};

/**********************************************************************/
dmp_start(uuid_t cid, inband callback, outband callback)
{
	...
	sdtRegister(uuid_t cid, grouprx_t scope, uint8_t scopebits,
						uint8_t expiry, memberevent_fn *membevent)
int
sdt_setListener(if_MANYCOMP(Lcomponent_t *Lcomp,) chanOpen_fn *joinRx, /* void *ref, */ netx_addr_t *adhocip)
	sdt_addClient(
int
sdt_addClient(if_MANYCOMP(Lcomponent_t *Lcomp,) dmprx, void *ref)
}

/**********************************************************************/

dmp_accept()
{
	
}

dmp_connect(remote, callback)
{
	
}


/**********************************************************************/
static void
dmp_flushpdus(struct dmpcxt_s *cxtp)
{
	if (cxtp->tx.txwrap == NULL) return;
	if (cxtp->tx.txwrap == NULL) return;
	endProtoMsg(cxtp->tx.txwrap, cxtp->tx.pdup);
	flushWrapper(cxtp->tx.txwrap, NULL);
	cxtp->tx.txwrap = NULL;
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

static uint8_t *
dmp_openpdu(struct dmpcxt_s *cxtp, uint16_t vecnrange, uint32_t addr, 
				uint32_t inc, uint32_t maxcnt)
{
	int datastart;

	if (cxtp->tx.txwrap && addr >= cxtp->tx.lastaddr) {
		/* use relative address if it will give smaller value */
		addr -= cxtp->tx.lastaddr;
		vecnrange |= DMPAD_R;
	}

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
	if (cxtp->tx.txwrap && (cxtp->tx.pdup + datastart + 1) > cxtp->tx.endp) {
		dmp_flushpdus(cxtp);
	}
	if (cxtp->tx.txwrap == NULL) {
		int txlen;
	
		txlen = -1;
		cxtp->tx.txwrap = initMemberMsg(cxtp->memb, DMP_PROTOCOL_ID, &txlen, WRAP_REL_ON | WRAP_REPLY, &cxtp->tx.pdup);
		if (cxtp->tx.txwrap == NULL) {
			acnlogerror(lgERR);
			return NULL;
		}
		cxtp->tx.endp = cxtp->tx.pdup + txlen;
		cxtp->tx.lastaddr = 0;
	}
	marshalU16(cxtp->tx.pdup + OFS_VECTOR, vecnrange);
	cxtp->tx.addr = addr;
	cxtp->tx.inc = inc;
	return cxtp->tx.pdup + datastart;
}

/**********************************************************************/
/*
Close a previously opened PDU.
count is the actual number of PDUs to be entered into the header (if 
0, the PDU is dumped) and nxtp is a pointer to the end of the current 
PDU.
*/

static void
dmp_closepdu(struct dmpcxt_s *cxtp, uint32_t count, uint8_t *nxtp)
{
	uint8_t tatype;
	uint8_t *tp;

	assert(nxtp < cxtp->tx.endp);
	if (count > 0) {
		marshalU16(cxtp->tx.pdup, ((nxtp - cxtp->tx.pdup) + FIRST_FLAGS));
		tatype = cxtp->tx.pdup[DMP_OFS_HEADER];

		tp = cxtp->tx.pdup + DMP_OFS_DATA;
		switch (tatype & DMPAD_SIZEMASK) {
		case (DMPAD_1BYTE):
			tp = marshalU8(tp, cxtp->tx.addr);
			if (IS_RANGE(tatype)) {
				tp = marshalU8(tp, cxtp->tx.inc);
				tp = marshalU8(tp, count);
			}
			break;
		case (DMPAD_2BYTE):
			marshalU16(tp, cxtp->tx.addr);
			if (IS_RANGE(tatype)) {
				tp = marshalU16(tp, cxtp->tx.inc);
				tp = marshalU16(tp, count);
			}
			break;
		case (DMPAD_4BYTE):
			marshalU32(tp, cxtp->tx.addr);
			if (IS_RANGE(tatype)) {
				tp = marshalU32(tp, cxtp->tx.inc);
				tp = marshalU32(tp, count);
			}
			break;
		}
		if ((tatype & DMPAD_R))
			cxtp->tx.lastaddr += cxtp->tx.addr;
		else
			cxtp->tx.lastaddr = cxtp->tx.addr;

		cxtp->tx.lastaddr += cxtp->tx.inc * (count - 1);

		cxtp->tx.pdup = nxtp;
	}
}

/**********************************************************************/
#define setpdutype(code) (pdup[OFS_VECTOR] = (code))
#define PDU_GPFAIL_ALL ((DMP_GET_PROPERTY_FAIL << 8) | DMPAD_RANGE_SINGLE)
#define PDU_GPFAIL_SOME ((DMP_GET_PROPERTY_FAIL << 8) | DMPAD_RANGE_STRUCT)
#define PDU_GPREPLY ((DMP_GET_PROPERTY_REPLY << 8) | DMPAD_RANGE_STRUCT)

int
rx_getprop(struct dmpcxt_s *cxtp, uint8_t header, const uint8_t *datap)
{
	int rslt;
	int goodcnt, errcnt;
	uint8_t *INITIALIZED(txp);
	const struct propinf_s *ppinf;

	/* multiple data formats are illegal */
	if (IS_MULTIDATA(header)) return -1;

	/*
	Test whether the range intersects our map at all.
	A rigorous test is not easy (since high values for inc can 
	produce patterns of addresses which repeatedly wrap round and 
	might intersect genuine properties in some places) so we do a 
	rough test and respond DMPRC_UNSPECIFIED if we don't like it.
	*/
	if (cxtp->rx.count >= cxtp->ppmap->nprops
		|| (uint32_t)(cxtp->rx.inc + (cxtp->ppmap->maxaddr - cxtp->ppmap->minaddr))
			>= (uint32_t)(2 * (cxtp->ppmap->maxaddr - cxtp->ppmap->minaddr))
	) {
		/*
		can't deal with too many properties  or very large +/-increments
		*/
		rslt = DMPRC_UNSPECIFIED;

		goto gpfail_all;

	} else if (cxtp->rx.addr > cxtp->ppmap->maxaddr) {
		uint32_t endaddr = cxtp->rx.addr + (cxtp->rx.count - 1) * cxtp->rx.inc;

		if (endaddr > cxtp->ppmap->maxaddr) {
			rslt = DMPRC_NOSUCHPROP;

gpfail_all:
			txp = dmp_openpdu(cxtp, PDU_GPFAIL_ALL, cxtp->rx.addr, cxtp->rx.count, cxtp->rx.inc);
			*txp++ = rslt;
			dmp_closepdu(cxtp, cxtp->rx.count, txp);
			return 0;
		}
	}
	/*
	We are still not sure all addresses are valid, but range 
	parameters are reasonable so step through one by one
	*/

	goodcnt = errcnt = 0;   /* these indicate if a PDU has started */

	for ( ; cxtp->rx.count >= 0; --cxtp->rx.count) {  /* walk though the properties in the range */

		/* find expected result and estimate data size */
		if (cxtp->rx.addr >= cxtp->ppmap->maxaddr) {
			ppinf = NULL;
			rslt = -DMPRC_NOSUCHPROP;
		} else {
			ppinf = cxtp->ppmap->map + cxtp->rx.addr;
			rslt = ppinf->size;         /* default */
			if (ppinf->flags == 0) {
				rslt = -DMPRC_NOSUCHPROP;
			} else if ((ppinf->flags & PROP_READ) == 0) {
				rslt = -DMPRC_NOREAD;
			} else assert(ppinf->getfn != NULL);
		}

		if (rslt >= 0) {   /* address is OK but may still fail */
			if (errcnt) {
				/*
				switching from error to possible success. If this 
				succeeds we will need to start a new PDU, so read 
				into reseerve buffer
				*/
				uint8_t rsrvread[MAX_PROP_SIZE];

				rslt = (*ppinf->getfn)(ppinf->fnref, rsrvread);
				if (rslt < 0) goto getfnfail2;

				dmp_closepdu(cxtp, errcnt, txp);
				errcnt = 0;
				txp = dmp_openpdu(cxtp, PDU_GPREPLY, cxtp->rx.addr, cxtp->rx.count, cxtp->rx.inc);
				memcpy(txp, rsrvread, rslt);
				txp += rslt;
				++goodcnt;
			} else {
				if (goodcnt && (txp + rslt) > cxtp->tx.endp) {
					/* no room - flush the PDU and start a new one */
					dmp_closepdu(cxtp, goodcnt, txp);
					dmp_flushpdus(cxtp);
					goodcnt = 0;
				}
				if (goodcnt == 0) {
					txp = dmp_openpdu(cxtp, PDU_GPREPLY, cxtp->rx.addr, cxtp->rx.count, cxtp->rx.inc);
				}
				rslt = (*ppinf->getfn)(ppinf->fnref, txp);
				
				/*
				Warning: The following jump dumps an opened PDU and leaves
				both errcnt and goodcnt == 0. Code below calls dmp_openpdu
				again to re-initialize the same PDU. Don't mess here!
				*/
				if (rslt < 0) goto getfnfail1;
				txp += rslt;
				++goodcnt;
			}
		} else {
getfnfail1:
			if (goodcnt) {
				dmp_closepdu(cxtp, goodcnt, txp);
				goodcnt = 0;
			}
getfnfail2:
			if (errcnt && (txp + 1) > cxtp->tx.endp) {
				dmp_closepdu(cxtp, errcnt, txp);
				dmp_flushpdus(cxtp);
				errcnt = 0;
			}
			if (errcnt == 0) {
				txp = dmp_openpdu(cxtp, PDU_GPFAIL_SOME, cxtp->rx.addr, cxtp->rx.count, cxtp->rx.inc);
			}
			*txp++ = -rslt;
			++errcnt;
		}
	}
	if (goodcnt + errcnt) dmp_closepdu(cxtp, goodcnt + errcnt, txp);   /* close any PDU we've started */
	return 0;
}

/************************************************************************/
#define PDU_SPFAIL_ALL ((DMP_SET_PROPERTY_FAIL << 8) | DMPAD_RANGE_SINGLE)
#define PDU_SPFAIL_SOME ((DMP_SET_PROPERTY_FAIL << 8) | DMPAD_RANGE_STRUCT)

int
rx_setprop(struct dmpcxt_s *cxtp, uint8_t header, const uint8_t *datap)
{
	int rslt;
	int errcnt;
	const uint8_t *rxp;	/* pointer to request data */
	uint8_t *INITIALIZED(txp);	/* pointer to response data */
	const struct propinf_s *ppinf;

	/*
	Test whether the range intersects our map at all.
	A rigorous test is not easy (since high values for inc can 
	produce patterns of addresses which repeatedly wrap round and 
	might intersect genuine properties in some places) so we do a 
	rough test and respond DMPRC_UNSPECIFIED if we don't like it.
	*/
	if (cxtp->rx.count >= cxtp->ppmap->nprops
		|| (uint32_t)(cxtp->rx.inc + (cxtp->ppmap->maxaddr - cxtp->ppmap->minaddr))
			>= (uint32_t)(2 * (cxtp->ppmap->maxaddr - cxtp->ppmap->minaddr))
	) {
		/*
		can't deal with too many properties  or very large +/-increments
		*/
		rslt = DMPRC_UNSPECIFIED;

		goto spfail_all;

	} else if (cxtp->rx.addr > cxtp->ppmap->maxaddr) {
		uint32_t endaddr = cxtp->rx.addr + (cxtp->rx.count - 1) * cxtp->rx.inc;

		if (endaddr > cxtp->ppmap->maxaddr) {
			/* whole range is invalid */
			rslt = DMPRC_NOSUCHPROP;

spfail_all:

			txp = dmp_openpdu(cxtp, PDU_SPFAIL_ALL, cxtp->rx.addr, cxtp->rx.count, cxtp->rx.inc);
			*txp++ = rslt;
			dmp_closepdu(cxtp, cxtp->rx.count, txp);
			return -1;   /* return error because this blows out any subsequent address/data fields */
		}
	}

	/*
	we are stil not sure all addresses are valid, but range parameters
	are reasonable so step through one by one
	*/
	errcnt = goodcnt = 0;
	rxp = datap;

	do {  /* walk though the properties in the range */
		/* check valid request */
		if (cxtp->rx.addr >= cxtp->ppmap->maxaddr
			|| (ppinf = cxtp->ppmap->map + cxtp->rx.addr)->flags == 0)
		{
			rslt = -DMPRC_NOSUCHPROP;
		} else {
			if ((ppinf->flags & PROP_EVENT) == 0) {
				rslt = -DMPRC_NOSUBSCRIBE;
			} else {
		   	/* expect OK but may still fail */
				assert(ppinf->subsfn != NULL);
				if ((header & DMPAD_TYPEMASK) == DMPAD_RANGE_SINGLE) {
					rxp = datap;   /* reset source pointer */
				}
				rslt = (*ppinf->subsfn)(ppinf->fnref, rxp);
			}
			if (ppinf->flags & PROP_VSIZE) {
				rxp += unmarshalU16(rxp);
			} else
				rxp += ppinf->size;
		}
		if (rslt < 0) {   /* test for error in write operation */
			if (errcnt && (txp + 1) > cxtp->tx.endp) {
				dmp_closepdu(cxtp, errcnt, txp);
				dmp_flushpdus(cxtp);
				errcnt = 0;
			}
			if (errcnt == 0) {
				txp = dmp_openpdu(cxtp, PDU_SPFAIL_SOME, cxtp->rx.addr, cxtp->rx.count, cxtp->rx.inc);
			}
			*txp++ = -rslt;
			++errcnt;
			if (rslt == -DMPRC_NOSUCHPROP) return -1;
		} else if (errcnt) {
			/* no error, but we had one previously so close the open PDU */
			dmp_closepdu(cxtp, errcnt, txp);
			errcnt = 0;
		}
		cxtp->rx.addr += cxtp->rx.inc;
	} while (--cxtp->rx.count);
	if (errcnt) dmp_closepdu(cxtp, errcnt, txp);
	return rxp - datap;
}
/**********************************************************************/

enum devcmd_e {
	DEV_GET,
	DEV_SET,
	DEV_SBS,
	DEV_UNSBS
};

uint8_t devcmd[] = {
	[DMP_GET_PROPERTY] = DEV_GET,
	[DMP_SET_PROPERTY] = DEV_SET,
	[DMP_SUBSCRIBE] = DEV_SBS,
	[DMP_UNSUBSCRIBE] = DEV_UNSBS,
};

struct {
	uint16_t okrsp,
	uint16_t failall,
	uint16_t failsome,
	uint8_t validflag,
	uint8_t invalrc
} devrsp[] = {
	[DMP_GET_PROPERTY] = {
		(DMP_GET_PROPERTY_REPLY << 8) | DMPAD_RANGE_STRUCT,
		(DMP_GET_PROPERTY_FAIL << 8) | DMPAD_RANGE_SINGLE,
		(DMP_GET_PROPERTY_FAIL << 8) | DMPAD_RANGE_STRUCT,
		PROP_READ, DMPRC_NOREAD
	},
	[DMP_SET_PROPERTY] = {
		0,
		(DMP_SET_PROPERTY_FAIL << 8) | DMPAD_RANGE_SINGLE,
		(DMP_SET_PROPERTY_FAIL << 8) | DMPAD_RANGE_STRUCT,
		PROP_WRITE, DMPRC_NOWRITE
	},
	[DMP_SUBSCRIBE]    = {
		DMP_SUBSCRIBE_ACCEPT, DMP_SUBSCRIBE_REJECT, DMPRC_NOEVENT
		(DMP_SUBSCRIBE_ACCEPT << 8) | DMPAD_RANGE_SINGLE,
		(DMP_SUBSCRIBE_REJECT << 8) | DMPAD_RANGE_SINGLE,
		(DMP_SUBSCRIBE_REJECT << 8) | DMPAD_RANGE_STRUCT,
		PROP_READ, DMPRC_NOREAD
	},
	[DMP_UNSUBSCRIBE]  = {0, 0, 0},
};

#if 0
/*
rx_devcmd() is a generalization of rx_getprop, rx_setprop, rx_subscribe
and rx_event.

incomplete
*/

int
rx_devcmd(uint8_t cmd, struct dmpcxt_s *cxtp, uint8_t header, const uint8_t *datap)
{
	int rslt;
	int goodcnt, errcnt;
	uint8_t *INITIALIZED(txp);
	const struct propinf_s *ppinf;

	/* multiple data formats are illegal */
	if (cmd != DMP_SET_PROPERTY && IS_MULTIDATA(header)) return -1;

	/*
	Test whether the range intersects our map at all.
	A rigorous test is not easy (since high values for inc can 
	produce patterns of addresses which repeatedly wrap round and 
	might intersect genuine properties in some places) so we do a 
	rough test and respond DMPRC_UNSPECIFIED if we don't like it.
	*/
	if (cxtp->rx.count >= cxtp->ppmap->nprops
		|| (uint32_t)(cxtp->rx.inc + (cxtp->ppmap->maxaddr - cxtp->ppmap->minaddr))
			>= (uint32_t)(2 * (cxtp->ppmap->maxaddr - cxtp->ppmap->minaddr))
	) {
		/*
		can't deal with too many properties  or very large +/-increments
		*/
		rslt = DMPRC_UNSPECIFIED;

		goto cmdfail_all;

	} else if (cxtp->rx.addr > cxtp->ppmap->maxaddr) {
		uint32_t endaddr = cxtp->rx.addr + (cxtp->rx.count - 1) * cxtp->rx.inc;

		if (endaddr > cxtp->ppmap->maxaddr) {
			rslt = DMPRC_NOSUCHPROP;

dvfail_all:
			if (devrsp[cmd].failall != 0) {
				txp = dmp_openpdu(cxtp, devrsp[cmd].failall,
								cxtp->rx.addr, cxtp->rx.count, cxtp->rx.inc);
				*txp++ = rslt;
				dmp_closepdu(cxtp, cxtp->rx.count, txp);
			}
			return 0;
		}
	}
	/*
	We are still not sure all addresses are valid, but range 
	parameters are reasonable so step through one by one
	*/

	goodcnt = errcnt = 0;   /* these indicate if a PDU has started */

	for ( ; cxtp->rx.count >= 0; --cxtp->rx.count) {  /* walk though the properties in the range */

		/* find expected result and estimate data size */
		if (cxtp->rx.addr >= cxtp->ppmap->maxaddr) {
			ppinf = NULL;
			rslt = -DMPRC_NOSUCHPROP;
		} else {
			ppinf = cxtp->ppmap->map + cxtp->rx.addr;
			if (ppinf->flags == 0) {
				rslt = -DMPRC_NOSUCHPROP;
			} else if ((ppinf->flags & devrsp[cmd].validflag) == 0) {
				rslt = -devrsp[cmd].invalrc;
			} else {
				rslt = (cmd == DMP_GET_PROPERTY) ? ppinf->size : 0;
			}
		}

		if (rslt >= 0) {   /* address is OK but may still fail */
			if (errcnt) {
				/*
				switching from error to possible success. If this 
				succeeds we will need to start a new PDU, so read 
				into reseerve buffer
				*/
				uint8_t rsrvread[MAX_PROP_SIZE];

				rslt = (*ppinf->propfn)(cmd, ppinf->fnref, rsrvread);
				if (rslt < 0) goto devfnfail2;
				/* success: close the error PDU */
				dmp_closepdu(cxtp, errcnt, txp);
				errcnt = 0;
				if (devrsp[cmd].okrsp) {
					txp = dmp_openpdu(cxtp, devrsp[cmd].okrsp, cxtp->rx.addr, cxtp->rx.count, cxtp->rx.inc);
					memcpy(txp, rsrvread, rslt);
					txp += rslt;
					++goodcnt;
				}
			} else {
				if (devrsp[cmd].okrsp) {
					if (goodcnt && (txp + rslt) > cxtp->tx.endp) {
						/* no room - flush the PDU and start a new one */
						dmp_closepdu(cxtp, goodcnt, txp);
						dmp_flushpdus(cxtp);
						goodcnt = 0;
					}
					if (goodcnt == 0) {
						txp = dmp_openpdu(cxtp, devrsp[cmd].okrsp, cxtp->rx.addr, cxtp->rx.count, cxtp->rx.inc);
					}
					rslt = (*ppinf->getfn)(ppinf->fnref, txp);
					
					/*
					Warning: The following jump dumps an opened PDU and leaves
					both errcnt and goodcnt == 0. Code below calls dmp_openpdu
					again to re-initialize the same PDU. Don't mess here!
					*/
					if (rslt < 0) goto getfnfail1;
					txp += rslt;
					++goodcnt;
				} else {
					rslt = (*ppinf->getfn)(ppinf->fnref, NULL);
					if (rslt < 0) goto getfnfail2;
				}
			}
		} else {
devfnfail1:
			if (goodcnt) {
				dmp_closepdu(cxtp, goodcnt, txp);
				goodcnt = 0;
			}
devfnfail2:
			if (devrsp[cmd].failsome) {
				if (errcnt && (txp + 1) > cxtp->tx.endp) {
					dmp_closepdu(cxtp, errcnt, txp);
					dmp_flushpdus(cxtp);
					errcnt = 0;
				}
				if (errcnt == 0) {
					txp = dmp_openpdu(cxtp, devrsp[cmd].failsome, cxtp->rx.addr, cxtp->rx.count, cxtp->rx.inc);
				}
				*txp++ = -rslt;
				++errcnt;
			}
		}
	}
	if (goodcnt + errcnt) dmp_closepdu(cxtp, goodcnt + errcnt, txp);   /* close any PDU we've started */
	return 0;
}

#endif

/**********************************************************************/
typedef int dmpcmd_fn(struct dmpcxt_s *cxtp, uint8_t header, const uint8_t *datap);

dmpcmd_fn *dmpvectors[] = {
#if CONFIG_DMP_DEVICE
	[DMP_GET_PROPERTY] = &rx_getprop,
	[DMP_SET_PROPERTY] = &rx_setprop,
	[DMP_SUBSCRIBE] = NULL,
	[DMP_UNSUBSCRIBE] = NULL,
#endif
#if CONFIG_DMP_CONTROLLER
	[DMP_GET_PROPERTY_REPLY] = NULL,
	[DMP_EVENT] = NULL,
	[DMP_GET_PROPERTY_FAIL] = NULL,
	[DMP_SET_PROPERTY_FAIL] = NULL,
	[DMP_SUBSCRIBE_ACCEPT] = NULL,
	[DMP_SUBSCRIBE_REJECT] = NULL,
	[DMP_SYNC_EVENT] = NULL,
#endif
};

/************************************************************************/
void
dmprx(struct member_s *memb, const uint8_t *pdus, int blocksize, void *ref)
{
	uint8_t INITIALIZED(header);
	const uint8_t *INITIALIZED(datap);
	int INITIALIZED(datasize);
	const uint8_t *pdup;
	const uint8_t *pp;
	const uint8_t *endp;
	uint8_t flags;
	dmpcmd_fn *INITIALIZED(pdufn);
	struct dmpcxt_s dmpcxt;

	LOG_FSTART(lgFCTY);
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

	dmpcxt.rx.lastaddr = 0;
	dmpcxt.ppmap = (struct propmap_s *)ref;

	dmpcxt.tx.txwrap = NULL;
	dmpcxt.memb = memb;

	for (pdup = pdus; pdup < pdus + blocksize - 2;)
	{
		flags = *pdup;
		pp = pdup + 2;
		pdup += getpdulen(pdup);   /* point to next PDU or end of block */

		if (flags & VECTOR_bFLAG) {
			if (*pp > arraycount(dmpvectors)
				|| (pdufn = dmpvectors[*pp]) == NULL)
			{
				acnlogmark(lgERR, "Unsupported DMP message type %u", *pp);
				continue;
			}
			++pp;
		}
		if (flags & HEADER_bFLAG) header = *pp++;
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
			int rslt;

			dmpcxt.rx.inc = 0; dmpcxt.rx.count = 1;
			/* determine requested address size in bytes */
			switch(header & DMPAD_SIZEMASK) {
			case DMPAD_1BYTE:
				dmpcxt.rx.addr = unmarshalU8(pp); pp += 1;
				if (IS_RANGE(header)) {
					dmpcxt.rx.inc = unmarshalU8(pp); pp += 1;
					dmpcxt.rx.count = unmarshalU8(pp); pp += 1;
				}
				break;
			case DMPAD_2BYTE:
				dmpcxt.rx.addr = unmarshalU16(pp); pp += 2;
				if (IS_RANGE(header)) {
					dmpcxt.rx.inc = unmarshalU16(pp); pp += 2;
					dmpcxt.rx.count = unmarshalU16(pp); pp += 2;
				}
				break;
			case DMPAD_4BYTE:
				dmpcxt.rx.addr = unmarshalU32(pp); pp += 4;
				if (IS_RANGE(header)) {
					dmpcxt.rx.inc = unmarshalU32(pp); pp += 4;
					dmpcxt.rx.count = unmarshalU32(pp); pp += 4;
				}
				break;
			default :
				acnlogmark(lgWARN,"Address length not valid..");
				return;
			}
			if (dmpcxt.rx.count == 0) continue;

			if (IS_RELADDR(header)) dmpcxt.rx.addr += dmpcxt.rx.lastaddr;
			dmpcxt.rx.lastaddr = dmpcxt.rx.addr + (dmpcxt.rx.count - 1) * dmpcxt.rx.inc;

			rslt = (*pdufn)(&dmpcxt, header, pp);
			if (rslt < 0) break;   /* severe error - dump entire DMP command */
			pp += rslt;
		}
	}
	if (dmpcxt.tx.txwrap) dmp_flushpdus(&dmpcxt);

	if (pdup != pdus + blocksize)  { /* sanity check */
		acnlogmark(lgNTCE, "Rx blocksize mismatch");
	}
	LOG_FEND(lgFCTY);
}
