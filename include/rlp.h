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
   protocolID_t protocol;
   rlpcallback_fn *func;
   void *ref;
   int nsubs;
};

struct skgroups_s {
   struct {struct skgroups_s *r;} lnk;  //  slLink(struct skgroups_s, lnk);
   nativesocket_t     sk;
   int                ngp;
   ip4addr_t          mad[IP_MAX_MEMBERSHIPS];
   int                nm[IP_MAX_MEMBERSHIPS];
};

struct rlpsocket_s {
   slLink(rlpsocket_t, lnk);
   int16_t             usecount;
   port_t              port;
   nativesocket_t      sk;
   poll_fn             *pollrx;
   struct skgroups_s   *groups;
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
extern int rlp_sendbuf(uint8_t *txbuf, int length, if_RLP_MANYCLIENT(protocolID_t protocol,) 
                        rlpsocket_t *src, netx_addr_t *dest, uuid_t srccid);

#ifdef __cplusplus
}
#endif

#endif
