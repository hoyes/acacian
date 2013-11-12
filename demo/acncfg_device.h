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

header: acncfg_device.h

Configuration for ACN device demo.

Do not include this file directly in source code use:
#include "acn.h"
*/

#ifndef __acncfg_device_h__
#define __acncfg_device_h__           1

/**********************************************************************/
/*
Demo Device
*/

#define ACNCFG_MULTI_COMPONENT 0
#define ACNCFG_ACNLOG ACNLOG_STDERR
//#define ACNCFG_LOGLEVEL LOG_INFO
#define ACNCFG_LOGLEVEL LOG_DEBUG
#define ACNCFG_LOGFUNCS ((LOG_ON) | ACNCFG_LOGLEVEL)

#define ACNCFG_DMPCOMP__D 1

/*
Warning: This definition probably needs to match acncfg_mapgen.h
Warning: ACNCFG_NUMEXTENDFIELDS needs to match ACNCFG_PROPEXT_TOKS
Warning: Tokens must be in lexical order
*/
#define ACNCFG_PROPEXT_TOKS \
		_EXTOKEN_(fn_getprop,     dmprx_fn *) \
		_EXTOKEN_(fn_setprop,     dmprx_fn *) \
		_EXTOKEN_(fn_subscribe,   dmprx_fn *) \
		_EXTOKEN_(fn_unsubscribe, dmprx_fn *) \
		_EXTOKEN_(propdata,       void *) \

#define ACNCFG_NUMEXTENDFIELDS 5
#define ACNCFG_PROPEXT_FNS 1

#endif  /* __acncfg_device_h__ */
