/************************************************************************/
/*
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

#tabs=3s
*/
/************************************************************************/

#ifndef __acnstd_protocols_h__
#define __acnstd_protocols_h__ 1

/************************************************************************/
/*
macros: Protocol Identifiers

ESTA registered protocol codes and names

EPI16 defines the method for registration. These are currently registered 
protocols collected from a number of specifications including:
 - SDT   section 7
 - DMP   section 13
 - e1.31 section 5.5
 - epi26
 - epi19

Numeric protocol codes:

ESTA_PROTOCOL_NONE - unspecified or no protocol
SDT_PROTOCOL_ID    - Session Data Transport
DMP_PROTOCOL_ID    - Device Management Protocol
E131_PROTOCOL_ID   - E1.31 "Streaming ACN" (sACN)

Protocol Names as used in Discovery and Elsewhere:
SDT_PROTOCOL_NAME  - Session Data Transport
DMP_PROTOCOL_NAME  - Device Management Protocol
E131_PROTOCOL_NAME - E1.31 "Streaming ACN" (sACN)

DDL Access Protocol Identifiers:
DMP_PROTOCOL_DDLNAME  - DMP as in DDL Specification Appendix B
E131_PROTOCOL_DDLNAME - sACN and DMX512 as defined in EPI-26

*/
/************************************************************************/
/*
ID zero is unused
*/
typedef uint32_t protocolID_t;

#define ESTA_PROTOCOL_NONE  0

#define SDT_PROTOCOL_ID     1
#define SDT_PROTOCOL_NAME   "esta.sdt"

#define DMP_PROTOCOL_ID     2
#define DMP_PROTOCOL_NAME   "esta.dmp"
#define DMP_PROTOCOL_DDLNAME  "ESTA.DMP"

#define E131_PROTOCOL_ID    4
#define E131_PROTOCOL_NAME  "esta.e1.31"
#define E131_PROTOCOL_DDLNAME  "ESTA.EPI26"

#endif  /* __acnstd_protocols_h__ */
