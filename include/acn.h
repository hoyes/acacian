/**********************************************************************/
/*
#tabs=3t

Copyright (c) 2007, Engineering Arts (UK)

*/
/**********************************************************************/
#ifndef _acn_h_
#define _acn_h_ 1

#include "acncfg.h"
#include "acnstdtypes.h"
#include "acncommon.h"

#include "acnstd.h"
#include "acnlists.h"

#include "acnip.h"

#include "acnlog.h"
#include "acnmem.h"

#if ACNCFG_EPI26
#include "dmxaccess.h"
#endif

#if ACNCFG_NET_IPV4 || ACNCFG_NET_IPV6
#include "netxface.h"
#endif
#include "uuid.h"
#include "marshal.h"
#include "helpers.h"
#if ACNCFG_EVLOOP
#include "evloop.h"
#endif
#if ACNCFG_NET_IPV4 || ACNCFG_NET_IPV6
#include "rxcontext.h"
#endif

#if ACNCFG_RLP
#include "rlp.h"
#endif

#if ACNCFG_SDT
#include "sdt.h"
#endif

#if ACNCFG_DMP && defined(ACNCFG_PROPEXT_TOKS)
#include "propext.h"
#endif
#if ACNCFG_DMP || ACNCFG_DDLACCESS_DMP
#include "dmpmap.h"
#endif

#if ACNCFG_DDL
#include "ddl/parse.h"
#include "ddl/behaviors.h"
#include "ddl/bvactions.h"
#include "ddl/resolve.h"
#endif

#if ACNCFG_DMP
#include "dmp.h"
#endif

#if ACNCFG_E131
#include "e131.h"
#endif

#if ACNCFG_EPI10
#include "mcastalloc.h"
#endif

#if ACNCFG_EPI19
#include "discovery.h"
#endif

#include "component.h"

#endif  /* _acn_h_ */
