**********************************************************************/
/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (c) 2013, Acuity Brands, Inc.

Author: Philip Nye <philip.nye@engarts.com>

#tabs=3
*/
/**********************************************************************/
/*
about: Acacian

Acacian is a full featured implementation of ANSI E1.17 2012
Architecture for Control Networks (ACN) from Acuity Brands

header: acncfg.h

Configuration Definitions
*/
/**********************************************************************/

#ifndef __acncfg_h__
#define __acncfg_h__ 1
#include "acncfg_local.h"

/**********************************************************************/
/*
topic: Configuration Definitions

IMPORTANT YOU SHOULD NOT NEED TO EDIT THIS HEADER:

If you just want to create your own tailored build you should put 
all your local configuration options into the header 
"acncfg_local.h" where the compiler will find it.

This header (acncfg.h) includes your acncfg_local.h first and only 
provides default values if options have not been defined there.

You can refer to this header to see which options are available and 
what  they do. Note that options may not be implemented, may only 
work for certain builds or may only work in specific combinations.

CONFIGURATION MACROS MUST BE DEFINED:

Most configuration macros need to be defined to something and are 
tested using:
> #if MACRO
rather than
> #ifdef MACRO
Simple booleans should therefore be defined to 0 to disable rather than
undefined.
*/
/*********************************************************************
*/ /* macros: Version

	ACNCFG_VERSION - An integer which represents the ACN revision to be
compiled.

Values:
20060000 - the original ANSI ESTA E1.17-2006 version
20100000 - the revised version ANSI ESTA E1.17-2010

As of Apr 2012 only 20100000 is supported

Notes::

These parts of the original 2006 standard were revised in 2010: SDT, 
DMP, DDL, EPI-10, EPI-11, EPI-18, EPI-19, EPI-22

EPIs which were not included in the original ACN suite have their own
standardization process and will need their own version numbers as
necessary.

*/

#ifndef ACNCFG_VERSION
#define ACNCFG_VERSION 20100000
#endif

/**********************************************************************/
/*
	macros: Operating system (and stack)

	Currently only linux is supported but we define some 
	configuration options to bracket OS dependent code.

	ACNCFG_OS_LINUX - Operating system is Linux. Macro defines to 
	version number (3 digits each for minor and sub-versions) 
	allowing version tests, though in most cases this is not relevant.

	ACNCFG_STACK_xxx - Only needed where stack is not defined by the OS
*/
#ifndef ACNCFG_OS_LINUX
#define ACNCFG_OS_LINUX 3007010
#endif

/**********************************************************************/
/*
	macros: Network and Transport Features
	
	IP version (or other transport)

	ACNCFG_NET_IPV4 - IP version 4
	ACNCFG_NET_IPV6 - IP version 6 (experimental)

	Picking more than one is allowed and automatically defines 
	ACNCFG_NET_MULTI below but makes code more complex so rely on 
	IPv6 and a hybrid stack if you can. However, this isn't so well 
	tested.

	ACNCFG_MAX_IPADS - Maximum number of supported IP addresses

	The code supports multihoming. This parameter determines the amount
	of memory allocated for IP addresses in various operations. 

	ACNCFG_LOCALIP_ANY - Delegate all interface decisions to the OS/stack
	
	In hosts with multiple interfaces (including the loopback 
	interface) it is normal to accept packets received on any 
	interface and to leave it to the stack to select the interface 
	for outgoing packets by binding sockets to INADDR_ANY.

	If set, RLP and SDT rely on the stack to handle IP addresses and 
	interfaces and beACoN only stores port information. This saves 
	using resources tracking redundant interface information. If 
	clear then the API allows higher layers to specify individual 
	interfaces (by their address) at the expense of slightly more 
	code and memory. This setting still allows the value 
	NETI_INADDR_ANY to be explicitly used as required.

	ACNCFG_MULTICAST_TTL - IP multicast TTL value
	
	The Linux manual (man 7 ip) states "It is very important for 
	multicast packets  to set the smallest TTL possible" but this 
	conflicts with rfc2365 and SLP defaults to 255. We follow the RFC 
	thinking by default - many routers will not pass multicast 
	without explicit configuration anyway.

	ACNCFG_JOIN_TX_GROUPS - Joining our Own Multicast Groups
	
	Ideally we don't want to join all the outgoing groups we transmit on
	as this just means we get our own messages back. However, joining a
	group prompts the stack to emit the correct IGMP messages for group
	subscription and unless we do that, many switches will block any
	messages we transmit to that group.

	RECEIVE_DEST_ADDRESS - Recover the destination address for received packets

	when a host has joined many mulitcast groups, it may be useful to 
	know on receipt of a packet, which group it belongs to. However, 
	this is the *destination* address in an *incoming* packet and 
	many stacks make it tortuous or impossible to extract this 
	information.

*/

#ifndef ACNCFG_NET_IPV4
#define ACNCFG_NET_IPV4  1
#endif
#ifndef ACNCFG_NET_IPV6
#define ACNCFG_NET_IPV6  0
#endif
#ifndef ACNCFG_MAX_IPADS
#define ACNCFG_MAX_IPADS 16
#endif

#ifndef ACNCFG_LOCALIP_ANY
#define ACNCFG_LOCALIP_ANY 1
#endif
#ifndef ACNCFG_MULTICAST_TTL
#define ACNCFG_MULTICAST_TTL 255
#endif
#ifndef ACNCFG_JOIN_TX_GROUPS
#define ACNCFG_JOIN_TX_GROUPS 1
#endif

#ifndef RECEIVE_DEST_ADDRESS
#define RECEIVE_DEST_ADDRESS 0
#endif

/**********************************************************************/
/*
	macros: Logging

	Logging options

	These are currently compile-time options so logging cannot be changed
	in running code

	ACNCFG_ACNLOG - determine how messages are logged.
	ACNCFG_LOGLEVEL - determine what level of messages are logged.

	Options for *ACNCFG_ACNLOG* are

	ACNLOG_OFF      - All logging is compiled out
	ACNLOG_SYSLOG   - Log using POSIX Syslog
	ACNLOG_STDOUT   - Log to standard output (default)
	ACNLOG_STDERR   - Log to standard error

	Syslog handles logging levels itself and ACNCFG_LOGLEVEL is ignored.
	For other options Messages up to ACNCFG_LOGLEVEL are logged & levels
	beyond this are ignored. Possible values are (in increasing order).

	- LOG_EMERG
	- LOG_ALERT
	- LOG_CRIT
	- LOG_ERR
	- LOG_WARNING
	- LOG_NOTICE
	- LOG_INFO
	- LOG_DEBUG

	The acnlog() macro is formated to match the POSIX syslog...
	> extern void syslog(int, const char *, ...);
	Where int is the combination of facility and error level (or'd),
	_const *_ is a formatting string and _..._ is a list of arguments.
	This allows for a function similar to the standard printf

	Individual modules (rlp, sdt etc.) each have their own facilities
	which may be set in acncfg_local.h to 
	
	LOG_OFF - don't log (the default)
	LOG_ON  - log to the default facility (same as LOG_USER)
	
	Or if using syslog you may select a specific facility e.g. LOG_LOCAL0
	
	for example:
	
	(code)
	#define LOG_RLP LOG_ON

	 ...

	acnlog(LOG_RLP, "I got an error")'
	anclog(LOG_RLP, "I got %d errors", error_count);
	(end code)

	Log levels can still be added: this would only print if 
	ACNCFG_LOGLEVEL was LOG_INFO or higher:

	> acn_log(LOG_RLP | LOG_INFO, "I do not like errors");

	ACNCFG_LOGFUNCS - Log function entry and exit

	Useful for deep debugging but very verbose, function start (and 
	end) logging gets its own config so it can be turned off separately.

	LOG_RLP - Log the root layer
	LOG_SDT - Log SDT
	LOG_NETX - Log network interface
	LOG_DMP - Log DMP
	LOG_DDL    - Log DDL parsing
	LOG_MISC   - Log various utilities
	LOG_EVLOOP - Log the event/timer loop
	LOG_E131   - Log sACN
	LOG_APP    - Available for your application
	LOG_SESS   - Special setting for SDT sessions command

	Separate macros allow logging to be set for different modiles
*/
#define ACNLOG_OFF   0
#define ACNLOG_SYSLOG 1
#define ACNLOG_STDOUT 2
#define ACNLOG_STDERR 3
#define LOG_OFF (-1)

#ifndef ACNCFG_ACNLOG
#define ACNCFG_ACNLOG ACNLOG_STDOUT
#endif

/*
define a default facility for LOG_ON
Facilities are only relevant when using syslog
*/
#ifndef LOG_ON
#if ACNCFG_ACNLOG == ACNLOG_SYSLOG
#define LOG_ON LOG_USER
#else
#define LOG_ON 0
#endif
#endif

#ifndef ACNCFG_LOGLEVEL
#define ACNCFG_LOGLEVEL LOG_NOTICE
#endif

#ifndef ACNCFG_LOGFUNCS
#define ACNCFG_LOGFUNCS ((LOG_ON) | LOG_DEBUG)
#endif

#ifndef LOG_RLP
#define LOG_RLP LOG_ON
#endif
#ifndef LOG_SDT
#define LOG_SDT LOG_ON
#endif
#ifndef LOG_NETX
#define LOG_NETX LOG_ON
#endif
#ifndef LOG_DMP
#define LOG_DMP LOG_ON
#endif
#ifndef LOG_DDL
#define LOG_DDL LOG_ON
#endif
#ifndef LOG_MISC
#define LOG_MISC LOG_ON
#endif
#ifndef LOG_EVLOOP
#define LOG_EVLOOP LOG_ON
#endif
#ifndef LOG_E131
#define LOG_E131 LOG_ON
#endif
#ifndef LOG_APP
#define LOG_APP LOG_ON
#endif
#ifndef LOG_SESS
#define LOG_SESS LOG_ON
#endif

/**********************************************************************/
/*
	macros: Data Marshalling and Unmarshalling
	
	ACNCFG_MARSHAL_INLINE - Use inline functions for Data Marshalling

	Inline functions for marshaling data are efficient and typecheck
	the code. If the compiler supports inline code then they are
	preferable.

	If you do not want to compile inline, then setting this false 
	uses macros instead, but these evaluate their arguments multiple 
	times and do not check their types so beware.
*/
#ifndef ACNCFG_MARSHAL_INLINE
#define ACNCFG_MARSHAL_INLINE 1
#endif

/**********************************************************************/
/*
	macros: Error Checking
	
	ACNCFG_STRICT_CHECKS - Extra error checking

	We can spend a lot of time checking for unlikely errors
	Turn on ACNCFG_STRICT_CHECKS to enable a bunch 
	of more esoteric and paranoid tests.
*/
#ifndef ACNCFG_STRICT_CHECKS
#define ACNCFG_STRICT_CHECKS 0
#endif

/**********************************************************************/
/*
	macros: Component Model
	
	ACNCFG_MULTI_COMPONENT - One or many components?

	A large number of applications (probably the majority) only 
	define a single component, so including code to register and 
	maintain many of them is wasteful. If your application 
	implements multiple components then set this true.

	ACN_FCTN_SIZE - Length (in bytes) assigned for FCTN
	ACN_UACN_SIZE - Length (in bytes) assigned for UACN

	Fixed Component Type Name (FCTN) and User Assigned Component Name
	(UACN) are defined in EPI19. They are also used in E1.31. They 
	are transmitted in UTF-8 encoding.

	The standard does not specify a size for FCTN so we arbirarily 
	assign storage.

	The standard specifies a minimum storage of 63 *characters* for 
	UACN which requires 189 bytes if stored as UTF-8. This is 
	probably a mistake in the standard which should have specified 
	63 octets however we default to 190 to be on the safe side. 
	Storing as UTF-16 would require less storage but more processing.
*/

#ifndef ACNCFG_MULTI_COMPONENT
#define ACNCFG_MULTI_COMPONENT 1
#endif
#ifndef ACN_FCTN_SIZE
#define ACN_FCTN_SIZE 128  /* arbitrary */
#endif
#ifndef ACN_UACN_SIZE
#define ACN_UACN_SIZE 190  /* allow for null terminator */
#endif

/**********************************************************************/
/*
	macros: UUID tracking

	ACN uses UUIDs extensively and various structures including 
	components and DDL modules are indexed by UUID. Acacian implements 
	generic routines for high speed indexing and searching by UUID 
	see <uuid.h>.

	ACNCFG_UUIDS_RADIX - Use radix tree (patricia tree) to store UUIDs
	ACNCFG_UUIDS_HASH - Use a hash table to store UUIDs

	These are mutually exclusive.

	ACNCFG_Rcomp_HASHBITS - Hash size for remote component table
	ACNCFG_Lcomp_HASHBITS - Hash size for local component table
	ACNCFG_DDL_HASHBITS - Hash size for DDL modules

	If using ACNCFG_UUIDS_HASH then the 
	size of the hash table (number of bits in the hash) must be 
	defined for each table.

	The number of functions are implemented inline and speed/memory
	trade-offs vary.
*/

#ifndef ACNCFG_UUIDS_RADIX
#if ACNCFG_UUIDS_HASH
#define ACNCFG_UUIDS_RADIX 0
#else
#define ACNCFG_UUIDS_RADIX 1
#endif
#endif

#ifndef ACNCFG_UUIDS_HASH
#define ACNCFG_UUIDS_HASH (!ACNCFG_UUIDS_RADIX)
#endif

#if ACNCFG_UUIDS_HASH && !defined(ACNCFG_R_HASHBITS)
#define ACNCFG_R_HASHBITS   7
#endif
#if ACNCFG_UUIDS_HASH && !defined(ACNCFG_L_HASHBITS)
#define ACNCFG_L_HASHBITS   3
#endif

/**********************************************************************/
/*
	macros: Event loop and timing

	ACNCFG_EVLOOP - Use ACN provided event loop and timing services

	Acacian provides a single threaded event loop using epoll. The 
	application can register its own events in this loop if desired. 
	Turn this off to provide an alternative model

	ACNCFG_TIME_ms - Use simple millisecond integers for times
	ACNCFG_TIME_POSIX_timeval - Use POSIX timeval struictures for timing
	ACNCFG_TIME_POSIX_timespec - Use timespec struictures for timing

	Millisecond counters are adequate (just) for SDT specifications.
*/

#ifndef ACNCFG_EVLOOP
#define ACNCFG_EVLOOP 1
#endif

#ifndef ACNCFG_TIME_ms
#if !((defined(ACNCFG_TIME_POSIX_timeval) && ACNCFG_TIME_POSIX_timeval) || (defined(ACNCFG_TIME_POSIX_timespec) && ACNCFG_TIME_POSIX_timespec))
#define ACNCFG_TIME_ms 1
#else
#define ACNCFG_TIME_ms 0
#endif
#endif

#ifndef ACNCFG_TIME_POSIX_timeval
#if !(ACNCFG_TIME_ms || (defined(ACNCFG_TIME_POSIX_timespec) && ACNCFG_TIME_POSIX_timespec))
#define ACNCFG_TIME_POSIX_timeval 1
#else
#define ACNCFG_TIME_POSIX_timeval 0
#endif
#endif

#ifndef ACNCFG_TIME_POSIX_timespec
#define ACNCFG_TIME_POSIX_timespec !(ACNCFG_TIME_POSIX_timeval || ACNCFG_TIME_ms)
#endif

/**********************************************************************/
/*
	macros: Root Layer Protocol

	ACNCFG_RLP - enable the ACN root layer
	
	Root layer is needed for UDP but may not be needed for other 
	transports.

	ACNCFG_RLP_MAX_CLIENT_PROTOCOLS - Number of client protocols to 
	allocate space for
	
	Typically very few client protocols are used. The default is to 
	build a generic RLP for multiple client protocols However, 
	efficiency gains can be made if RLP is built for only one client 
	protocol (probably SDT or E1.31), in this case set 
	ACNCFG_RLP_MAX_CLIENT_PROTOCOLS to 1 and define 
	ACNCFG_RLP_CLIENTPROTO to the protocol ID of that client.

	ACNCFG_RLP_CLIENTPROTO - Client protocol ID for single protocol 
	implementations. ignored if ACNCFG_RLP_MAX_CLIENT_PROTOCOLS is 
	greater than one.

	e.g. For SDT only support
>		#define ACNCFG_RLP_MAX_CLIENT_PROTOCOLS 1
>		#define ACNCFG_RLP_CLIENTPROTO SDT_PROTOCOL_ID
	
	Normally both ACNCFG_RLP_MAX_CLIENT_PROTOCOLS and 
	ACNCFG_RLP_CLIENTPROTO are set to useful default values 
	depending on ACNCFG_SDT and ACNCFG_E131

	ACNCFG_RLP_OPTIMIZE_PACK - Optimize PDU packing in RLP (at the 
	cost of speed)
*/

#ifndef ACNCFG_RLP
#define ACNCFG_RLP     1
#endif

#ifndef ACNCFG_RLP_MAX_CLIENT_PROTOCOLS
#define ACNCFG_RLP_MAX_CLIENT_PROTOCOLS (ACNCFG_SDT + ACNCFG_E131)
#endif

/*
default is set below
*/
// #define ACNCFG_RLP_CLIENTPROTO

#ifndef ACNCFG_RLP_OPTIMIZE_PACK
#define ACNCFG_RLP_OPTIMIZE_PACK 0
#endif

/**********************************************************************/
/*
	macros: SDT

	ACNCFG_SDT - enable the SDT layer
	
	ACNCFG_SDT_MAX_CLIENT_PROTOCOLS - Number of client protocols to allocate
	space for. Typically very few client protocols are used. See ACNCFG_RLP_MAX_CLIENT_PROTOCOLS for
	explanation of this and ACNCFG_SDT_CLIENTPROTO

	ACNCFG_SDT_CLIENTPROTO - Client protocol for single protocol 
	implementations
	
	e.g. for DMP only
>		#define ACNCFG_SDT_MAX_CLIENT_PROTOCOLS 1
>		#define ACNCFG_SDT_CLIENTPROTO DMP_PROTOCOL_ID


	ACNCFG_RX_AUTOCALL - When an sdt wrapper is correctly received it
	is placed in an ordered queue. If ACNCFG_RX_AUTOCALL is set then
	all queued messages are unpacked and processed on completion of
	the wrapper processing.  If not defined then <readrxqueue> must
	be called from elsewhere to process the queue.

	ACNCFG_SDT_CHECK_ASSOC - The association field in SDT wrappers 
	is entirely redundant and this implementation has no need of it. 
	It sets it appropriately on transmit but only checks on receive 
	if this macro is true.
*/

#ifndef ACNCFG_SDT
#define ACNCFG_SDT     1
#endif

#ifndef ACNCFG_SDT_MAX_CLIENT_PROTOCOLS
#define ACNCFG_SDT_MAX_CLIENT_PROTOCOLS (ACNCFG_DMP)
#endif

#ifndef ACNCFG_SDTRX_AUTOCALL
#define ACNCFG_SDTRX_AUTOCALL 1
#endif

#ifndef ACNCFG_SDT_CHECK_ASSOC
#define ACNCFG_SDT_CHECK_ASSOC 0
#endif

/**********************************************************************/
/*
	macros: DMP

	ACNCFG_DMP - enable the DMP layer

	ACNCFG_DMPCOMP_xD - Enable DMP device support
	ACNCFG_DMPCOMP_Cx - Enable DMP controller support

	At least one must be set. Many components need to implement both 
	device and controller functions, but if they only do one or the 
	other then a bunch of code can be omitted.

	DMP_MAX_SUBSCRIPTIONS - Number of property subscriptions to accept

	ACNCFG_DMPAD_MAXBYTES - Maximum DMP address size
	
	For DMP devices where it is known that all addresses fall within a
	one or two byte range, reducing ACNCFG_DMPAD_MAXBYTES to 1 or 2 
	may enable some simplifications. For generic controller code this 
	must be four (or undefined).

	*Property Mapping* DMP usually uses property maps to screen for bad addresses, for
	access permissions, and to tie incoming messages to device properties.
	These property maps can be generated from DDL.

	ACNCFG_DMPMAP_SEARCH - Use a binary search to identify the property 
	associated with an address. This is the most generic form suitable 
	for use in general purpose controllers or code serving multiple 
	complex devices.
	ACNCFG_DMPMAP_INDEX - Store properties in an array indexed by address.
	It is faster and simpler than searching 
	but only suitable for applications where all device types are known 
	and whose property addresses are packed close enough to fit within a 
	directly addressed array.
	ACNCFG_DMPMAP_NONE - eliminate all address checking code in the DMP 
	layer and simply passes up all addresses unchecked to the application.
	ACNCFG_DMPMAP_NAME - For a single device or a controller matched 
	to a single device type, define to the name of the statically 
	defined map structure to save a lot of passing pointers and 
	references. Leave undefined if not using this.

	Transport protocols - DMP may operate over multiple transport 
	protocols. e.g. SDT and TCP
	ACNCFG_DMP_MULTITRANSPORT
	ACNCFG_DMPON_SDT - Include SDT transport support
	ACNCFG_DMPON_TCP - Include TCP transport support

	ACNCFG_DMP_RMAXCXNS - Number of connections to/from the same 
	remote component. These take space in the component structure 
	for each remote.
*/

#ifndef ACNCFG_DMP
#define ACNCFG_DMP     1
#endif

#if ACNCFG_DMP

#ifndef DMP_MAX_SUBSCRIPTIONS
#define DMP_MAX_SUBSCRIPTIONS 100
#endif

#ifndef ACNCFG_DMPCOMP_CD
#define ACNCFG_DMPCOMP_CD 0
#endif

#ifndef ACNCFG_DMPCOMP_C_
#define ACNCFG_DMPCOMP_C_ 0
#endif

#ifndef ACNCFG_DMPCOMP__D
#define ACNCFG_DMPCOMP__D 0
#endif

#ifndef ACNCFG_DMPAD_MAXBYTES
#define ACNCFG_DMPAD_MAXBYTES 4
#endif

#if !(defined(ACNCFG_DMPMAP_INDEX) \
	|| defined(ACNCFG_DMPMAP_SEARCH) \
	|| defined(ACNCFG_DMPMAP_NONE) \
	|| defined(ACNCFG_DMPMAP_NAME) \
	)

#define ACNCFG_DMPMAP_INDEX 0
#define ACNCFG_DMPMAP_SEARCH 1
#define ACNCFG_DMPMAP_NONE 0

#else  /* ACNCFG_DMPMAP_xxxx all undefined */

/* assume one must be defined and default others to 0 */

#ifndef ACNCFG_DMPMAP_INDEX
#define ACNCFG_DMPMAP_INDEX 0
#endif

#ifndef ACNCFG_DMPMAP_SEARCH
#define ACNCFG_DMPMAP_SEARCH 0
#endif

#ifndef ACNCFG_DMPMAP_NONE
#define ACNCFG_DMPMAP_NONE 0
#endif

#endif

#ifndef ACNCFG_DMPON_SDT
#define ACNCFG_DMPON_SDT ACNCFG_SDT
#endif

#ifndef ACNCFG_DMPON_TCP
/* currently unsupported though there are some hooks */
#define ACNCFG_DMPON_TCP (!ACNCFG_DMPON_SDT)
#endif

#ifndef ACNCFG_DMP_MULTITRANSPORT
#define ACNCFG_DMP_MULTITRANSPORT (ACNCFG_DMPON_TCP && ACNCFG_DMPON_SDT)
#endif

#ifndef ACNCFG_DMP_RMAXCXNS
#define ACNCFG_DMP_RMAXCXNS 4
#endif

#ifndef ACNCFG_PROPEXT_FNS
#define ACNCFG_PROPEXT_FNS 0
#endif

#endif  /* ACNCFG_DMP */

/**********************************************************************/
/*
	macros: DDL
	
	ACNCFG_DDL - Enable DDL code

	DDL Parsing is rather open ended. We have various levels starting at
	a basic parse which extracts only DMPproperty map - even this level
	needs to support parameters and includedevs

	ACNCFG_DDLACCESS_DMP - Compile DDL for DMP access
	ACNCFG_DDLACCESS_EPI26 - Compile DDL for EPI26 (DMX/E1.31) access

	DDL describes how to access network devices using an access protocol.
	It is currently defined for two access protocols, DMP and EPI26 (E1.31/DMX512)
	and may be extended to others.

	ACNCFG_DDL_BEHAVIORS - Parse and apply DDL behaviors
	ACNCFG_DDL_BEHAVIORFLAGS - Add flags to property for simple boolean 
	behaviors including constant, volatile, persistent
	ACNCFG_DDL_BEHAVIORTYPES - Add an enumeration to the information 
	for each property which specifies the datatype/encoding. This is 
	generated by the behaviors of the property.
	ACNCFG_DDL_IMMEDIATEPROPS - Parse and record values for immediate 
	properties.
	ACNCFG_DDL_IMPLIEDPROPS - Parse and record implied properties
	ACNCFG_DDL_MAXNEST - Maximum XML nesting level within a single 
	DDL module.
	ACNCFG_DDL_MAXTEXT - Size allocated for parsing text nodes.

*/

#ifndef ACNCFG_DDL
#define ACNCFG_DDL 	   1
#endif

#if ACNCFG_DDL

#ifndef ACNCFG_DDLACCESS_DMP
#define ACNCFG_DDLACCESS_DMP   1
#endif

#ifndef ACNCFG_DDLACCESS_EPI26
#define ACNCFG_DDLACCESS_EPI26  0
#endif

#ifndef ACNCFG_DDL_BEHAVIORS
#define ACNCFG_DDL_BEHAVIORS   1
#endif

#ifndef ACNCFG_DDL_BEHAVIORFLAGS
#define ACNCFG_DDL_BEHAVIORFLAGS   1
#endif

#ifndef ACNCFG_DDL_BEHAVIORTYPES
#define ACNCFG_DDL_BEHAVIORTYPES   1
#endif

#ifndef ACNCFG_DDL_IMMEDIATEPROPS
#define ACNCFG_DDL_IMMEDIATEPROPS   1
#endif

#ifndef ACNCFG_DDL_IMPLIEDPROPS
#define ACNCFG_DDL_IMPLIEDPROPS   1
#endif

#ifndef ACNCFG_DDL_LABELS
#define ACNCFG_DDL_LABELS   1
#endif

#ifndef ACNCFG_DDL_MAXNEST
#define ACNCFG_DDL_MAXNEST 256
#endif

/*
FIXME: this should be done more elegantly - behaviorset::p nodes can get
big
*/
#ifndef ACNCFG_DDL_MAXTEXT
#define ACNCFG_DDL_MAXTEXT 512
#endif

/*
macro: ACNCFG_MAPGEN

Enable static map generation extensions
*/
#ifndef ACNCFG_MAPGEN
#define ACNCFG_MAPGEN 0
#endif

#endif /* ACNCFG_DDL */

/**********************************************************************/
/*
	macros: E1.31
	
	ACNCFG_E131 - Enable E1.31 code

	ACNCFG_E131_RX - Generate receive code
	ACNCFG_E131_TX - generate transmit code
	
	We have separate configures for transmit and receive as they are
	frequently different components and do not share much code.

	E131MEM_MAXUNIVS - Maximum number of universes to track

	ACNCFG_E131_ZSTART_ONLY - Drop ASC packets on
	read and automatically add a zero start on write

	ACNCFG_E131_IGNORE_PREVIEW - Drop preview packets on read and 
	never set preview on write. All the while PREVIEW is the only 
	option flag apart from Terminate, this also cuts passing of 
	options to and from the app altogether.
*/

#ifndef ACNCFG_E131
/* currently unsupported or incomplete */
#define ACNCFG_E131            0
#endif

#if ACNCFG_E131

#ifndef ACNCFG_E131_RX
#define ACNCFG_E131_RX         1
#endif

#ifndef ACNCFG_E131_TX
#define ACNCFG_E131_TX         1
#endif

#ifndef E131MEM_MAXUNIVS
#define E131MEM_MAXUNIVS       4
#endif

#ifndef ACNCFG_E131_ZSTART_ONLY
#define ACNCFG_E131_ZSTART_ONLY        1
#endif

#ifndef ACNCFG_E131_IGNORE_PREVIEW
#define ACNCFG_E131_IGNORE_PREVIEW     1
#endif

#endif /* ACNCFG_E131 */

/**********************************************************************/
/*
macros: EPIs

Conformance to specific EPIs. This is the complete set of EPIs for 
ACN 2010 p;us some defined in ANSI E1.30. They are defined (or not) for 
completeness but some have no effect.

*Note:* Turning some of these options off may just mean the system 
will not build since there are currently no alternatives available.

ACNCFG_EPI10 - Multicast address allocation
ACNCFG_EPI11 - DDL Retrieval
ACNCFG_EPI12 - Requirements on Homogeneous Ethernet Networks
ACNCFG_EPI13 - IPv4 Addresses. Superseded by EPI29
ACNCFG_EPI15 - Multicast allocation infrastructure
ACNCFG_EPI16 - ESTA/PLASA Identifiers
ACNCFG_EPI17 - Root layer protocol for UDP
ACNCFG_EPI18 - Requirements for SDT on UDP
ACNCFG_EPI19 - Discovery using RLP
ACNCFG_EPI20 - MTU
ACNCFG_EPI26 - DDL syntax for E1.31/DMX access
ACNCFG_EPI29 - IPv4 address assignment
*/

#ifndef ACNCFG_EPI10
#define ACNCFG_EPI10   1
#endif
#ifndef ACNCFG_EPI11
#define ACNCFG_EPI11   1
#endif
#ifndef ACNCFG_EPI12
#define ACNCFG_EPI12   1
#endif
#ifndef ACNCFG_EPI15
#define ACNCFG_EPI15   1
#endif
#ifndef ACNCFG_EPI16
#define ACNCFG_EPI16   1
#endif
#ifndef ACNCFG_EPI17
#define ACNCFG_EPI17   1
#endif
#ifndef ACNCFG_EPI18
#define ACNCFG_EPI18   1
#endif
#ifndef ACNCFG_EPI19
#define ACNCFG_EPI19   1
#endif
#ifndef ACNCFG_EPI20
#define ACNCFG_EPI20   1
#endif
#ifndef ACNCFG_EPI26
#define ACNCFG_EPI26   0
#endif
#ifndef ACNCFG_EPI29
#define ACNCFG_EPI29   1
#endif

/**********************************************************************/
/*
Derived macros (including defaults which depend on earlier 
definitions) and sanity checks for some illegal configurations.
*/

#if !defined(ACNCFG_RLP_CLIENTPROTO) && ACNCFG_RLP_MAX_CLIENT_PROTOCOLS == 1
#if ACNCFG_SDT
#define ACNCFG_RLP_CLIENTPROTO SDT_PROTOCOL_ID
#elif ACNCFG_E131
#define ACNCFG_RLP_CLIENTPROTO E131_PROTOCOL_ID
#else
#error "ACNCFG_RLP_CLIENTPROTO must be defined"
#endif
#endif  /* !defined(ACNCFG_RLP_CLIENTPROTO) && ACNCFG_RLP_MAX_CLIENT_PROTOCOLS == 1 */

#if !defined(ACNCFG_SDT_CLIENTPROTO) && ACNCFG_SDT_MAX_CLIENT_PROTOCOLS == 1
#if ACNCFG_DMP
#define ACNCFG_SDT_CLIENTPROTO DMP_PROTOCOL_ID
#else
#error "ACNCFG_SDT_CLIENTPROTO must be defined"
#endif
#endif

#define ACNCFG_DMPCOMP_Cx (ACNCFG_DMPCOMP_CD || ACNCFG_DMPCOMP_C_)
#define ACNCFG_DMPCOMP_xD (ACNCFG_DMPCOMP_CD || ACNCFG_DMPCOMP__D)

#if ACNCFG_DMP && (ACNCFG_DMPCOMP_CD + ACNCFG_DMPCOMP_C_ + ACNCFG_DMPCOMP__D) != 1
#error "DMP component: set exactly 1 of ACNCFG_DMPCOMP_CD ACNCFG_DMPCOMP_C_ ACNCFG_DMPCOMP__D"
#endif

#if ACNCFG_MULTI_COMPONENT
#define ifMC(...) __VA_ARGS__
#define ifnMC(...)
#else
#define ifMC(...)
#define ifnMC(...) __VA_ARGS__
#endif

#if ACNCFG_NET_IPV4
#define ifNETv4(...) __VA_ARGS__
#define ifnNETv4(...)
#else
#define ifNETv4(...)
#define ifnNETv4(...) __VA_ARGS__
#endif

#if ACNCFG_NET_IPV6
#define ifNETv6(...) __VA_ARGS__
#define ifnNETv6(...)
#else
#define ifNETv6(...)
#define ifnNETv6(...) __VA_ARGS__
#endif

#if ACNCFG_NET_IPV4 && ACNCFG_NET_IPV6
#define ACNCFG_NET_MULTI 1
#define ifNETMULT(...) __VA_ARGS__
#define ifnNETMULT(...)
#else
#define ACNCFG_NET_MULTI 0
#define ifNETMULT(...)
#define ifnNETMULT(...) __VA_ARGS__
#endif

#if ACNCFG_RLP_MAX_CLIENT_PROTOCOLS > 1
#define ifRLP_MP(...) __VA_ARGS__
#define ifnRLP_MP(...)
#else
#define ifRLP_MP(...)
#define ifnRLP_MP(...) __VA_ARGS__
#endif

#if ACNCFG_DMPCOMP_xD
#define ifDMP_D(...) __VA_ARGS__
#define ifnDMP_D(...)
#else
#define ifDMP_D(...)
#define ifnDMP_D(...) __VA_ARGS__
#endif

#if ACNCFG_DMPCOMP_Cx
#define ifDMP_C(...) __VA_ARGS__
#define ifnDMP_C(...)
#else
#define ifDMP_C(...)
#define ifnDMP_C(...) __VA_ARGS__
#endif

#if ACNCFG_DMPCOMP_Cx && ACNCFG_DMPCOMP_xD
#define ifDMP_CD(...) __VA_ARGS__
#define ifnDMP_CD(...)
#else
#define ifDMP_CD(...)
#define ifnDMP_CD(...) __VA_ARGS__
#endif

#endif /* __acncfg_h__ */
