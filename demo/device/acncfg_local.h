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

#define CF_ACNLOG ACNLOG_STDERR
#define CF_LOGLEVEL LOG_INFO
//#define CF_LOGLEVEL LOG_DEBUG
#define CF_LOGFUNCS ((LOG_OFF) | CF_LOGLEVEL)
#define LOG_SDT LOG_ON

#define CF_MULTI_COMPONENT 0
#define CF_DDL 0
#define CF_DMPCOMP__D 1

/*
Warning: This definition probably needs to match acncfg_mapgen.h
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
#define CF_PROPEXT_FNS 1

#endif  /* __acncfg_device_h__ */
