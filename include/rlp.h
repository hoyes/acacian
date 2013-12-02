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

ACN Root Layer Protocol
*/

#ifndef __rlp_h__
#define __rlp_h__ 1

#ifdef __cplusplus
extern "C" {
#endif

struct rxcontext_s;

struct rlphandler_s {
#if ACNCFG_RLP_MAX_CLIENT_PROTOCOLS > 1
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
	struct rlphandler_s handlers[ACNCFG_RLP_MAX_CLIENT_PROTOCOLS];
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
extern int rlp_sendbuf(uint8_t *txbuf, int length, 
#if ACNCFG_RLP_MAX_CLIENT_PROTOCOLS > 1
								protocolID_t protocol,
#endif
								struct rlpsocket_s *src, netx_addr_t *dest, uint8_t *srccid);

#ifdef __cplusplus
}
#endif

#endif
