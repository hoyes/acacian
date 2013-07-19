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


#define ACNCFG_MULTI_COMPONENT 0
#define ACNCFG_LOGLEVEL LOG_DEBUG

/**********************************************************************/
/*
These configs depend on which demo we are building
*/

#if defined(device_demo)

#define ACNCFG_DMP_DEVICE 1
#define ACNCFG_DMP_CONTROLLER 0

#elif defined(controller_demo)

#define ACNCFG_DMP_DEVICE 0
#define ACNCFG_DMP_CONTROLLER 1

#elif defined(mapgen) || defined(ddl_tree)

#define ACNCFG_ACNLOG ACNLOG_STDERR

#endif

#endif  /* __acncfg_local_h__ */
