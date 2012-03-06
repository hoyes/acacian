/************************************************************************/
/*
#tabs=3t

Copyright (c) 2007, Engineering Arts (UK)

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

 * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
 * Neither the name of Engineering Arts nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  $Id$

*/
/************************************************************************/
/*
#tabs=3t
*/
#ifndef _acn_h_
#define _acn_h_ 1

#include "acncommon.h"
#include "acnlog.h"
#include "acnmem.h"
#include "netxface.h"
#include "marshal.h"
#include "uuid.h"
#include "acntimer.h"

#if CONFIG_EPI10
#include "acnstd/epi10.h"
#endif

#if CONFIG_EPI11
#include "acnstd/epi11.h"
#endif

#if CONFIG_EPI12
#include "acnstd/epi12.h"
#endif

#if CONFIG_EPI15
#include "acnstd/epi15.h"
#endif

#if CONFIG_EPI16
#include "acnstd/epi16.h"
#endif

#if CONFIG_EPI17
#include "acnstd/epi17.h"
#endif

#if CONFIG_EPI18
#include "acnstd/epi18.h"
#endif

#if CONFIG_EPI19
#include "acnstd/epi19.h"
#endif

#if CONFIG_EPI20
#include "acnstd/epi20.h"
#endif

#if CONFIG_EPI29
#include "acnstd/epi29.h"
#endif

#if CONFIG_RLP
#include "rlp.h"
#endif

#if CONFIG_SLP
#include "slp.h"
#endif

#if CONFIG_NET_TCP
#include "tcp.h"
#endif

#if CONFIG_DMP
#include "dmpccs.h"
#endif

#if CONFIG_E131
#include "e131.h"
#endif

#if CONFIG_DDL
#include "ddl.h"
#endif

#include "component.h"

#endif  /* _acn_h_ */
