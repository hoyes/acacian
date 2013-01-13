/**********************************************************************/
/*

Copyright (c) 2012, Engineering Arts (UK)

All rights reserved.

  $Id$

#tabs=3t
*/
/**********************************************************************/

#ifndef __e1_17_h__
#define __e1_17_h__ 1
/*
header: acnstd.h

Constants from ACN Standards

These constants represent requirements defined in standard documents of
*ANSI E1.17*
*/
/**********************************************************************/
/*
section: Protocol Identifiers

EPI16 defines the method for registration of ACN protocol 
identifiers including both numeric codes as used on the wire in ACN 
PDUs and text names as used in discovery. DDL also defines 
identifiers for different access protocols. Specific protocol 
identifiers are mostly defined in individual standard docs.

macros: Numeric protocol codes

ESTA_PROTOCOL_NONE - unspecified or no protocol
SDT_PROTOCOL_ID    - Session Data Transport (SDT section 7)
DMP_PROTOCOL_ID    - Device Management Protocol (DMP section 13)
E131_PROTOCOL_ID   - E1.31 "Streaming ACN" or "sACN" (E1.31 section 5.5)

macros: Protocol Names

SDT_PROTOCOL_NAME  - Session Data Transport (EPI-19)
DMP_PROTOCOL_NAME  - Device Management Protocol (EPI-19)
E131_PROTOCOL_NAME - E1.31 "Streaming ACN" or "sACN" (E1.31 section 5.5)

macros: DDL Access Protocol Identifiers

DMP_PROTOCOL_DDLNAME  - DMP (DDL appendix B)
E131_PROTOCOL_DDLNAME - sACN and DMX512 (EPI-26)

*/
/**********************************************************************/
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

/**********************************************************************/
/*
section: Constants from ACN Architecture

These constants represent requirements defined in standard document 
*ANSI E1.17 - 2010 Architecture for Control Networks – ACN Architecture*

macros: PDU flags

Flags for PDU flag and length field. These flags apply to the 
complete 16-bit flags and length field

LENGTH_FLAG  - Set if length > 4095 (can never be set if epi20 used)
VECTOR_FLAG  - if set vector is present
HEADER_FLAG  - if set header is present
DATA_FLAG    - if set data is present
LENGTH_MASK  - mask off flags leaving length
FLAG_MASK    - mask off length leaving flags

Sometimes we just want to apply flags to the first octet

LENGTH_bFLAG  - 8-bit equivalent of LENGTH_FLAG 
VECTOR_bFLAG  - 8-bit equivalent of VECTOR_FLAG 
HEADER_bFLAG  - 8-bit equivalent of HEADER_FLAG 
DATA_bFLAG    - 8-bit equivalent of DATA_FLAG   
LENGTH_bMASK  - 8-bit equivalent of LENGTH_MASK 
FLAG_bMASK    - 8-bit equivalent of FLAG_MASK   

*/
/* flag and length field is 16 bits */
#define LENGTH_FLAG    0x8000
#define VECTOR_FLAG    0x4000
#define HEADER_FLAG    0x2000
#define DATA_FLAG      0x1000
#define LENGTH_MASK    0x0fff
#define FLAG_MASK      0xf000

/*
first flags must be the same in any PDU block (assume LENGTH_FLAG is 0)
*/
#define FIRST_FLAGS (VECTOR_FLAG | HEADER_FLAG | DATA_FLAG)

/* sometimes we only want 8 bits */
#define LENGTH_bFLAG    0x80
#define VECTOR_bFLAG    0x40
#define HEADER_bFLAG    0x20
#define DATA_bFLAG      0x10
#define LENGTH_bMASK    0x0f
#define FLAG_bMASK      0xf0
#define FIRST_bFLAGS (VECTOR_bFLAG | HEADER_bFLAG | DATA_bFLAG)

/**********************************************************************/
#if defined(ACNCFG_SDT)
/*
section: SDT Constants

Constants from Session Data Transport

These constants represent requirements defined in standard document 
*ANSI E1.17 - 2010 Architecture for Control Networks – Session Data Transport*

enums: PDU vector codes

From SDT spec Table 3

SDT_REL_WRAP,
SDT_UNREL_WRAP,
SDT_CHANNEL_PARAMS,
SDT_JOIN,
SDT_JOIN_REFUSE,
SDT_JOIN_ACCEPT,
SDT_LEAVE,
SDT_LEAVING,
SDT_CONNECT,
SDT_CONNECT_ACCEPT,
SDT_CONNECT_REFUSE,
SDT_DISCONNECT,
SDT_DISCONNECTING,
SDT_ACK,
SDT_NAK,
SDT_GET_SESSIONS,
SDT_SESSIONS
*/
enum
{
  SDT_REL_WRAP        = 1,
  SDT_UNREL_WRAP      = 2,
  SDT_CHANNEL_PARAMS  = 3,
  SDT_JOIN            = 4,
  SDT_JOIN_REFUSE     = 5,
  SDT_JOIN_ACCEPT     = 6,
  SDT_LEAVE           = 7,
  SDT_LEAVING         = 8,
  SDT_CONNECT         = 9,
  SDT_CONNECT_ACCEPT  = 10,
  SDT_CONNECT_REFUSE  = 11,
  SDT_DISCONNECT      = 12,
  SDT_DISCONNECTING   = 13,
  SDT_ACK             = 14,
  SDT_NAK             = 15,
  SDT_GET_SESSIONS    = 16,
  SDT_SESSIONS        = 17,
};

/*
enum: Reason codes

From SDT spec Table 6

SDT_REASON_NONSPEC,
SDT_REASON_PARAMETERS,
SDT_REASON_RESOURCES,
SDT_REASON_ALREADY_MEMBER,
SDT_REASON_BAD_ADDR,
SDT_REASON_NO_RECIPROCAL,
SDT_REASON_CHANNEL_EXPIRED,
SDT_REASON_LOST_SEQUENCE,
SDT_REASON_SATURATED,
SDT_REASON_ADDR_CHANGING,
SDT_REASON_ASKED_TO_LEAVE,
SDT_REASON_NO_RECIPIENT,
SDT_REASON_ONLY_UNICAST
*/
enum
{
  SDT_REASON_NONSPEC          = 1,
  SDT_REASON_PARAMETERS       = 2,
  SDT_REASON_RESOURCES        = 3,
  SDT_REASON_ALREADY_MEMBER   = 4,
  SDT_REASON_BAD_ADDR         = 5,
  SDT_REASON_NO_RECIPROCAL    = 6,
  SDT_REASON_CHANNEL_EXPIRED  = 7,
  SDT_REASON_LOST_SEQUENCE    = 8,
  SDT_REASON_SATURATED        = 9,
  SDT_REASON_ADDR_CHANGING    = 10,
  SDT_REASON_ASKED_TO_LEAVE   = 11,
  SDT_REASON_NO_RECIPIENT     = 12,
  SDT_REASON_ONLY_UNICAST     = 13,
};

/*
enum: Address specification types

From SDT spec Table 7

SDT_ADDR_NULL,
SDT_ADDR_IPV4,
SDT_ADDR_IPV6
*/
enum
{
  SDT_ADDR_NULL = 0,
  SDT_ADDR_IPV4 = 1,
  SDT_ADDR_IPV6 = 2,
};

/*
macros: miscellaneous flags and vlaues

NAK_OUTBOUND - Send NAKs to the multicast group as well as unicast to the leader
PARAM_FLAG_MASK - Mask to extract parameter flags
ALL_MEMBERS - MID value for PDUs addressed to all members
*/
#define NAK_OUTBOUND 0x80
#define PARAM_FLAG_MASK NAK_OUTBOUND
#define ALL_MEMBERS 0xffff

#endif  /* defined(ACNCFG_SDT) */

/**********************************************************************/
#if defined(ACNCFG_DMP)
/*
section: DMP Constants

Constants from Device Management Protocol

These constants represent requirements defined in standard document 
*ANSI E1.17 - 2010 Architecture for Control Networks – Device Managemenet Protocol*

macros: field lengths

DMP_VECTOR_LEN - Length of vector in DMP
DMP_HEADER_LEN - Length of header in DMP

macros: Address specification bits - in header field

Individual bit-field definitions
	DMPAD_A0 - A1 A0 specify the address size
	DMPAD_A1 - A1 A0 specify the address size
	DMPAD_X0 - Must be zero
	DMPAD_X1 - Must be zero
	DMPAD_D0 - D1 D0 give the address format
	DMPAD_D1 - D1 D0 give the address format
	DMPAD_R  - Address is relative to previous property
	DMPAD_Z  - Must be zero
	DMPAD_ZMASK - Combined mask of bits which must be zero

macros: Address types (combined from individual bits)
	DMPAD_SINGLE - A single property
	DMPAD_RANGE_NODATA - Range address, no property values (e.g. in get-property message)
	DMPAD_RANGE_SINGLE - Range address, single value for all properties in range
	DMPAD_RANGE_ARRAY - Redundant in ACN-2010 (applied to obsolete virtual addresses)
	DMPAD_RANGE_STRUCT - Range address, one value per property in range
	DMPAD_TYPEMASK - Mask to select just the address type from header field

macros: Address sizes (combined from individual bits)
	DMPAD_1BYTE - Address, and count and increment if range, are 1 byte each
	DMPAD_2BYTE - Address, and count and increment if range, are 2 bytes each
	DMPAD_4BYTE - Address, and count and increment if range, are 4 bytes each
	DMPAD_BADSIZE - Illegal value
	DMPAD_SIZEMASK - Mask to select just the address size from header field
	ADDR_SIZE(hdr) - Extract the address size (1,2 or 4) from the header field


*/

#define DMP_VECTOR_LEN 1
#define DMP_HEADER_LEN 1

enum {
	DMPAD_A0 = 0x01,
	DMPAD_A1 = 0x02,
	DMPAD_X0 = 0x04,
	DMPAD_X1 = 0x08,
	DMPAD_D0 = 0x10,
	DMPAD_D1 = 0x20,
	DMPAD_R  = 0x40,
	DMPAD_Z  = 0x80,
	/* these bits must be zero */
    DMPAD_ZMASK = (DMPAD_Z | DMPAD_X1 | DMPAD_X0)
};

enum
{
	DMPAD_SINGLE       = 0,
	DMPAD_RANGE_NODATA = 0x10,
	DMPAD_RANGE_SINGLE = 0x10,
	DMPAD_RANGE_ARRAY  = 0x20,
	DMPAD_RANGE_STRUCT = 0x30,
	DMPAD_TYPEMASK     = 0x30
};

enum {
	DMPAD_1BYTE = 0,
	DMPAD_2BYTE = 1,
	DMPAD_4BYTE = 2,
	DMPAD_BADSIZE = 3,
	DMPAD_SIZEMASK = 3
};

#define ADDR_SIZE(hdr) (((hdr) & ADDRESS_SIZE_MASK) + 1 + (((hdr) & ADDRESS_SIZE_MASK) == 2))

/*
enum: dmp_reason_e
Reason codes (From DMP spec)

DMPRC_SUCCESS - success
DMPRC_UNSPECIFIED - unspecified refusal or failure
DMPRC_NOSUCHPROP - property does not exist
DMPRC_NOREAD - property is not readable by Get-Property
DMPRC_NOWRITE - property is not writeable by Set-Property
DMPRC_BADDATA - "illegal" data value supplied
DMPRC_NOEVENT - property does not support event generation
DMPRC_NOSUBSCRIBE - device cannot accept subscriptions (does not generate events)
DMPRC_NORESOURCES - unspecified resource limit
DMPRC_NOPERMISSION - requester does not have permission for request

enum: dmp_message_e
DMP messsage vectors (commands)

DMP_GET_PROPERTY - Get-Property
DMP_SET_PROPERTY - Set-Property
DMP_GET_PROPERTY_REPLY - Get-Property reply
DMP_EVENT - Event
DMP_SUBSCRIBE - Eubscribe
DMP_UNSUBSCRIBE - Unsubscribe
DMP_GET_PROPERTY_FAIL - Get-Property fail
DMP_SET_PROPERTY_FAIL - Set-Property fail
DMP_SUBSCRIBE_ACCEPT - Subscribe accept
DMP_SUBSCRIBE_REJECT - Subscribe reject
DMP_SYNC_EVENT - Synchronization event
*/
enum dmp_reason_e
{
	DMPRC_SUCCESS = 0,
	DMPRC_UNSPECIFIED = 1,
	DMPRC_NOSUCHPROP = 2,
	DMPRC_NOREAD = 3,
	DMPRC_NOWRITE = 4,
	DMPRC_BADDATA = 5,
	DMPRC_NOEVENT = 10,
	DMPRC_NOSUBSCRIBE = 11,
	DMPRC_NORESOURCES = 12,
	DMPRC_NOPERMISSION = 13,
	DMPRC_MAXINC = 13
};

enum dmp_message_e
{
	DMP_reserved0             = 0,
	DMP_GET_PROPERTY          = 1,
	DMP_SET_PROPERTY          = 2,
	DMP_GET_PROPERTY_REPLY    = 3,
	DMP_EVENT                 = 4,
	DMP_reserved5             = 5,
	DMP_reserved6             = 6,
	DMP_SUBSCRIBE             = 7,
	DMP_UNSUBSCRIBE           = 8,
	DMP_GET_PROPERTY_FAIL     = 9,
	DMP_SET_PROPERTY_FAIL     = 10,
	DMP_reserved11            = 11,
	DMP_SUBSCRIBE_ACCEPT      = 12,
	DMP_SUBSCRIBE_REJECT      = 13,
	DMP_reserved14            = 14,
	DMP_reserved15            = 15,
	DMP_reserved16            = 16,
	DMP_SYNC_EVENT            = 17
};
#endif  /* defined(ACNCFG_DMP) */

/**********************************************************************/
#if defined(ACNCFG_EPI10)
/*
section: EPI-10 Constants

These constants represent requirements defined in standard document 
*ANSI E1.17-2010 Architecture for Control Networks
EPI 10.
Autogeneration of Multicast Address on IPv4
Networks*

macros: Multicast Autogeneration

All constants except EPI10_HOST_PART_MASK are defined in network byte order.


E1_17_AUTO_SCOPE_ADDRESS  - see epi10 for details
E1_17_AUTO_SCOPE_MASK    - see epi10 for details
E1_17_AUTO_SCOPE_BITS    - see epi10 for details

EPI10_SCOPE_MIN_MASK     - see epi10 for details
EPI10_SCOPE_MIN_BITS     - see epi10 for details
EPI10_SCOPE_MAX_MASK     - see epi10 for details
EPI10_SCOPE_MAX_BITS     - see epi10 for details

EPI10_HOST_PART_MASK  - see epi10 for details
*/
#include "acnip.h"

#define E1_17_AUTO_SCOPE_ADDRESS  DD2NIP( 239,192,0,0 )
#define E1_17_AUTO_SCOPE_MASK     DD2NIP( 255,252,0,0 )
#define E1_17_AUTO_SCOPE_BITS     14

#define EPI10_SCOPE_MIN_MASK      DD2NIP( 255,192,0,0 )
#define EPI10_SCOPE_MIN_BITS      10
#define EPI10_SCOPE_MAX_MASK      DD2NIP( 255,255,0,0 )
#define EPI10_SCOPE_MAX_BITS      16

/* Note EPI10_HOST_PART_MASK is not in network byte order */
#define EPI10_HOST_PART_MASK 0xff

#endif  /* defined(ACNCFG_EPI10) */

/**********************************************************************/
#if defined(ACNCFG_EPI17)
/*
section: Constants from EPI-17

These constants represent requirements defined in standard document 
*ANSI E1.17 - 2010
Architecture for Control Networks –
EPI 17. ACN Root Layer Protocol
Operation on UDP*

macros: Root Layer for UDP

RLP_PREAMBLE_LENGTH - Length of RLP preamble
RLP_POSTAMBLE_LENGTH - Length of RLP postamble

RLP_PREAMBLE_VALUE - string representation of RLP preamble (assumes compiler will add NUL terminator)

*/

#define RLP_PREAMBLE_LENGTH 16
#define RLP_POSTAMBLE_LENGTH 0

/* Note string below assumes a nul terminator will be added */
#define RLP_PREAMBLE_VALUE "\0\x10\0\0" "ASC-E1.17\0\0"

#endif  /* defined(ACNCFG_EPI17) */

/**********************************************************************/
#if defined(ACNCFG_EPI18)
/*
section: EPI-18 Constants

These constants represent requirements defined in standard document 
*ANSI E1.17-2010, Architecture for Control Networks
EPI 18.
Operation of SDT on UDP Networks*

macros: Constants from EPI18. SDT on UDP

Note:
These values and the method of specification changed between 
ACN-2006 and ACN-2010 with ACN-2010 defining several timeouts in 
terms of a timeout factor which relates the timeout to variable 
channel expiry time. Values are 
provided for both versions (controlled by <ACNCFG_VERSION>).

MAK_TIMEOUT_FACTOR - ACN-2010 method
MAK_TIMEOUT_ms - ACN-2006 only
MAK_MAX_RETRIES - both ACN-2006 and ACN-2010

AD_HOC_TIMEOUT_ms - both ACN-2006 and ACN-2010
AD_HOC_RETRIES - both ACN-2006 and ACN-2010

RECIPROCAL_TIMEOUT_FACTOR - ACN-2010 method

MIN_EXPIRY_TIME_s - ACN-2010 method
MIN_EXPIRY_TIME_ms - ACN-2006 method

NAK_TIMEOUT_FACTOR - ACN-2010 method
NAK_TIMEOUT_ms - ACN-2006 only
NAK_MAX_RETRIES - both ACN-2006 and ACN-2010
NAK_HOLDOFF_INTERVAL_ms - both ACN-2006 and ACN-2010
NAK_MAX_TIME(hldoff) - calculate based on holdoff
NAK_BLANKTIME(hldoff) - calculate based on holdoff
NAK_MAX_TIME_ms - based on EPI-18 suggested value for holdoff
NAK_BLANKTIME_ms - based on EPI-18 suggested value for holdoff
SDT_MULTICAST_PORT - ACN-2006 and ACN-2010


*/

#if ACNCFG_VERSION == 20060000

#define MAK_TIMEOUT_ms           200
#define MAK_MAX_RETRIES          3
#define AD_HOC_TIMEOUT_ms        200
#define AD_HOC_RETRIES           3
#define MIN_EXPIRY_TIME_ms       2000
#define NAK_TIMEOUT_ms           100
#define NAK_MAX_RETRIES          3
#define NAK_HOLDOFF_INTERVAL_ms  2
#define NAK_MAX_TIME(hldoff)        (10 * (hldoff))     /* x NAK_HOLDOFF_INTERVAL */
#define NAK_BLANKTIME(hldoff)       (3  * (hldoff))     /* x NAK_HOLDOFF_INTERVAL */
#define NAK_MAX_TIME_ms  NAK_MAX_TIME(NAK_HOLDOFF_INTERVAL_ms)
#define NAK_BLANKTIME_ms NAK_BLANKTIME(NAK_HOLDOFF_INTERVAL_ms)
#define SDT_MULTICAST_PORT       5568

#elif ACNCFG_VERSION >= 20100000

#define MAK_TIMEOUT_FACTOR          0.1
#define MAK_MAX_RETRIES             2        /* 3 tries total */
#define AD_HOC_TIMEOUT_ms           1000     //200      /* ms */
#define AD_HOC_RETRIES              2        /* 3 tries total */
#define RECIPROCAL_TIMEOUT_FACTOR   0.2
#define MIN_EXPIRY_TIME_s           2        /* s */
#define NAK_TIMEOUT_FACTOR          0.1
#define NAK_MAX_RETRIES             2        /* 3 tries total */
#define NAK_HOLDOFF_INTERVAL_ms     2        /* ms */
#define NAK_MAX_TIME(hldoff)        (10 * (hldoff))     /* x NAK_HOLDOFF_INTERVAL */
#define NAK_BLANKTIME(hldoff)       (3  * (hldoff))     /* x NAK_HOLDOFF_INTERVAL */
#define NAK_MAX_TIME_ms  NAK_MAX_TIME(NAK_HOLDOFF_INTERVAL_ms)
#define NAK_BLANKTIME_ms NAK_BLANKTIME(NAK_HOLDOFF_INTERVAL_ms)
#define SDT_MULTICAST_PORT          5568     /* IANA registered port "sdt" */

#else
#error Unknown ACN version
#endif
#endif  /* defined(ACNCFG_EPI18) */

/**********************************************************************/
#if defined(ACNCFG_EPI20)
/*
section: EPI-20 Constants

These constants represent requirements defined in standard document 
*ANSI E1.17 - 2010
Architecture for Control Networks –
EPI 20. Maximum Transmission Unit
(MTU) Size for ACN on IPv4 Networks*

macros: Constants from EPI20. Maximum Transmission Unit (MTU)

DEFAULT_MTU - default value
MAX_MTU - implementation maximum
*/

#define DEFAULT_MTU 1472
#define MAX_MTU 1472

#endif  /* defined(ACNCFG_EPI20) */

#endif   /* __e1_17_h__ */
