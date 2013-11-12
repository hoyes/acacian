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
header: acncfg_controller.h

Configuration for controller demo.

Do not include this file directly in source code use:
#include "acn.h"

*/

#ifndef __acncfg_controller_h__
#define __acncfg_controller_h__           1

/**********************************************************************/
/*
Demo controller
*/
#define ACNCFG_MULTI_COMPONENT 0
#define ACNCFG_ACNLOG ACNLOG_STDERR
//#define ACNCFG_LOGLEVEL LOG_INFO
#define ACNCFG_LOGLEVEL LOG_DEBUG
#define ACNCFG_LOGFUNCS (LOG_ON | ACNCFG_LOGLEVEL)

#define ACNCFG_DMPCOMP_C_ 1

#endif  /* __acncfg_controller_h__ */
