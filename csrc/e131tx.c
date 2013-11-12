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

file: e131tx.c

Implementation of E1.31 treaming protocol (sACN)
*/
/*
#tabs=3s
*/

#include <unistd.h>
#include <errno.h>

#include "acn.h"

#define E131_OPEN_WAIT_TIMEOUT   2
#define E131_OPEN_WAIT_MAXTIMEOUT   120
#define E131_OPEN_WAIT_TRIES   0

#define E131_DEVICE 1
#define E131_CONTROLLER 0

/*************************************************************************
Implementation of priority arbitration in receivers

A limit of E131_MAX_SOURCES is applied per universe. This limit is also
the number of source components tracked so if a packet is rejected for
priority/arbitration reasons, no record is kept of the component which
sent it.

According to the spec, we must impose some sort of deterministic priority

When a packet is received for a universe with a priority P.

If P < lopri we dump the packet.
Otherwise we look up the source in our tracking list.
If it is known we:
   update activity timer for that source
   update priority for that source and if changed update lopri and hipri
   now if priority < hipri we dump the packet
   else pass packet up to app - don't merge or impose other algorithms
Unknown source
   if tracked components < E131_MAX_SOURCES add the new one
   else
      find tracked component with pri = lopri
      If its lopri is lower than the new one replace this tracked comp with
      the new one then
         if new priority >= hipri pass packet up to app - don't merge
      If lopri == hipri we have sources_exceeded condition call
      to inform app, set sources_exceeded timer and dump the packet.

When a tracked source disappears (timeout or stream_terminated) it is
removed from the tracked list. There may be a glitch as a new source is 
established but the above algorithm provides a good chance the next lowest
component will be tracked already.
*************************************************************************/

struct netxsocket_s *e131sock = NULL;   /* can do it all on one socket */
void e131_rx_callback(const uint8_t *datap, int datasize, void *ref, const netx_addr_t *remhost, const cid_t remcid);
void rx_timeout(struct timer_s *timer);

/*
   acn_protect_t  protect;
   protect = ACN_PORT_PROTECT();
   ACN_PORT_UNPROTECT(protect);
*/

#if (CONFIG_MEM == MEM_STATIC)

struct e131_univinfo_s univ_m[MAXUNIVS];
struct e131_univinfo_s *active_univs;
struct e131_univinfo_s *free_univs;

int e131_had_init = 0;

void e131_init_m(void)
{
   acn_protect_t  protect;
   e131_univinfo_s *univinfo, *lstinf;

   protect = ACN_PORT_PROTECT();
   active_univs = lstinf = NULL;
   univinfo = univ_m;
   while (univinfo < univ_m + MAXUNIVS) {
      univinfo->next = lstinf;
      lstinf = univinfo++;
   }
   free_univs = lstinf;
   ACN_PORT_UNPROTECT(protect);
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

#elif (CONFIG_MEM == MEM_MALLOC)

struct e131_univinfo_s *active_univs = NULL;

#define e131_init_m()

static inline struct e131_univinfo_s *new_univinfo(uint16_t universe)
{
   struct e131_univinfo_s *univinfo;
   univinfo = (struct e131_univinfo_s *)calloc(1, sizeof(struct e131_univinfo_s));
   if ((univinfo = (struct e131_univinfo_s *)calloc(1, sizeof(struct e131_univinfo_s))) == NULL)
      return NULL;
   univinfo->next = active_univs;
   active_univs = univinfo;
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

/*************************************************************************
*************************************************************************/
struct e131_univinfo_s *findunivinfo(uint16_t univ)
{
   struct e131_univinfo_s *univinfo;
   
   for (univinfo = active_univs; univinfo != NULL; univinfo = univinfo->next)
      if (univinfo->universe == univ) break;
   return univinfo;
}

/*************************************************************************
   Just open a socket for the local address/port in e131addr
   If e131addr is NULL default to inaddr_any and SDT_MULTICAST_PORT
   Try tries times - if tries is 0 repeat indefinitely
*************************************************************************/
struct netxsocket_s *e131_socket(localaddr_t *e131addr, int tries)
{
   int rslt;
   localaddr_t rxaddr;
   int wait;
   int try = 0;
   struct netxsocket_s *nsk;

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
   return nsk;
}

/*************************************************************************
   Register a receiver for a universe
*************************************************************************/
struct rlp_listener_s *unicast_listener;

struct e131_univinfo_s *e131_register_rx(uint16_t universe, localaddr_t *localaddr, struct e131_callback_s *app_callback, int tries)
{
   struct e131_univinfo_s *univinfo;
   struct e131_source_s *srcp;
   acn_protect_t  protect;

   if (universe == 0 || universe >= 64000) {
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
            ACN_PORT_UNPROTECT(protect);
            return NULL;
         }
         if (NULL == (unicast_listener = rlp_add_listener(e131sock, netx_GROUP_UNICAST,
                                                         E131_PROTOCOL_ID,
                                                         e131_rx_callback, NULL))) {
            free_univinfo(univinfo);
            rlp_close_netsocket(e131sock);
            e131sock = NULL;
            ACN_PORT_UNPROTECT(protect);
            return NULL;
         }
      }
      univinfo->universe = universe;
      univinfo->srcxcdtimer.action = NULL;   // Don't need callback
      univinfo->srcxcdtimer.userp = (void *)univinfo;
      for (srcp = univinfo->sources; srcp < univinfo->sources + E131_MAX_SOURCES; ++srcp) {
         srcp->rxtimer.action = &rx_timeout;
         srcp->rxtimer.userp = (void *)srcp;
      }

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
   ACN_PORT_UNPROTECT(protect);
   return univinfo;
}

/*************************************************************************
*************************************************************************/

void e131_deregister_rx(struct e131_univinfo_s *univinfo)
{
   struct component_s *comp;
   struct e131_source_s *srcp;
   acn_protect_t  protect;

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

   ACN_PORT_UNPROTECT(protect);
}

/*************************************************************************
   Check a source for priority etc
*************************************************************************/

struct e131_source_s *e131_check_source(
      struct e131_univinfo_s *univinfo,
      uint8_t priority,
      uint8_t option,
      const cid_t remcid,
      const netx_addr_t *remhost,
      const uint8_t *uacn)
{
   struct e131_source_s *srcp;
   struct e131_source_s *freesrcp;
   struct e131_source_s *losrcp;
   struct e131_source_s *mysrcp;
   struct component_s *comp;
   uint8_t hipri, lopri;

   if (priority < univinfo->lopri) return NULL;

   // search univinfo first
   // we aim to scan only once so save a free slot in case we need it,
   // or failing that, the slot with lowest priority
   // update the priority range as we go
   srcp = univinfo->sources;
   hipri = 0;
   lopri = 255;
   losrcp = srcp;
   mysrcp = freesrcp = NULL;
   for (; srcp < univinfo->sources + E131_MAX_SOURCES; ++srcp) {
      if ((comp = srcp->component) == NULL) {
         // spare record
         freesrcp = srcp;
         continue;
      }
      if (cidIsEqual(comp->cid, remcid)) {
         mysrcp = srcp;             // save a pointer
         if (srcp->priority == priority && !(option & E131_OPT_TERMINATE)) {
            hipri = univinfo->hipri;   // no priority change don't need to scan further
            lopri = univinfo->lopri;
            break;
         }
         continue;   // leave this one out of priority calculations for now
      }
      if (srcp->priority < lopri) { // check min priority
         lopri = srcp->priority;
         losrcp = srcp;
      }
      if (srcp->priority > hipri) hipri = srcp->priority;
   }
   if ((option & E131_OPT_TERMINATE)) {
      if (mysrcp) {
         // lost a tracked source
         if (univinfo->app_cb.notify_lostsrc)
            (*univinfo->app_cb.notify_lostsrc)(univinfo->universe, mysrcp->component, LOST_TERMINATE);
         comp_release(mysrcp->component);   // forget the component
         mysrcp->component = NULL;
         cancel_timer(&mysrcp->rxtimer);
         univinfo->lopri = mysrcp->priority = 0;   // set priority to 0
      }
      return NULL;
   }
   if (!mysrcp) {
      // not in our immediate list
      // if it is lower or equal to lowest priority (and not all equal), dump it
      if (priority <= lopri && priority < hipri) return NULL;

      // no - we need to track it
      // see if we already have the component or get a new one
      comp = comp_get_by_cid(remcid);
      if (comp == NULL) return NULL;   // out of mem or something

      // if we found a spare source record simply save it there
      if (freesrcp) {
         freesrcp->component = comp;
         freesrcp->priority = priority;
         mysrcp = freesrcp;
      } else if (priority > lopri) {
         // no spare sources - we're not the lowest so try and dump a lowest priority
         if (univinfo->app_cb.notify_lostsrc)
            (*univinfo->app_cb.notify_lostsrc)(univinfo->universe, losrcp->component, LOST_PRIORITY);
         comp_release(losrcp->component);   // forget that one
         losrcp->component = comp;          // use its slot for the new one
         losrcp->priority = priority;
      } else {
         // priority, lopri and hipri must all be the same so sources exceeded
         if (univinfo->app_cb.notify_srcxcd && !is_active(&univinfo->srcxcdtimer))
            (*univinfo->app_cb.notify_srcxcd)(univinfo->universe, comp, priority);
         set_timeout(&univinfo->srcxcdtimer, E131_SRCXCD_TIMEOUT);
         comp_release(comp);
         return NULL;
      }
      // tracking a new component
      if (comp->created_by == cbNONE) comp->created_by = cbE131;
      if (*uacn) strncpy(comp->uacn, uacn, E131_UACN_SIZE);
      if (univinfo->app_cb.notify_newsrc)
         (*univinfo->app_cb.notify_newsrc)(univinfo->universe, comp, priority);
   }
   univinfo->hipri = hipri;
   univinfo->lopri = lopri;
   set_timeout(&mysrcp->rxtimer, E131_UNIV_TIMEOUT);
   if (is_active(&univinfo->srcxcdtimer)) return NULL;
   return mysrcp;
}

/*************************************************************************
   Receive and check an E131 packet
*************************************************************************/
const uint8_t dmpmatch[] = {0x02, 0xa1, 0x00, 0x00, 0x00, 0x01};

void e131_rx_callback(const uint8_t *datap, int datasize, void *ref, const netx_addr_t *remhost, const cid_t remcid)
{
   struct e131_univinfo_s *univinfo = (struct e131_univinfo_s *)ref;
   uint32_t u;
   uint16_t len;
   uint16_t univ;
   uint8_t  pri;
   uint8_t  opt;
   struct e131_source_s *remote;

   // check the E1.31 packet - quick tests likely to fail first to eliminate bad packets ASAP
   // if len and flags are wrong we can't trust anything else
   len = unmarshalU16(datap);
   if ((len & FLAG_MASK) != (VECTOR_FLAG | HEADER_FLAG | DATA_FLAG)) {
      acnlog(LOG_ERR | LOG_E131, "e131_rx_callback: framing layer: bad flags %" PRIu16 "x", len & FLAG_MASK);
      return;
   }
   if ((len & LENGTH_MASK) != datasize) {
      acnlog(LOG_ERR | LOG_E131, "e131_rx_callback: framing layer: bad length %" PRIu16 "u", len & LENGTH_MASK);
      return;
   }
   // if protocol is wrong we can't trust anything else
   if ((u = unmarshalU32(datap + E131_FLO_VECTOR)) != DMP_PROTOCOL_ID) {
      acnlog(LOG_ERR | LOG_E131, "e131_rx_callback: framing layer: bad vector %" PRIu32 "u", u);
      return;
   }
   univ = unmarshalU16(datap + E131_FLO_UNIVERSE);
   if (univinfo == NULL) { // must be unicast - need to find the right universe
      if ((univinfo = findunivinfo(univ)) == NULL) return;
   } else if (univ != univinfo->universe) {
      // likely to receive wrong universes if STACK_RETURNS_DEST_ADDR is false. Check anyway
      acnlog(LOG_INFO | LOG_E131, "e131_rx_callback: universe mismatch");
      return;
   }
   // Now check dmp layer
   // DMP layer flags and length
   len = unmarshalU16(datap + E131_DLO_LENGTH);
   if ((len & FLAG_MASK) != (VECTOR_FLAG | HEADER_FLAG | DATA_FLAG)) {
      acnlog(LOG_ERR | LOG_E131, "e131_rx_callback: DMP layer: bad flags %" PRIu16 "x", len & FLAG_MASK);
      return;
   } 
   if ((len & LENGTH_MASK) != (datasize - E131_FLO_DMP)) {
      acnlog(LOG_ERR | LOG_E131, "e131_rx_callback: DMP layer: bad length %" PRIu16 "u", len & LENGTH_MASK);
      return;
   }
   // rest of DMP pdu up to count
   if (memcmp(datap + E131_DLO_VECTOR, dmpmatch, sizeof(dmpmatch)) != 0) {
      acnlog(LOG_ERR | LOG_E131, "e131_rx_callback: DMP layer: bad format "
                        "%02" PRIu8 "x, %02" PRIu8 "x, %" PRIu16 "u, %" PRIu16 "u",
                        datap[E131_DLO_VECTOR], datap[E131_DLO_ADDRTYPE],
                        unmarshalU16(datap + E131_DLO_ADDR),
                        unmarshalU16(datap + E131_DLO_INC));
      return;
   }
   // dmp count
   if ((len = unmarshalU16(datap + E131_DLO_COUNT)) != (datasize - E131_FLO_DMP - E131_DLO_START)) {
      acnlog(LOG_ERR | LOG_E131, "e131_rx_callback: DMP layer: bad count %" PRIu16 "u", len);
      return;
   }
   // looks like a good packet
   // get its priority
   pri = datap[E131_FLO_PRIORITY];
   // now perform source tracking and prioritizing
   remote = e131_check_source(univinfo, pri, datap[E131_FLO_OPTIONS],
                              remcid, remhost, datap + E131_FLO_UACN);
   if (remote == NULL) {
#if acntestlog(LOG_DEBUG | LOG_E131)
      char  cid_text[CID_STR_SIZE];
#endif
      acnlog(LOG_DEBUG | LOG_E131, "e131_rx_callback: reject source %s", cidToText(remcid, cid_text));
      return;
   }
   // OK we have a packet for the app
   if (univinfo->app_cb.notify_data)
      (* univinfo->app_cb.notify_data)(univ, remote->component, pri, datap[E131_FLO_OPTIONS],
                                          datap + E131_DLO_START, len);
}

void rx_timeout(struct timer_s *timer)
{
   struct e131_univinfo_s *univinfo;
   struct e131_source_s *srcp;


   univinfo = (struct e131_univinfo_s *)timer->userp;
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
}
