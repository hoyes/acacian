/**********************************************************************/
/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (c) 2013, Acuity Brands, Inc.

Author: Philip Nye <philip.nye@engarts.com>

#tabs=3
*/
/**********************************************************************/
/*
about: Acacian

Acacian is a full featured implementation of ANSI E1.17 2012
Architecture for Control Networks (ACN) from Acuity Brands

header: e131.h

E1.31 sACN (streaming ACN) macros and functions
*/

#ifndef __e131_h__
#define __e131_h__ 1

// Base address of multicast range 239.255.x.x
// Ensure htonl is used on this
#define E131_MULTICAST_BASE 0xefff0000

/*************************************************************************
   Test for out-of-sequence. See E1.31 spec section 6.9.2
*************************************************************************/
#define LOST_SEQ_THRESHOLD  20

#define MISS_SEQ_LIMIT      -20

#define E131_SRCXCD_TIMEOUT 2500
#define E131_UNIV_TIMEOUT   2500

// Option flags
#define E131_OPT_PREVIEW   0x80
#define E131_OPT_TERMINATE 0x40

// E131 packet has many items at fixed offsets so we can hard define them
// Framing Layer Offsets - from start of framing layer PDU
#define E131_FLO_LENGTH   0
#define E131_FLO_VECTOR   2
#define E131_FLO_UACN     6
#define E131_FLO_PRIORITY 70
#define E131_FLO_RESERVED 71
#define E131_FLO_SEQNO    73
#define E131_FLO_OPTIONS  74
#define E131_FLO_UNIVERSE 75
#define E131_FLO_DMP      77

// DMP Layer offsets - also from start of framing layer PDU
#define E131_DLO_LENGTH   77
#define E131_DLO_VECTOR   79
#define E131_DLO_ADDRTYPE 80
#define E131_DLO_ADDR     81
#define E131_DLO_INC      83
#define E131_DLO_COUNT    85
#define E131_DLO_START    87
#define E131_DLO_DMXDATA  88

#define E131_UACN_SIZE 64

typedef void e131cb_openunv_fn(uint16_t universe);
typedef void e131cb_closeunv_fn(uint16_t universe);
typedef void e131cb_dmxdata_fn(uint16_t universe, struct component_s *srccomp, uint8_t priority,
#if !CONFIG_E131_IGNORE_PREVIEW
                               uint8_t options,
#endif
#if !CONFIG_E131_ZSTART_ONLY
                               uint8_t STARTcode,
#endif
                               const uint8_t *slotdata, uint16_t slotcount);
typedef void e131cb_srcxcd_fn(uint16_t universe, struct component_s *srccomp, uint8_t priority);
typedef void e131cb_newsrc_fn(uint16_t universe, struct component_s *srccomp, uint8_t priority);
typedef void e131cb_lostsrc_fn(uint16_t universe, struct component_s *srccomp, uint8_t reason);

enum lostSrc_e {
   LOST_TERMINATE = 1,
   LOST_TIMEOUT,
   LOST_PRIORITY,
   LOST_CLOSING
};

struct e131_source_s {
   struct component_s *component;
   uint8_t priority;
   uint8_t seqno;
   struct timer_s rxtimer;
};

struct e131_callback_s {
//   void *cookie;
   e131cb_openunv_fn  *notify_open;
   e131cb_closeunv_fn *notify_close;
   e131cb_dmxdata_fn  *notify_data;
   e131cb_srcxcd_fn   *notify_srcxcd;
   e131cb_newsrc_fn   *notify_newsrc;
   e131cb_lostsrc_fn  *notify_lostsrc;
};

struct e131_univinfo_s {
   struct e131_univinfo_s *next;
   uint16_t universe;
   struct e131_callback_s app_cb;
   struct e131_source_s sources[E131_MAX_SOURCES];
   struct rlp_listener_s *listener;
   uint8_t hipri;
   uint8_t lopri;
   struct timer_s srcxcdtimer;
};

#if (CONFIG_E131_MEM == MEM_STATIC)
extern void e131_init_m(void);
#else
#define e131_init_m()
#endif
extern struct e131_univinfo_s *e131_register_rx(uint16_t universe, localaddr_t *localaddr, const struct e131_callback_s *app_callback, int tries);
extern void e131_deregister_rx(struct e131_univinfo_s *univinfo);


#endif // __e131_h__
