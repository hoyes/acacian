/**********************************************************************/
/*
Copyright (c) 2013, Philip Nye
All rights reserved.

#tabs=3t
*/
/**********************************************************************/
/*
Do not include this file directly use:
#include "acncfg.h"

*/
/**********************************************************************/

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
