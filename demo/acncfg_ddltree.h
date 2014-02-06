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
header: acncfg_ddltree.h

Configuration for DDL tree print demo.

Do not include this file directly in source code use:
#include "acn.h"
*/

#ifndef __acncfg_ddltree_h__
#define __acncfg_ddltree_h__           1

/**********************************************************************/
/*
Print DDL tree demo
*/

#define CF_MULTI_COMPONENT 0
#define CF_LOGLEVEL LOG_NOTICE
//#define CF_LOGLEVEL LOG_INFO
//#define CF_LOGLEVEL LOG_DEBUG
#define CF_LOGFUNCS ((LOG_OFF) | CF_LOGLEVEL)

#define CF_NET_IPV4 0
#define CF_NET_IPV6 0

#define CF_ACNLOG ACNLOG_STDERR
#define CF_STR_FOLDSPACE 1

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

#endif  /* __acncfg_ddltree_h__ */
