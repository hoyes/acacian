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
#include "acnstd/arch.h"
#include "acncommon.h"
#include "acnlists.h"

#include "acnip.h"
#include "e1.17.h"

#include "acnlog.h"
#include "acnmem.h"

#if CONFIG_EPI26
#include "dmxaccess.h"
#endif

#if (CONFIG_STACK_BSD || CONFIG_STACK_CYGWIN)
#include "netx_bsd.h"
#elif CONFIG_STACK_LWIP
#include "netx_lwip.h"
#elif CONFIG_STACK_PATHWAY
#include "netx_pathway.h"
#elif CONFIG_STACK_NETBURNER
#include "netx_netburner.h"
#elif CONFIG_STACK_WIN32
#include "netx_win32.h"
#endif
#include "netxface.h"

#include "uuid.h"
#include "marshal.h"
#include "helpers.h"
#if CONFIG_ACN_EVLOOP
#include "evloop.h"
#endif
#include "rxcontext.h"

#if CONFIG_RLP
#include "rlp.h"
#endif

#if CONFIG_SLP
#include "acnstd/slp.h"
#include "slp.h"
#endif

#if CONFIG_SDT
#include "sdt.h"
#endif

#if CONFIG_DMP
#include "dmpmap.h"
#endif

#if CONFIG_DDL
#include "ddl/parse.h"
#include "ddl/behaviors.h"
#include "ddl/resolve.h"
#endif

#if CONFIG_DMP
#include "dmp.h"
#endif

#if CONFIG_E131
#include "e131.h"
#endif

#if CONFIG_EPI10
#include "mcastalloc.h"
#endif

#include "component.h"

#endif  /* _acn_h_ */
