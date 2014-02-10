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
header: acncfg_mapgen.h

Configuration for DDL to map generator.

Do not include this file directly in source code use:
#include "acn.h"
*/

#ifndef __acncfg_mapgen_h__
#define __acncfg_mapgen_h__           1

/**********************************************************************/
/*
Device map generator - used to create map for device demo
*/
#define CF_MULTI_COMPONENT 0
//#define CF_LOGLEVEL LOG_INFO
#define CF_LOGLEVEL LOG_DEBUG
#define CF_LOGFUNCS ((LOG_OFF) | CF_LOGLEVEL)

#define CF_MAPGEN 1
#define CF_ACNLOG ACNLOG_STDERR

#define CF_NET_IPV4 0
#define CF_NET_IPV6 0

#define CF_MAPGEN 1
#define CF_ACNLOG ACNLOG_STDERR

#define CF_EPI10   0
#define CF_EPI11   0
#define CF_EPI12   0
#define CF_EPI15   0
#define CF_EPI16   0
#define CF_EPI17   0
#define CF_EPI18   0
#define CF_EPI19   0
#define CF_EPI20   0
#define CF_EPI26   0
#define CF_EPI29   0

#define CF_EVLOOP  0
#define CF_RLP 0
#define CF_SDT 0
#define CF_DMP 0

/*
Warning: This definition probably needs to match acncfg_device.h
Warning: CF_NUMEXTENDFIELDS needs to match CF_PROPEXT_TOKS
Warning: Tokens must be in lexical order
*/

#define CF_PROPEXT_TOKS \
		_EXTOKEN_(fn_getprop,     dmprx_fn *) \
		_EXTOKEN_(fn_setprop,     dmprx_fn *) \
		_EXTOKEN_(fn_subscribe,   dmprx_fn *) \
		_EXTOKEN_(fn_unsubscribe, dmprx_fn *) \
		_EXTOKEN_(propdata,       void *) \

#define CF_NUMEXTENDFIELDS 5

#endif  /* __acncfg_mapgen_h__ */
