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
file: e131rx.c

Implementation of E1.31 treaming protocol (sACN)

Warning:

This source worked once but much has changed in Acacian since then and
E131 has not been updated.
*/

#include <unistd.h>
#include <errno.h>

#include "acn.h"

/* some handy macros */
#define LOG_FSTART() acnlog(LOG_DEBUG | LOG_E131, "%s [", __func__)
#define LOG_FEND() acnlog(LOG_DEBUG | LOG_E131, "%s ]", __func__)

/**********************************************************************/
/*
Retry parameters for opening a socket
Useful if application runs before networking is fully functional
e.g. because of DHCP delays
Timeouts are in seconds and starts at an intitial value and double each 
time up to a maximum.
The app can specify the number of tries when calling e131_register_rx()
*/
#define E131_OPEN_WAIT_TIMEOUT      2
#define E131_OPEN_WAIT_MAXTIMEOUT   120
/*
E131_IGNORE_COUNT is a workaround for a bug in some versions of sACNview
#define E131_IGNORE_COUNT 1
*/
/**********************************************************************/
/*
Implementation of priority arbitration in receivers

A limit of E131_MAX_SOURCES is applied per universe. This limit is also
the number of source components tracked so if a packet is rejected for
priority/arbitration reasons, no record is kept of the component which
sent it. Up to E131_MAX_SOURCES we maintain a record of highest and
lowest priority values: hipri and lopri. If we have E131_MAX_SOURCES at
the same highest priority then hipri == lopri. If we have fewer than
E131_MAX_SOURCES of any sort then lopri == 0.

According to the spec, we must impose some sort of deterministic priority

When a packet is received for a universe with a priority P.

If P < lopri we dump the packet.
Otherwise we look up the source in our tracking list.
If it is known we:
   update activity timer for that source
   update priority for that source and if changed update lopri and hipri
   now if P < hipri we dump the packet
   else pass packet up to app with source info - don't merge or impose
   other algorithms
If the source is unknown:
   if tracked components < E131_MAX_SOURCES add the new one
   else
      find a tracked component with priority == lopri
      If P > lopri replace this tracked comp with the new one then:
         if P >= hipri pass packet up to app - don't merge
      Else P == lopri: if lopri == hipri we have sources_exceeded
      condition call to inform app, set sources_exceeded timer and dump
      the packet.

When a tracked source disappears (timeout or stream_terminated) it is
removed from the tracked list. There may be a glitch as a new source is 
established but the above algorithm provides a good chance the next lowest
component will be tracked already.
*/
/**********************************************************************/

struct netxsocket_s *e131sock = NULL;   /* can do it all on one socket */
void e131_rx_callback(const uint8_t *datap, int datasize, void *ref, const netx_addr_t *remhost, const cid_t remcid);
void rx_timeout(struct timer_s *timer);

/*
   acn_protect_t  protect;
   protect = ACN_PORT_PROTECT();
   ACN_PORT_UNPROTECT(protect);
*/

#if (CONFIG_E131_MEM == MEM_STATIC)

struct e131_univinfo_s univ_m[E131MEM_MAXUNIVS];
struct e131_univinfo_s *active_univs;
struct e131_univinfo_s *free_univs;

int e131_had_init = 0;

void e131_init_m(void)
{
   acn_protect_t  protect;
   e131_univinfo_s *univinfo, *lstinf;

   LOG_FSTART();
   protect = ACN_PORT_PROTECT();
   active_univs = lstinf = NULL;
   univinfo = univ_m;
   while (univinfo < univ_m + E131MEM_MAXUNIVS) {
      univinfo->next = lstinf;
      lstinf = univinfo++;
   }
   free_univs = lstinf;
   ACN_PORT_UNPROTECT(protect);
   LOG_FEND();
}

static inline struct e131_univinfo_s *new_univinfo(uint16_t universe)
{
   struct e131_univinfo_s *univinfo;

   if ((univinfo = free_univs) != NULL) {
      free_univs = univinfo->next;
      univinfo->next = active_univs;
      active_univs = univinfo;
      memset(univinfo, 0, sizeof(struct e131_univinfo_s))
   }
   return univinfo;
}

void free_univinfo(struct e131_univinfo_s *univinfo)
{
   struct e131_univinfo_s *xuniv;

   if ((xuniv = active_univs) == univinfo) active_univs = univinfo->next;
   else while (xuniv && xuniv->next != univinfo) xuniv = xuniv->next;
   if (xuniv) xuniv->next = univinfo->next;
   
   univinfo->next = free_univs;
   free_univs = univinfo;
}

#elif (CONFIG_E131_MEM == MEM_MALLOC)

struct e131_univinfo_s *active_univs = NULL;

#define e131_init_m()

static inline struct e131_univinfo_s *new_univinfo(uint16_t universe)
{
   struct e131_univinfo_s *univinfo;

   if ((univinfo = (struct e131_univinfo_s *)calloc(1, sizeof(struct e131_univinfo_s))) != NULL) {
      univinfo->next = active_univs;
      active_univs = univinfo;
   }
   return univinfo;   
}

void free_univinfo(struct e131_univinfo_s *univinfo)
{
   struct e131_univinfo_s *xuniv;

   if ((xuniv = active_univs) == univinfo) active_univs = univinfo->next;
   else while (xuniv && xuniv->next != univinfo) xuniv = xuniv->next;
   if (xuniv) xuniv->next = univinfo->next;
   free(univinfo);
}

#endif

int num_univs = 0;

/**********************************************************************/
struct e131_univinfo_s *findunivinfo(uint16_t univ)
{
   struct e131_univinfo_s *univinfo;
   
   for (univinfo = active_univs; univinfo != NULL; univinfo = univinfo->next)
      if (univinfo->universe == univ) break;
   return univinfo;
}

/**********************************************************************/
/*
Just open a socket for the local address/port in e131addr
If e131addr is NULL default to inaddr_any and SDT_MULTICAST_PORT
Try tries times - if tries is 0 repeat indefinitely
*/
/**********************************************************************/
struct netxsocket_s *e131_socket(localaddr_t *e131addr, int tries)
{
   int rslt;
   localaddr_t rxaddr;
   int wait;
   int try = 0;
   struct netxsocket_s *nsk;

   LOG_FSTART();
   if (e131addr == NULL) {
      e131addr = &rxaddr;
      netx_INIT_LOCALADDR(&rxaddr, netx_INADDR_ANY, htons(SDT_MULTICAST_PORT));
   }
   wait = E131_OPEN_WAIT_TIMEOUT;
   while ((nsk = rlp_open_netsocket(e131addr)) == NULL) {
      if (tries && --tries == 0) break;
      sleep(wait);
      wait = wait * 2;
      if (wait > E131_OPEN_WAIT_MAXTIMEOUT) wait = E131_OPEN_WAIT_MAXTIMEOUT;
   }
   LOG_FEND();
   return nsk;
}

/**********************************************************************/
/*
   Register a receiver for a universe
*/
/**********************************************************************/
struct rlp_listener_s *unicast_listener;

struct e131_univinfo_s *e131_register_rx(uint16_t universe, localaddr_t *localaddr, const struct e131_callback_s *app_callback, int tries)
{
   struct e131_univinfo_s *univinfo;
   struct e131_source_s *srcp;
   acn_protect_t  protect;

   LOG_FSTART();
   if (universe == 0 || universe >= 64000) {
      acnlog(LOG_WARNING | LOG_E131, "e131_register_rx: bad universe No\n");
      errno = EINVAL;
      return NULL;
   }

   protect = ACN_PORT_PROTECT();
   if ((univinfo = new_univinfo(universe)) == NULL) {
      errno = ENOMEM;
   } else {
      /* if we're first we need to open a new socket */
      if (e131sock == NULL) {
         if ((e131sock = e131_socket(localaddr, tries)) == NULL) {
            free_univinfo(univinfo);
            univinfo = NULL;
            goto register_rx_exit;
         }
         if (NULL == (unicast_listener = rlp_add_listener(e131sock, netx_GROUP_UNICAST,
                                                         E131_PROTOCOL_ID,
                                                         e131_rx_callback, NULL))) {
            free_univinfo(univinfo);
            rlp_close_netsocket(e131sock);
            e131sock = NULL;
            univinfo = NULL;
            goto register_rx_exit;
         }
      }
      univinfo->universe = universe;
      univinfo->srcxcdtimer.action = NULL;   // Don't need callback
      univinfo->srcxcdtimer.userp = (void *)univinfo;
      for (srcp = univinfo->sources; srcp < univinfo->sources + E131_MAX_SOURCES; ++srcp) {
         srcp->rxtimer.action = &rx_timeout;
         srcp->rxtimer.userp = (void *)univinfo;
      }
      memcpy(&univinfo->app_cb, app_callback, sizeof(struct e131_callback_s));

      if (NULL == (univinfo->listener = rlp_add_listener(e131sock, 
                                                      htonl(E131_MULTICAST_BASE | universe),
                                                      E131_PROTOCOL_ID,
                                                      e131_rx_callback, (void *)univinfo))) {
         free_univinfo(univinfo);
         univinfo = NULL;
         if (num_univs == 0) {
            rlp_del_listener(e131sock, unicast_listener);
            rlp_close_netsocket(e131sock);
            e131sock = NULL;
         }
      } else {
         ++num_univs;
      }
   }
register_rx_exit:
   ACN_PORT_UNPROTECT(protect);
   LOG_FEND();
   return univinfo;
}

/**********************************************************************/
/*
*/
/**********************************************************************/

void e131_deregister_rx(struct e131_univinfo_s *univinfo)
{
   struct component_s *comp;
   struct e131_source_s *srcp;
   acn_protect_t  protect;

   LOG_FSTART();
   if (univinfo == NULL) return;

   protect = ACN_PORT_PROTECT();
   rlp_del_listener(e131sock, univinfo->listener); // turn off the listener
   cancel_timer(&univinfo->srcxcdtimer);
   for (srcp = univinfo->sources; srcp < univinfo->sources + E131_MAX_SOURCES; ++srcp) {
      if ((comp = srcp->component) != NULL) {
         cancel_timer(&srcp->rxtimer);
         comp_release(comp);   // forget the component
      }
   }
   if (univinfo->app_cb.notify_close) (*univinfo->app_cb.notify_close)(univinfo->universe);
   free_univinfo(univinfo);
   if (--num_univs == 0) {
      rlp_del_listener(e131sock, unicast_listener);
      rlp_close_netsocket(e131sock);
      e131sock = NULL;
   }

   LOG_FEND();
   ACN_PORT_UNPROTECT(protect);
}

/**********************************************************************/
/*
   Receive and check an E131 packet
*/
/**********************************************************************/
const uint8_t dmpmatch[] = {0x02, 0xa1, 0x00, 0x00, 0x00, 0x01};

void e131_rx_callback(const uint8_t *datap, int datasize, void *ref, const netx_addr_t *remhost, const cid_t remcid)
{
   struct e131_univinfo_s *univinfo = (struct e131_univinfo_s *)ref;
   uint16_t len;
   uint16_t univ;
   uint8_t priority;
   struct e131_source_s *srcp;
   struct e131_source_s *freesrcp;
   struct e131_source_s *losrcp;
   struct e131_source_s *mysrcp;
   uint8_t hipri, lopri;
   int numAtHipri;
   struct component_s *comp;
   uint32_t vec;

//   LOG_FSTART();
   // check the E1.31 packet - quick tests likely to fail first to eliminate bad packets ASAP
   // if len and flags are wrong we can't trust anything else
   len = unmarshalU16(datap);
   if ((len & FLAG_MASK) != FIRST_FLAGS) {
      acnlog(LOG_ERR | LOG_E131, "e131_rx: framing layer: bad flags %" PRIx16, len & FLAG_MASK);
      return;
   }
   if ((len & LENGTH_MASK) != datasize) {
      acnlog(LOG_ERR | LOG_E131, "e131_rx: framing layer: bad length %" PRIu16, len & LENGTH_MASK);
      return;
   }
   if ((vec = unmarshalU32(datap + E131_FLO_VECTOR)) != DMP_PROTOCOL_ID) {
      acnlog(LOG_ERR | LOG_E131, "e131_rx: framing layer: bad vector %" PRIu32, vec);
      return;
   }
#if CONFIG_E131_IGNORE_PREVIEW
   if ((datap[E131_FLO_OPTIONS] & E131_OPT_PREVIEW)) {
      acnlog(LOG_DEBUG | LOG_E131, "e131_rx: preview dump");
      return;
   }
#endif
   // Now check dmp layer
   // DMP layer flags and length
   len = unmarshalU16(datap + E131_DLO_LENGTH);
   if ((len & FLAG_MASK) != FIRST_FLAGS) {
      acnlog(LOG_ERR | LOG_E131, "e131_rx: DMP layer: bad flags %" PRIx16, len & FLAG_MASK);
      return;
   } 
   if ((len & LENGTH_MASK) != (datasize - E131_FLO_DMP)) {
      acnlog(LOG_ERR | LOG_E131, "e131_rx: DMP layer: bad length %" PRIu16, len & LENGTH_MASK);
      return;
   }
   // rest of DMP pdu up to count
   if (memcmp(datap + E131_DLO_VECTOR, dmpmatch, sizeof(dmpmatch)) != 0) {
      acnlog(LOG_ERR | LOG_E131, "e131_rx: DMP layer: bad format "
                        "%02" PRIu8 "x, %02" PRIu8 "x, %" PRIu16 "u, %" PRIu16,
                        datap[E131_DLO_VECTOR], datap[E131_DLO_ADDRTYPE],
                        unmarshalU16(datap + E131_DLO_ADDR),
                        unmarshalU16(datap + E131_DLO_INC));
      return;
   }
#ifndef E131_IGNORE_COUNT
   // dmp count
   if ((len = unmarshalU16(datap + E131_DLO_COUNT)) != (datasize - E131_DLO_START)) {
      acnlog(LOG_ERR | LOG_E131, "e131_rx: DMP layer: bad count %" PRIu16, len);
      return;
   }
#endif

#if CONFIG_E131_ZSTART_ONLY
   switch (datap[E131_DLO_START])
   {
   case 0: break;
   case 0xdd:  //don't want to even log this one
   default:
      acnlog(LOG_DEBUG | LOG_E131, "e131_rx: ASC %" PRIx8, datap[E131_DLO_START]);
      return;
      // Fall through
   }
#endif
   univ = unmarshalU16(datap + E131_FLO_UNIVERSE);
   if (univinfo == NULL) { // must be unicast - need to find the right universe
      if ((univinfo = findunivinfo(univ)) == NULL) {
         acnlog(LOG_DEBUG | LOG_E131, "e131_rx: unicast wrong universe");
         return;
      }
   } else if (univ != univinfo->universe) {
      // likely to receive wrong universes if STACK_RETURNS_DEST_ADDR is false. Check anyway
      acnlog(LOG_INFO | LOG_E131, "e131_rx: universe mismatch");
      return;
   }
   // looks like a good packet
   // now perform source tracking and prioritizing
   if ((priority = datap[E131_FLO_PRIORITY]) < univinfo->lopri) {
      acnlog(LOG_DEBUG | LOG_E131, "e131_rx: drop priority %" PRIu8 " < %" PRIu8, priority, univinfo->lopri);
      return;
   }

   // search univinfo first
   // we aim to scan only once so save a free slot in case we need it,
   // or failing that, the slot with lowest priority
   // update the priority range as we go
   hipri = 0;
   lopri = 255;
   losrcp = mysrcp = freesrcp = NULL;
   for (srcp = univinfo->sources; srcp < (univinfo->sources + E131_MAX_SOURCES); ++srcp) {
      if ((comp = srcp->component) == NULL) {
         // spare record
         freesrcp = srcp;
         lopri = 0;
         continue;
      }
      if (cidIsEqual(comp->cid, remcid)) {
         // This is the case for most packets we hope
         mysrcp = srcp;             // save a pointer
         srcp->priority = priority;    // in case priority changed
      }
      if (srcp->priority < lopri) { // check min priority
         lopri = srcp->priority;
         losrcp = srcp;
      }
      if (srcp->priority > hipri) {
         hipri = srcp->priority; // check max priority
         numAtHipri = 0;         // will increment below
      }
      if (srcp->priority == hipri) ++numAtHipri; // check max priority
   }
   if ((datap[E131_FLO_OPTIONS] & E131_OPT_TERMINATE)) {
      if (mysrcp) {
         acnlog(LOG_INFO | LOG_E131, "e131_rx: active source terminate");
         // lost a tracked source
         if (univinfo->app_cb.notify_lostsrc)
            (*univinfo->app_cb.notify_lostsrc)(univinfo->universe, mysrcp->component, LOST_TERMINATE);
         comp_release(mysrcp->component);   // forget the component
         mysrcp->component = NULL;
         cancel_timer(&mysrcp->rxtimer);
         univinfo->lopri = mysrcp->priority = 0;   // set priority to 0
      } else {
         acnlog(LOG_DEBUG | LOG_E131, "e131_rx: other source terminate");
      }
      return;
   }
   if (mysrcp) {  // check sequence no
      uint8_t b;

      if ((b = datap[E131_FLO_SEQNO] - mysrcp->seqno - 1) > (255 - LOST_SEQ_THRESHOLD)) {
         acnlog(LOG_NOTICE | LOG_E131, "e131_rx: lost sequence");
         return;
      } else if (b != 0) {
         // acnlog(LOG_INFO | LOG_E131, "e131_rx: missed %" PRIu8 " packets", b);
      } else {
         // acnlog(LOG_DEBUG | LOG_E131, "e131_rx: seq OK");
      }
   } else {
      // new source (not in our list)
      // if it is lower or equal to lowest priority (and not all equal), dump it
      if (priority <= lopri && priority < hipri) {
         acnlog(LOG_DEBUG | LOG_E131, "e131_rx: drop new source");
         return;
      }

      // no - we need to track it
      // see if we already have the component or get a new one
      comp = comp_get_by_cid(remcid);
      if (comp == NULL) {
         acnlog(LOG_DEBUG | LOG_E131, "e131_rx: new source no mem");
         return;   // out of mem or something
      }

      // if we found a spare source record simply save it there
      if (freesrcp) {
         mysrcp = freesrcp;
         goto newcomp;
      } else if (priority > lopri) {
         // no spare sources - we're not the lowest so dump a lowest priority
         assert(losrcp);
         mysrcp = losrcp;
         if (univinfo->app_cb.notify_lostsrc) {
            (*univinfo->app_cb.notify_lostsrc)(univinfo->universe, mysrcp->component, LOST_PRIORITY);
         }
         comp_release(mysrcp->component);   // forget that one
         //acnlog(LOG_INFO | LOG_E131, "e131_rx: dump tracked source");
newcomp:    // tracking a new component
         mysrcp->component = comp;
         mysrcp->priority = priority;
         if (comp->created_by == cbNONE) comp->created_by = cbE131;
         if (datap[E131_FLO_UACN]) {
            strncpy(comp->uacn, datap + E131_FLO_UACN, ((ACN_UACN_SIZE < E131_UACN_SIZE) ? ACN_UACN_SIZE : E131_UACN_SIZE));
         }
         if (univinfo->app_cb.notify_newsrc) {
            acnlog(LOG_DEBUG | LOG_E131, "e131_rx: new"); return;
            (*univinfo->app_cb.notify_newsrc)(univinfo->universe, comp, priority);
         }
         if (priority > hipri) {
            hipri = priority;
            numAtHipri = 0;   // will increment below
         }
      } else {
         // can only get here if priority == lopri == hipri and no free slot
         comp_release(comp);    // release - still not tracking it
         comp = NULL;
      }
      if (priority == hipri) ++numAtHipri;
   }
   univinfo->hipri = hipri;
   univinfo->lopri = lopri;
   if (numAtHipri > MAX_HIPRI_SOURCES) {
      // sources exceeded
      // univinfo->srcxcdtimer proxies for rx_timeout on untracked sources
      if (univinfo->app_cb.notify_srcxcd && !is_active(&univinfo->srcxcdtimer))
         (*univinfo->app_cb.notify_srcxcd)(univinfo->universe, comp, priority);
      set_timeout(&univinfo->srcxcdtimer, E131_SRCXCD_TIMEOUT);
      acnlog(LOG_INFO | LOG_E131, "e131_rx: sources exceeded");
      return;
   }
   if (is_active(&univinfo->srcxcdtimer)) {
      // set srcxcdtimer previously - is it still valid?
      if (numAtHipri == MAX_HIPRI_SOURCES) return;
      cancel_timer(&univinfo->srcxcdtimer);
   }
   assert(mysrcp != NULL);
   set_timeout(&mysrcp->rxtimer, E131_UNIV_TIMEOUT);
   mysrcp->seqno = datap[E131_FLO_SEQNO];
   // OK we have a packet for the app
   if (priority == hipri && univinfo->app_cb.notify_data)
      (* univinfo->app_cb.notify_data)(univ, comp, priority,
#if !CONFIG_E131_IGNORE_PREVIEW
                                       datap[E131_FLO_OPTIONS],
#endif
#if !CONFIG_E131_ZSTART_ONLY
                                       datap[E131_DLO_START],
#endif
                                       datap + E131_DLO_DMXDATA, datasize - E131_DLO_DMXDATA);
//   LOG_FEND();
}

void rx_timeout(struct timer_s *timer)
{
   struct e131_univinfo_s *univinfo;
   struct e131_source_s *srcp;


   //LOG_FSTART();
   univinfo = (struct e131_univinfo_s *)(timer->userp);
   //which source has timed out?
   for (srcp = univinfo->sources; &srcp->rxtimer != timer;) {
      if (++srcp >= univinfo->sources + E131_MAX_SOURCES) {
         acnlog(LOG_CRIT | LOG_E131, "rx_timeout: can't match source");
         return;
      }
   }
   // lost a tracked source
   if (univinfo->app_cb.notify_lostsrc)
      (*univinfo->app_cb.notify_lostsrc)(univinfo->universe, srcp->component, LOST_TIMEOUT);
   comp_release(srcp->component);   // forget the component
   srcp->component = NULL;
   univinfo->lopri = srcp->priority = 0;   // set priority to 0
   //LOG_FEND();
}
