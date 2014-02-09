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
header: acn.h

Include all necessary ACN headers as determined by acncfg.h in order.

This should normally be the only Acacian header you need to include.
*/

#ifndef _acn_h_
#define _acn_h_ 1

#define ACACIAN

#include "acncfg.h"
#include "acnstdtypes.h"
#include "acncommon.h"

#include "acnstd.h"
#include "acnlists.h"

#include "acnip.h"

#include "acnlog.h"
#include "acnmem.h"

#if CF_EPI26
#include "dmxaccess.h"
#endif

#if CF_NET_IPV4 || CF_NET_IPV6
#include "netxface.h"
#endif
#include "uuid.h"
#include "marshal.h"
#include "random.h"
#if CF_EVLOOP
#include "evloop.h"
#endif
#if CF_NET_IPV4 || CF_NET_IPV6
#include "rxcontext.h"
#endif

#if CF_RLP
#include "rlp.h"
#endif

#if CF_SDT
#include "sdt.h"
#endif

#if CF_DMP && defined(CF_PROPEXT_TOKS)
#include "propext.h"
#endif
#if CF_DMP || CF_DDLACCESS_DMP
#include "dmpmap.h"
#endif

#if CF_DDL
#include "ddlparse.h"
#include "behaviors.h"
#include "resolve.h"
#endif

#if CF_DMP
#include "dmp.h"
#endif

#if CF_E131
#include "e131.h"
#endif

#if CF_EPI10
#include "mcastalloc.h"
#endif

#if CF_EPI19
#include "discovery.h"
#endif

#include "component.h"

#endif  /* _acn_h_ */
