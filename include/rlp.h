/************************************************************************/
/*
Copyright (c) 2010, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3s
*/
/************************************************************************/
#ifndef __rlp_h__
#define __rlp_h__ 1

#if CONFIG_EPI17
#include "acnstd/epi17.h"
#endif
#if CONFIG_NET_TCP
#include "acnstd/rlp_tcp.h"
#endif
#include "acnstd/protocols.h"
#include "acnlists.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rlphandler_s rlphandler_t;
typedef struct skgroups_s skgroups_t;
typedef struct rlpsocket_s rlpsocket_t;
typedef struct rlp_txbuf_s rlp_txbuf_t;

struct rxcontext_s;

struct rlphandler_s {
#if !CONFIG_RLP_SINGLE_CLIENT
   protocolID_t protocol;
#endif
   rlpcallback_fn *func;
   //void *ref;
#if CONFIG_NET_UDP
   int nsubs;
#endif
};

struct skgroups_s {
   struct {struct skgroups_s *r;} lnk;  //  slLink(struct skgroups_s, lnk);
   nativesocket_t     sk;
   int                ngp;
   grouprx_t          mad[IP_MAX_MEMBERSHIPS];
   int                nm[IP_MAX_MEMBERSHIPS];
};

struct rlpsocket_s {
   slLink(rlpsocket_t, lnk);
   //int16_t             usecount;
#if CONFIG_NET_UDP
   port_t              port;
   nativesocket_t      sk;
   struct skgroups_s   *groups;
#endif
   poll_fn             *pollrx;
   rlphandler_t        handlers[MAX_RLP_CLIENT_PROTOCOLS];
};

/************************************************************************/
/*
PDU sizes an offsets
*/

#define RLP_OFS_PDU1DATA   (RLP_PREAMBLE_LENGTH + OFS_VECTOR + (int)sizeof(protocolID_t) + (int)sizeof(uuid_t))    /* 38 */
#define RLP_OVERHEAD      (RLP_OFS_PDU1DATA + RLP_POSTAMBLE_LENGTH)
#define RLP_PDU_MINLENGTH 2

/************************************************************************/
/*
Prototypes
*/
extern int rlp_init(void);
#if CONFIG_NET_UDP
extern int rlp_sendbuf(uint8_t *txbuf, int length, if_RLP_MANYCLIENT(protocolID_t protocol,) 
                        rlpsocket_t *src, netx_addr_t *dest, uuid_t srccid);
#elif CONFIG_NET_TCP
extern int rlp_sendbuf(uint8_t *txbuf, int length, if_RLP_MANYCLIENT(protocolID_t protocol,)
								struct tcpconnect_s *cxn);
#endif /* CONFIG_NET_TCP */

#ifdef __cplusplus
}
#endif

#endif
