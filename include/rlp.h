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
header: rlp.h

ACN Root Layer Protocol and Networking Interface
*/

#ifndef __rlp_h__
#define __rlp_h__ 1

/**********************************************************************/
/*
struct: rxbuf_s

At minimum, the rxbuffer needs a usecount attaching. There are several
possibilities.

	o Define a structure with the usecount ahead of the data. This 
	allows a size field too and variable size data blocks. However, 
	it does not work in an OS/Stack where the raw packet is used and 
	the space ahead of the data is filled with UDP and IP headers.

	o As previous, but in the case of a stack supplying a raw 
	packet, the UDP/IP headers get overwritten by the usecount. Only 
	works if the stack will tolerate this.
	
	o Place the usecount after the end of the data. This limits the data
	buffer to a fixed size so we know where the usecount is.
	
	o Use a separate structure supplying the usecount and a pointer to
	the data buffer. This adds the overhead of memory management for a
	very small structure. In many impolementations, the pointer to the
	buffer is bigger than the whole of the rest of the structure.

The definition here assumes the first.
*/

typedef struct rxbuf_s rxbuf_s;

struct rxbuf_s {
	uint16_t usecount;
	uint8_t data[MAX_MTU];
};

static inline void
releaseRxbuf(struct rxbuf_s *rxbuf)
{
	if (--rxbuf->usecount <= 0) {
		free(rxbuf);
	}
}

#define getRxdata(rxbufp) (rxbufp)->data
#define getRxBsize(rxbufp) MAX_MTU

/**********************************************************************/
struct rxcontext_s;
struct rlpsocket_s;

typedef void rlpcallback_fn(const uint8_t *data, int datasize, 
								struct rxcontext_s *rcxt);

struct rlphandler_s {
#if CF_RLP_MAX_CLIENT_PROTOCOLS > 1
	protocolID_t protocol;
#endif
	rlpcallback_fn *func;
	void *ref;
	int nsubs;
};

struct skgroups_s {
	struct {struct skgroups_s *r;} lnk;  //  slLink(struct skgroups_s, lnk);
	nativesocket_t     sk;
	int                ngp;
	grouprx_t          mad[IP_MAX_MEMBERSHIPS];
	int                nm[IP_MAX_MEMBERSHIPS];
};

struct rlpsocket_s {
	slLink(struct rlpsocket_s, lnk);
	//int16_t             usecount;
	port_t              port;
	nativesocket_t      sk;
	struct skgroups_s   *groups;
	poll_fn             *rxfn;
	struct rlphandler_s handlers[CF_RLP_MAX_CLIENT_PROTOCOLS];
};

/************************************************************************/
/*
PDU sizes an offsets
*/

#define RLP_OFS_PDU1DATA   (RLP_PREAMBLE_LENGTH + OFS_VECTOR + (int)sizeof(protocolID_t) + UUID_SIZE)    /* 38 */
#define RLP_OVERHEAD      (RLP_OFS_PDU1DATA + RLP_POSTAMBLE_LENGTH)
#define RLP_PDU_MINLENGTH 2

/************************************************************************/
/*
Prototypes
*/

extern int rlp_init(void);

extern struct rlpsocket_s *rlpSubscribe(netx_addr_t *lclad, 
							protocolID_t protocol, 
							rlpcallback_fn *callback, void *ref);

extern int rlpUnsubscribe(struct rlpsocket_s *rs, netx_addr_t *lclad, 
							protocolID_t protocol);

int netxGetMyAddr(struct rlpsocket_s *rs, netx_addr_t *addr);

extern int rlp_sendbuf(uint8_t *txbuf, int length,
								ifRLP_MP(protocolID_t protocol,)
								struct rlpsocket_s *src, netx_addr_t *dest, 
								uint8_t *srccid);

#endif  /* __rlp_h__ */
