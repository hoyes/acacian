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
header: mkcfg.h

Header used co generate <prog>-cfg.mak summary of configuration
in Makefile format.

note:
This header is processed by the C preprocessor but is not compiled and
is not valic C code.
*/

#include "acn.h"

@_CF_VERSION CF_VERSION
@_CF_OS_LINUX CF_OS_LINUX
@_CF_NET_IPV4 CF_NET_IPV4
@_CF_NET_IPV6 CF_NET_IPV6
@_CF_MAX_IPADS CF_MAX_IPADS
@_CF_LOCALIP_ANY CF_LOCALIP_ANY
@_CF_MULTICAST_TTL CF_MULTICAST_TTL
@_CF_JOIN_TX_GROUPS CF_JOIN_TX_GROUPS
@_CF_RECEIVE_DEST_ADDRESS RECEIVE_DEST_ADDRESS


/*
@_CF_ACNLOG_OFF ACNLOG_OFF
@_CF_ACNLOG_SYSLOG ACNLOG_SYSLOG
@_CF_ACNLOG_STDOUT ACNLOG_STDOUT
@_CF_ACNLOG_STDERR ACNLOG_STDERR
@_CF_LOG_OFF LOG_OFF
@_CF_LOG_ON LOG_ON
@_CF_ACNLOG CF_ACNLOG
@_CF_LOGLEVEL CF_LOGLEVEL
@_CF_LOGFUNCS CF_LOGFUNCS
*/

#if CF_ACNLOG == ACNLOG_OFF
@_CF_LOGLEVEL off
#elif CF_ACNLOG == ACNLOG_SYSLOG
@_CF_LOGLEVEL syslog
#elif CF_ACNLOG == ACNLOG_STDOUT
@_CF_LOGLEVEL stdout
#elif CF_ACNLOG == ACNLOG_STDERR
@_CF_LOGLEVEL stderr
#else
@_CF_LOGLEVEL unknown (CF_ACNLOG)
#endif
#if CF_LOGFUNCS
@_CF_LOGFUNCS On
#else
@_CF_LOGFUNCS Off
#endif

#if CF_LOG_RLP < 0
@_CF_LOG_RLP Off
#else
@_CF_LOG_RLP On
#endif
#if CF_LOG_SDT < 0
@_CF_LOG_SDT Off
#else
@_CF_LOG_SDT On
#endif
#if CF_LOG_NETX < 0
@_CF_LOG_NETX Off
#else
@_CF_LOG_NETX On
#endif
#if CF_LOG_DMP < 0
@_CF_LOG_DMP Off
#else
@_CF_LOG_DMP On
#endif
#if CF_LOG_DDL < 0
@_CF_LOG_DDL Off
#else
@_CF_LOG_DDL On
#endif
#if CF_LOG_MISC < 0
@_CF_LOG_MISC Off
#else
@_CF_LOG_MISC On
#endif
#if CF_LOG_EVLOOP < 0
@_CF_LOG_EVLOOP Off
#else
@_CF_LOG_EVLOOP On
#endif
#if CF_LOG_E131 < 0
@_CF_LOG_E131 Off
#else
@_CF_LOG_E131 On
#endif
#if CF_LOG_APP < 0
@_CF_LOG_APP Off
#else
@_CF_LOG_APP On
#endif
#if CF_LOG_SESS < 0
@_CF_LOG_SESS Off
#else
@_CF_LOG_SESS On
#endif

@_CF_MARSHAL_INLINE CF_MARSHAL_INLINE
@_CF_STRICT_CHECKS CF_STRICT_CHECKS
@_CF_MULTI_COMPONENT CF_MULTI_COMPONENT
@_CF_ACN_FCTN_SIZE ACN_FCTN_SIZE
@_CF_ACN_UACN_SIZE ACN_UACN_SIZE

#if CF_UUIDS_RADIX
@_CF_UUIDS_RADIX 1
#else
@_CF_UUIDS_RADIX 0
#endif
#if CF_UUIDS_HASH
@_CF_UUIDS_HASH 1
@_CF_R_HASHBITS CF_R_HASHBITS
@_CF_L_HASHBITS CF_L_HASHBITS
#else
@_CF_UUIDS_HASH 0
#endif

@_CF_EVLOOP CF_EVLOOP

#if CF_TIME_ms
@_CF_TIME TIME_ms
#elif CF_TIME_POSIX_timeval
@_CF_TIME POSIX_timeval
#elif CF_TIME_POSIX_timespec
@_CF_TIME POSIX_timespec
#else
@_CF_TIME unknown
#endif

#if CF_RLP
@_CF_RLP 1
@_CF_RLP_MAX_CLIENT_PROTOCOLS CF_RLP_MAX_CLIENT_PROTOCOLS
#if CF_RLP_MAX_CLIENT_PROTOCOLS == 1
#if CF_RLP_CLIENTPROTO == SDT_PROTOCOL_ID
@_CF_RLP_CLIENTPROTO SDT
#elif CF_RLP_CLIENTPROTO == E131_PROTOCOL_ID
@_CF_RLP_CLIENTPROTO E1.31
#else
@_CF_RLP_CLIENTPROTO unknown (CF_RLP_CLIENTPROTO)
#endif
#endif
@_CF_RLP_OPTIMIZE_PACK CF_RLP_OPTIMIZE_PACK
#else
@_CF_RLP 0
#endif

#if CF_SDT
@_CF_SDT 1
@_CF_SDT_MAX_CLIENT_PROTOCOLS CF_SDT_MAX_CLIENT_PROTOCOLS
@_CF_SDTRX_AUTOCALL CF_SDTRX_AUTOCALL
@_CF_SDT_CHECK_ASSOC CF_SDT_CHECK_ASSOC
#if CF_SDT_MAX_CLIENT_PROTOCOLS == 1
@_CF_SDT_CLIENTPROTO CF_SDT_CLIENTPROTO
#if CF_SDT_CLIENTPROTO == DMP_PROTOCOL_ID
@_CF_SDT_CLIENTPROTO DMP
#else
@_CF_SDT_CLIENTPROTO unknown (CF_SDT_CLIENTPROTO)
#endif
#endif
#else
@_CF_SDT 0
#endif

#if CF_DMP
@_CF_DMP 1
@_CF_DMP_MAX_SUBSCRIPTIONS DMP_MAX_SUBSCRIPTIONS
@_CF_DMPCOMP_CD CF_DMPCOMP_CD
@_CF_DMPCOMP_C_ CF_DMPCOMP_C_
@_CF_DMPCOMP__D CF_DMPCOMP__D

#if CF_DMPCOMP_Cx
@_CF_DMPCOMP_Cx 1
#else
@_CF_DMPCOMP_Cx 0
#endif
#if CF_DMPCOMP_xD
@_CF_DMPCOMP_xD 1
#else
@_CF_DMPCOMP_xD 0
#endif


@_CF_DMPAD_MAXBYTES CF_DMPAD_MAXBYTES
@_CF_DMPMAP_INDEX CF_DMPMAP_INDEX
@_CF_DMPMAP_SEARCH CF_DMPMAP_SEARCH
@_CF_DMPMAP_NONE CF_DMPMAP_NONE

#if CF_DMPON_SDT
@_CF_DMPON_SDT 1
#else
@_CF_DMPON_SDT 0
#endif
#if CF_DMPON_TCP
@_CF_DMPON_TCP 1
#else
@_CF_DMPON_TCP 0
#endif
#if CF_DMP_MULTITRANSPORT
@_CF_DMP_MULTITRANSPORT 1
#else
@_CF_DMP_MULTITRANSPORT 0
#endif

@_CF_DMP_RMAXCXNS CF_DMP_RMAXCXNS
@_CF_PROPEXT_FNS CF_PROPEXT_FNS
#else
@_CF_DMP 0
#endif

#if CF_DDL
@_CF_DDL 1
@_CF_EXPAT_BUILTIN CF_EXPAT_BUILTIN
@_CF_DDLACCESS_DMP CF_DDLACCESS_DMP
@_CF_DDLACCESS_EPI26 CF_DDLACCESS_EPI26
@_CF_DDL_BEHAVIORS CF_DDL_BEHAVIORS
@_CF_DDL_IMMEDIATEPROPS CF_DDL_IMMEDIATEPROPS
@_CF_DDL_STRINGS CF_DDL_STRINGS
@_CF_DDL_MAXNEST CF_DDL_MAXNEST
@_CF_STR_FOLDSPACE CF_STR_FOLDSPACE
@_CF_DDL_MAXTEXT CF_DDL_MAXTEXT
@_CF_MAPGEN CF_MAPGEN
#else
@_CF_DDL 0
#endif

#if CF_E131
@_CF_E131 1
@_CF_E131_RX CF_E131_RX
@_CF_E131_TX CF_E131_TX
@_CF_E131MEM_MAXUNIVS E131MEM_MAXUNIVS
@_CF_E131_ZSTART_ONLY CF_E131_ZSTART_ONLY
@_CF_E131_IGNORE_PREVIEW CF_E131_IGNORE_PREVIEW
#else
@_CF_E131 0
#endif
@_CF_EPI10 CF_EPI10
@_CF_EPI11 CF_EPI11
@_CF_EPI12 CF_EPI12
@_CF_EPI15 CF_EPI15
@_CF_EPI16 CF_EPI16
@_CF_EPI17 CF_EPI17
@_CF_EPI18 CF_EPI18
@_CF_EPI19 CF_EPI19
@_CF_EPI20 CF_EPI20
@_CF_EPI26 CF_EPI26
@_CF_EPI29 CF_EPI29