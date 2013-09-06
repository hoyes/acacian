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
#define ACNCFG_LOGLEVEL LOG_INFO
//#define ACNCFG_LOGLEVEL LOG_DEBUG
#define ACNCFG_LOGFUNCS ((LOG_OFF) | LOG_DEBUG)

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

#define ACNCFG_MAPGEN 1
#define ACNCFG_ACNLOG ACNLOG_STDERR

#define ACNCFG_EXTENDTOKENS \
		_TOKEN_(TK_functiondata, "functiondata"), \
		_TOKEN_(TK_getfunction, "getfunction"), \
		_TOKEN_(TK_setfunction, "setfunction"), \
		_TOKEN_(TK_subscribefunction, "subscribefunction"),

#define ACNCFG_NUMEXTENDFIELDS 4

#endif

#endif  /* __acncfg_local_h__ */
