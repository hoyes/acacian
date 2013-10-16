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

#ifndef __acncfg_ddltree_h__
#define __acncfg_ddltree_h__           1

/**********************************************************************/
/*
Print DDL tree demo
*/

#define ACNCFG_MULTI_COMPONENT 0
#define ACNCFG_LOGLEVEL LOG_INFO
//#define ACNCFG_LOGLEVEL LOG_DEBUG
#define ACNCFG_LOGFUNCS ((LOG_OFF) | ACNCFG_LOGLEVEL)

#define ACNCFG_NET_IPV4 0
#define ACNCFG_NET_IPV6 0

#define ACNCFG_MAPGEN 1
#define ACNCFG_ACNLOG ACNLOG_STDERR

#define ACNCFG_EPI10   0
#define ACNCFG_EPI11   0
#define ACNCFG_EPI12   0
#define ACNCFG_EPI15   0
#define ACNCFG_EPI16   0
#define ACNCFG_EPI17   0
#define ACNCFG_EPI18   0
#define ACNCFG_EPI19   0
#define ACNCFG_EPI20   0
#define ACNCFG_EPI26   0
#define ACNCFG_EPI29   0

#define ACNCFG_EVLOOP  0
#define ACNCFG_RLP 0
#define ACNCFG_SDT 0
#define ACNCFG_DMP 0

#endif  /* __acncfg_ddltree_h__ */
