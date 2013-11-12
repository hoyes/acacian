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

header: acncfg_local.h

Local configuratrion options

Do not include this file directly in source code use:
#include "acn.h"
*/

#ifndef __acncfg_local_h__
#define __acncfg_local_h__           1

/**********************************************************************/
/*
These configs depend on which demo we are building
*/

#if defined(device_demo)
#include "acncfg_device.h"
#elif defined(controller_demo)
#include "acncfg_controller.h"
#elif defined(ddl_tree)
#include "acncfg_ddltree.h"
#elif defined(mapgen)
#include "acncfg_mapgen.h"
#endif

#endif  /* __acncfg_local_h__ */
