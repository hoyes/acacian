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

#ifndef __acncfg_mapgen_h__
#define __acncfg_mapgen_h__           1

/**********************************************************************/
/*
Device map generator - used to create map for device demo
*/
#define ACNCFG_MULTI_COMPONENT 0
#define ACNCFG_LOGLEVEL LOG_INFO
//#define ACNCFG_LOGLEVEL LOG_DEBUG
#define ACNCFG_LOGFUNCS ((LOG_OFF) | ACNCFG_LOGLEVEL)

#define ACNCFG_MAPGEN 1
#define ACNCFG_ACNLOG ACNLOG_STDERR

#define ACNCFG_EXTENDTOKENS \
		_EXTOKEN_(functiondata,      void *)

#define ACNCFG_NUMEXTENDFIELDS 1

#endif  /* __acncfg_mapgen_h__ */
