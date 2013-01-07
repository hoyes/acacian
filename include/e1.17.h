/**********************************************************************/
/*

Copyright (c) 2012, Engineering Arts (UK)

All rights reserved.

  $Id$

*/
/**********************************************************************/

#ifndef __e1_17_h__
#define __e1_17_h__ 1
/*
file: e1.17.h

Constants from ACN Standards

These constants represent requirements defined in standard documents of
*ANSI E1.17*
*/

/*
Constants from ACN Architecture

These constants represent requirements defined in standard document 
*ANSI E1.17 - 2010 Architecture for Control Networks – ACN Architecture*
*/

/*
macros: PDU flags

Flags for PDU flag and length field

These flags apply to the complete 16-bit flags and length field

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
/* first flags must be the same in any PDU block (assume LENGTH_FLAG is 0) */
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
macros: SDT Constants

Constants from Session Data Transport

These constants represent requirements defined in standard document 
*ANSI E1.17 - 2010 Architecture for Control Networks – Session Data Transport*

*/

/* PDU vector codes [SDT spec Table 3] */
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

/* Reason codes [SDT spec Table 6] */
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

/* Address specification types [SDT spec Table 7] */
enum
{
  SDT_ADDR_NULL = 0,
  SDT_ADDR_IPV4 = 1,
  SDT_ADDR_IPV6 = 2,
};

/* Miscellaneous definitions from the spec */
/* NAK flag */
#define NAK_OUTBOUND 0x80
#define PARAM_FLAG_MASK NAK_OUTBOUND

/* All members address */
#define ALL_MEMBERS 0xffff

/**********************************************************************/
/*
macros: DMP Constants

Constants from Device Management Protocol

These constants represent requirements defined in standard document 
*ANSI E1.17 - 2010 Architecture for Control Networks – Device Managemenet Protocol*
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

#define ADDR_SIZE(adtype) (((adtype) & ADDRESS_SIZE_MASK) + 1 + (((adtype) & ADDRESS_SIZE_MASK) == 2))

/* Reason codes [DMP spec] */
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

/* DMP messsage types (commands) */
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
/**********************************************************************/
/*
macros: EPI-10 Constants

Constants from EPI10. Multicast Autogeneration

These constants represent requirements defined in standard document 
*ANSI E1.17-2010 Architecture for Control Networks
EPI 10.
Autogeneration of Multicast Address on IPv4
Networks*

These constants are defined in network byte order.

E1_17_AUTO_SCOPE_ADDRESS  - see epi10 for details
E1_17_AUTO_SCOPE_MASK    - see epi10 for details
E1_17_AUTO_SCOPE_BITS    - see epi10 for details

EPI10_SCOPE_MIN_MASK     - see epi10 for details
EPI10_SCOPE_MIN_BITS     - see epi10 for details
EPI10_SCOPE_MAX_MASK     - see epi10 for details
EPI10_SCOPE_MAX_BITS     - see epi10 for details

EPI10_HOST_PART_MASK  - see epi10 for details

*/
/*
  Constants from EPI10 spec - in Network Byte order
  (you may need to include acnip.h before this).
*/
#define E1_17_AUTO_SCOPE_ADDRESS  DD2NIP( 239,192,0,0 )
#define E1_17_AUTO_SCOPE_MASK     DD2NIP( 255,252,0,0 )
#define E1_17_AUTO_SCOPE_BITS     14

#define EPI10_SCOPE_MIN_MASK      DD2NIP( 255,192,0,0 )
#define EPI10_SCOPE_MIN_BITS      10
#define EPI10_SCOPE_MAX_MASK      DD2NIP( 255,255,0,0 )
#define EPI10_SCOPE_MAX_BITS      16

/* Note EPI10_HOST_PART_MASK is not in network byte order */
#define EPI10_HOST_PART_MASK 0xff

/**********************************************************************/
/*
macros: EPI-17 Constants

Constants from EPI17. Root Layer for UDP

These constants represent requirements defined in standard document 
*ANSI E1.17 - 2010
Architecture for Control Networks –
EPI 17. ACN Root Layer Protocol
Operation on UDP*
*/

#define RLP_PREAMBLE_LENGTH 16
#define RLP_POSTAMBLE_LENGTH 0

/* Note string below assumes a nul terminator will be added */
#define RLP_PREAMBLE_VALUE "\0\x10\0\0" "ASC-E1.17\0\0"

/**********************************************************************/
/*
macros: EPI-18 Constants

Constants from EPI18. SDT on UDP

These constants represent requirements defined in standard document 
*ANSI E1.17-2010, Architecture for Control Networks
EPI 18.
Operation of SDT on UDP Networks*
*/

#if CONFIG_ACN_VERSION == 20060000

#define MAK_TIMEOUT_ms           200
#define MAK_MAX_RETRIES          3
#define AD_HOC_TIMEOUT_ms        200
#define AD_HOC_RETRIES           3
#define MIN_EXPIRY_TIME_ms       2000
#define NAK_TIMEOUT_ms           100
#define NAK_MAX_RETRIES          3
#define NAK_HOLDOFF_INTERVAL_ms  2
#define NAK_MAX_TIME_ms          (10 * NAK_HOLDOFF_INTERVAL_ms)
#define NAK_BLANKTIME_ms         (3 * NAK_HOLDOFF_INTERVAL_ms)
#define SDT_MULTICAST_PORT       5568

#elif CONFIG_ACN_VERSION >= 20100000

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
/**********************************************************************/
/*
macros: EPI-20 Constants

Constants from EPI20. Maximum Transmission Unit (MTU)

These constants represent requirements defined in standard document 
*ANSI E1.17 - 2010
Architecture for Control Networks –
EPI 20. Maximum Transmission Unit
(MTU) Size for ACN on IPv4 Networks*
*/

#define DEFAULT_MTU 1472
#define MAX_MTU 1472

#endif   /* __e1_17_h__ */
