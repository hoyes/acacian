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
#define ACNCFG_LOGFUNCS (LOG_OFF | ACNCFG_LOGLEVEL)

#define ACNCFG_DMPCOMP_C_ 1

#endif  /* __acncfg_controller_h__ */
