/************************************************************************/
/*
Copyright (c) 2010, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3t
*/
/************************************************************************/
#ifndef __rlp_h__
#define __rlp_h__ 1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rlphandler_s rlphandler_t;
typedef struct skgroups_s skgroups_t;
typedef struct rlpsocket_s rlpsocket_t;
typedef struct rlp_txbuf_s rlp_txbuf_t;

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
	rlphandler_t        handlers[ACNCFG_RLP_MAX_CLIENT_PROTOCOLS];
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
