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
IMPORTANT YOU SHOULD NOT NEED TO EDIT THIS HEADER:

If you just want to create your own tailored build you should put 
all your local configuration options into the header 
"acncfg_local.h" where the compiler will find it.
Those definitions will override those in this file.
*/
/*
header: acncfg.h

Configuration Definitions
*/
/**********************************************************************/

#ifndef __acncfg_h__
#define __acncfg_h__ 1
#include "acncfg_local.h"

/**********************************************************************/
/*
title: Options for building ACN

There are quite a number of compile time options for building Acacian. 
Some of these make very significant improvements in code size or 
performance in specific cases.

Your local build configuration should be defined in "local_cfg.h" which 
overrides the default values for any options you need to change.

"local_cfg.h" is then included in "acncfg.h" which is included 
(usually indirectly via "acn.h") in virtually every source 
file. Do not edit "acncfg.h" itself unless adding completely new 
options to the code.

*Important:* Most relevant configuration options must be defined to `something` - 
usually either to 0 (disabed) or to 1 (enabled). They are tested 
using |#if ...| rather than |#ifdef ...| and if undefined may result in 
strange behavior. The exception is when whole modules are omitted when 
their detailed configuration options may be left undefined.

group: Main Options

All options are documented below. However, to get started, the 
most significant options you should look at are:
 - <CF_MULTI_COMPONENT> Does your program implement multiple components?
 - <CF_RLP_MAX_CLIENT_PROTOCOLS>, <CF_SDT_MAX_CLIENT_PROTOCOLS>
 Simplifies code if SLP is the only client protocol of RLP and/or DMP is the 
 only client protocol of SDT.
 - <CF_DMPCOMP_C_>, <CF_DMPCOMP__D> or <CF_DMPCOMP_CD> Is 
 your component acontroller a device or both?

*/
/*********************************************************************
*/ /* macros: Version

	CF_VERSION - An integer which represents the ACN revision to be
compiled.

Values:
20060000 - the original ANSI ESTA E1.17-2006 version
20100000 - the revised version ANSI ESTA E1.17-2010

As of April 2012 only 20100000 is supported

Note: These parts of the original 2006 standard were revised in 2010:
 - SDT
 - DMP
 - DDL
 - EPI-10
 - EPI-11
 - EPI-18
 - EPI-19
 - EPI-22

EPIs which were not included in the original ACN suite have their own
standardization process and may need their own version numbers as
necessary.

*/

#ifndef CF_VERSION
#define CF_VERSION 20100000
#endif

/**********************************************************************/
/*
	macros: Operating system (and stack)

	Currently only linux is supported but we define some 
	configuration options to bracket OS dependent code.

	CF_OS_LINUX - Operating system is Linux. Macro defines to 
	version number (3 digits each for minor and sub-versions) 
	allowing version tests, though in most cases this is not relevant.

	CF_STACK_xxx - Only needed where stack is not defined by the OS default.
*/
#ifndef CF_OS_LINUX
#define CF_OS_LINUX 3007010
#endif

/**********************************************************************/
/*
	macros: Network and Transport Features
	
	IP version (or other transport)

	CF_NET_IPV4 - IP version 4
	CF_NET_IPV6 - IP version 6 (experimental)

	Picking more than one is allowed and automatically defines 
	CF_NET_MULTI below but makes code more complex so rely on 
	IPv6 and a hybrid stack which automatically handles IPv4 if you 
	can. However, this isn't so well tested.

	CF_MAX_IPADS - Maximum number of supported IP addresses

	The code supports multihoming. This parameter may be used to set the amount
	of memory allocated for IP addresses in various operations. 

	CF_LOCALIP_ANY - Delegate all interface decisions to the OS/stack
	
	In hosts with multiple interfaces (including the loopback 
	interface) it is normal to accept packets received on any 
	interface and to leave it to the stack to select the interface 
	for outgoing packets by binding sockets to INADDR_ANY.

	If set, RLP and SDT rely on the stack to handle IP addresses and 
	interfaces and Acacian only stores port information. This saves 
	using resources tracking redundant interface information. If 
	clear then the API allows higher layers to specify individual 
	interfaces (by their address) at the expense of slightly more 
	code and memory. This setting still allows the value 
	netx_INADDR_ANY to be explicitly used as required.

	CF_MULTICAST_TTL - IP multicast TTL value
	
	The Linux manual (man 7 ip) states "It is very important for 
	multicast packets  to set the smallest TTL possible" but this 
	conflicts with rfc2365 and SLP defaults to 255. We follow the RFC 
	thinking by default - routers at critical boundaries will usually 
	not pass multicast without explicit configuration anyway.

	CF_JOIN_TX_GROUPS - Joining our own multicast groups
	
	Ideally we don't want to join all the outgoing groups we transmit on
	as this just means we get our own messages back. However, joining a
	group prompts the stack to emit the correct IGMP messages for group
	subscription and unless we do that, many switches will block any
	messages we transmit to that group.

	RECEIVE_DEST_ADDRESS - Recover the destination address for received packets

	When a host has joined many mulitcast groups, it may be useful to 
	know on receipt of a packet, which group it belongs to. However, 
	this is the `destination` address in an `incoming` packet and 
	many stacks make it tortuous or impossible to extract this 
	information so Acacian code cannot rely on this option.

*/

#ifndef CF_NET_IPV4
#define CF_NET_IPV4  1
#endif
#ifndef CF_NET_IPV6
#define CF_NET_IPV6  0
#endif
#ifndef CF_MAX_IPADS
#define CF_MAX_IPADS 16
#endif

#ifndef CF_LOCALIP_ANY
#define CF_LOCALIP_ANY 1
#endif
#ifndef CF_MULTICAST_TTL
#define CF_MULTICAST_TTL 255
#endif
#ifndef CF_JOIN_TX_GROUPS
#define CF_JOIN_TX_GROUPS 1
#endif

#ifndef RECEIVE_DEST_ADDRESS
#define RECEIVE_DEST_ADDRESS 0
#endif

/**********************************************************************/
/*
	macros: Logging

	Logging options

	These are currently compile-time options so logging cannot be changed
	in running code.

	CF_ACNLOG - determine how messages are logged.
	CF_LOGLEVEL - determine what level of messages are logged.

	Options for *CF_ACNLOG* are

	ACNLOG_OFF      - All logging is compiled out
	ACNLOG_SYSLOG   - Log using POSIX Syslog
	ACNLOG_STDOUT   - Log to standard output (default)
	ACNLOG_STDERR   - Log to standard error

	Syslog handles logging levels itself and CF_LOGLEVEL is 
	ignored. For other options log messages up to CF_LOGLEVEL are 
	logged and higher levels are ignored. Possible values are (in 
	increasing order).

	- LOG_EMERG
	- LOG_ALERT
	- LOG_CRIT
	- LOG_ERR
	- LOG_WARNING
	- LOG_NOTICE
	- LOG_INFO
	- LOG_DEBUG

	The acnlog() macro is formatted to match the POSIX syslog...
	> extern void syslog(int, const char *, ...);
	Where int is the combination of facility and error level (or'd),
	_const *_ is a formatting string and _..._ is a list of arguments.
	This allows for a function similar to the standard printf

	Individual modules (rlp, sdt etc.) each have their own facilities
	which may be set in <acncfg_local.h> to 
	
	LOG_OFF - don't log (the default)
	LOG_ON  - log to the default facility (same as LOG_USER)
	
	Or if using syslog you may select a specific facility e.g. LOG_LOCAL0
	
	for example:
	
	(code)
	#define LOG_RLP LOG_ON

	 ...

	acnlog(LOG_RLP, "I got an error");
	anclog(LOG_RLP, "I got %d errors", error_count);
	(end code)

	Log levels can still be added: this would only print if 
	CF_LOGLEVEL was LOG_INFO or higher:

	(code)
	acn_log(LOG_RLP | LOG_INFO, "I do not like errors");
	(end code)

	CF_LOGFUNCS - Log function entry and exit

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

#ifndef CF_ACNLOG
#define CF_ACNLOG ACNLOG_STDOUT
#endif

/*
LOG_ON: define a default facility for LOG_ON
Facilities are only relevant when using syslog. Default to LOG_USER
*/
#ifndef LOG_ON
#if CF_ACNLOG == ACNLOG_SYSLOG
#define LOG_ON LOG_USER
#else
#define LOG_ON 0
#endif
#endif

#ifndef CF_LOGLEVEL
#define CF_LOGLEVEL LOG_NOTICE
#endif

#ifndef CF_LOGFUNCS
#define CF_LOGFUNCS ((LOG_OFF) | LOG_DEBUG)
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
	
	CF_MARSHAL_INLINE - Use inline functions for Data Marshalling

	Inline functions for marshaling data are efficient and typecheck
	the code. If the compiler supports inline code well then they are
	preferable.

	If you do not want to compile inline, then setting this false 
	uses macros instead, but these evaluate their arguments multiple 
	times and do not check their types so beware.
*/
#ifndef CF_MARSHAL_INLINE
#define CF_MARSHAL_INLINE 1
#endif

/**********************************************************************/
/*
	macros: Error Checking
	
	CF_STRICT_CHECKS - Extra error checking

	We can spend a lot of time checking for unlikely errors
	Turn on CF_STRICT_CHECKS to enable a bunch of more esoteric 
	and paranoid tests.
*/
#ifndef CF_STRICT_CHECKS
#define CF_STRICT_CHECKS 0
#endif

/**********************************************************************/
/*
	macros: Component Model
	
	CF_MULTI_COMPONENT - One or many components?

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
	UACN which can require 189 bytesa if stored as UTF-8. This is 
	probably a mistake in the standard which should have specified 
	63 octets however we default to 190 to be on the safe side. 
	Storing as UTF-16 would require less storage but more processing.
*/

#ifndef CF_MULTI_COMPONENT
#define CF_MULTI_COMPONENT 1
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

	CF_UUIDS_RADIX - Use radix tree (patricia tree) to store UUIDs
	CF_UUIDS_HASH - Use a hash table to store UUIDs (not tested recently).

	These are mutually exclusive. If using CF_UUIDS_HASH you may also
	want to change:

	CF_Rcomp_HASHBITS - Hash size for remote component table
	CF_Lcomp_HASHBITS - Hash size for local component table
	CF_DDL_HASHBITS - Hash size for DDL modules

	If using CF_UUIDS_HASH then the 
	size of the hash table (number of bits in the hash) must be 
	defined for each table.

	The number of functions are implemented inline and speed/memory
	trade-offs vary.
*/

#ifndef CF_UUIDS_RADIX
#if CF_UUIDS_HASH
#define CF_UUIDS_RADIX 0
#else
#define CF_UUIDS_RADIX 1
#endif
#endif

#ifndef CF_UUIDS_HASH
#define CF_UUIDS_HASH (!CF_UUIDS_RADIX)
#endif

#if CF_UUIDS_HASH && !defined(CF_R_HASHBITS)
#define CF_R_HASHBITS   7
#endif
#if CF_UUIDS_HASH && !defined(CF_L_HASHBITS)
#define CF_L_HASHBITS   3
#endif

/**********************************************************************/
/*
	macros: Event loop and timing

	CF_EVLOOP - Use ACN provided event loop and timing services

	Acacian provides a single threaded event loop using epoll. The 
	application can register its own events in this loop if desired. 
	Turn this off to provide an alternative model

	CF_TIME_ms - Use simple millisecond integers for times
	CF_TIME_POSIX_timeval - Use POSIX timeval struictures for timing
	CF_TIME_POSIX_timespec - Use timespec struictures for timing

	Millisecond counters are adequate (just) for SDT specifications.
*/

#ifndef CF_EVLOOP
#define CF_EVLOOP 1
#endif

#ifndef CF_TIME_ms
#if !((defined(CF_TIME_POSIX_timeval) && CF_TIME_POSIX_timeval) || (defined(CF_TIME_POSIX_timespec) && CF_TIME_POSIX_timespec))
#define CF_TIME_ms 1
#else
#define CF_TIME_ms 0
#endif
#endif

#ifndef CF_TIME_POSIX_timeval
#if !(CF_TIME_ms || (defined(CF_TIME_POSIX_timespec) && CF_TIME_POSIX_timespec))
#define CF_TIME_POSIX_timeval 1
#else
#define CF_TIME_POSIX_timeval 0
#endif
#endif

#ifndef CF_TIME_POSIX_timespec
#define CF_TIME_POSIX_timespec !(CF_TIME_POSIX_timeval || CF_TIME_ms)
#endif

/**********************************************************************/
/*
	macros: Root Layer Protocol

	CF_RLP - enable the ACN root layer
	
	Root layer is needed for UDP but may not be needed for other 
	transports.

	CF_RLP_MAX_CLIENT_PROTOCOLS - Number of client protocols to 
	allocate space for
	
	Typically very few client protocols are used. The default is to 
	build a generic RLP for multiple client protocols However, 
	efficiency gains can be made if RLP is built for only one client 
	protocol (probably SDT or E1.31), in this case set 
	CF_RLP_MAX_CLIENT_PROTOCOLS to 1 and define 
	CF_RLP_CLIENTPROTO to the protocol ID of that client.

	CF_RLP_CLIENTPROTO - Client protocol ID for single protocol 
	implementations. ignored if CF_RLP_MAX_CLIENT_PROTOCOLS is 
	greater than one.

	e.g. For SDT only support
>		#define CF_RLP_MAX_CLIENT_PROTOCOLS 1
>		#define CF_RLP_CLIENTPROTO SDT_PROTOCOL_ID
	
	Normally both CF_RLP_MAX_CLIENT_PROTOCOLS and 
	CF_RLP_CLIENTPROTO are set to useful default values 
	depending on CF_SDT and CF_E131

	CF_RLP_OPTIMIZE_PACK - Optimize PDU packing in RLP (at the 
	cost of speed)
*/

#ifndef CF_RLP
#define CF_RLP     1
#endif

#ifndef CF_RLP_MAX_CLIENT_PROTOCOLS
#define CF_RLP_MAX_CLIENT_PROTOCOLS (CF_SDT + CF_E131)
#endif

/*
default is set below
*/
// #define CF_RLP_CLIENTPROTO

#ifndef CF_RLP_OPTIMIZE_PACK
#define CF_RLP_OPTIMIZE_PACK 0
#endif

/**********************************************************************/
/*
	macros: SDT

	CF_SDT - enable the SDT layer
	
	CF_SDT_MAX_CLIENT_PROTOCOLS - Number of client protocols to allocate
	space for. Typically very few client protocols are used. See CF_RLP_MAX_CLIENT_PROTOCOLS for
	explanation of this and CF_SDT_CLIENTPROTO

	CF_SDT_CLIENTPROTO - Client protocol for single protocol 
	implementations
	
	e.g. for DMP only
>		#define CF_SDT_MAX_CLIENT_PROTOCOLS 1
>		#define CF_SDT_CLIENTPROTO DMP_PROTOCOL_ID


	CF_RX_AUTOCALL - When an sdt wrapper is correctly received it
	is placed in an ordered queue. If CF_RX_AUTOCALL is set then
	all queued messages are unpacked and processed on completion of
	the wrapper processing.  If not defined then <readrxqueue> must
	be called from elsewhere to process the queue.

	CF_SDT_CHECK_ASSOC - The association field in SDT wrappers 
	is entirely redundant and this implementation has no need of it. 
	It sets it appropriately on transmit but only checks on receive 
	if this macro is true.
*/

#ifndef CF_SDT
#define CF_SDT     1
#endif

#ifndef CF_SDT_MAX_CLIENT_PROTOCOLS
#define CF_SDT_MAX_CLIENT_PROTOCOLS (CF_DMP)
#endif

#ifndef CF_SDTRX_AUTOCALL
#define CF_SDTRX_AUTOCALL 1
#endif

#ifndef CF_SDT_CHECK_ASSOC
#define CF_SDT_CHECK_ASSOC 0
#endif

/**********************************************************************/
/*
	macros: DMP

	CF_DMP - enable the DMP layer

	Device or Controller?

	CF_DMPCOMP__D - Build DMP device code only
	CF_DMPCOMP_C_ - Build DMP controller code only
	CF_DMPCOMP_CD - Build combined device and controller code

	At least one must be set. Many components need to implement both 
	device and controller functions, but if they only do one or the 
	other then a bunch of code can be omitted.

	DMP_MAX_SUBSCRIPTIONS - Number of property subscriptions to accept

	CF_DMPAD_MAXBYTES - Maximum DMP address size
	
	For DMP devices where it is known that all addresses fall within a
	one or two byte range, reducing CF_DMPAD_MAXBYTES to 1 or 2 
	may enable some simplifications. For generic controller code this 
	must be four (or undefined).

	*Property Mapping* DMP usually uses property maps to screen for bad addresses, for
	access permissions, and to tie incoming messages to device properties.
	These property maps can be generated from DDL.

	CF_DMPMAP_SEARCH - Use a binary search to identify the property 
	associated with an address. This is the most generic form suitable 
	for use in general purpose controllers or code serving multiple 
	complex devices.
	CF_DMPMAP_INDEX - Store properties in an array indexed by address.
	It is faster and simpler than searching 
	but only suitable for applications where all device types are known 
	and whose property addresses are packed close enough to fit within a 
	directly addressed array.
	CF_DMPMAP_NONE - eliminate all address checking code in the DMP 
	layer and simply passes up all addresses unchecked to the application.
	CF_DMPMAP_NAME - For a single device or a controller matched 
	to a single device type, define to the name of the statically 
	defined map structure to save a lot of passing pointers and 
	references. Leave undefined if not using this.

	Transport protocols - DMP may operate over multiple transport 
	protocols. e.g. SDT and TCP
	CF_DMP_MULTITRANSPORT
	CF_DMPON_SDT - Include SDT transport support
	CF_DMPON_TCP - Include TCP transport support

	CF_DMP_RMAXCXNS - Number of connections to/from the same 
	remote component. These take space in the component structure 
	for each remote.
*/

#ifndef CF_DMP
#define CF_DMP     1
#endif

#if CF_DMP

#ifndef DMP_MAX_SUBSCRIPTIONS
#define DMP_MAX_SUBSCRIPTIONS 100
#endif

#ifndef CF_DMPCOMP_CD
#define CF_DMPCOMP_CD 0
#endif

#ifndef CF_DMPCOMP_C_
#define CF_DMPCOMP_C_ 0
#endif

#ifndef CF_DMPCOMP__D
#define CF_DMPCOMP__D 0
#endif

#ifndef CF_DMPAD_MAXBYTES
#define CF_DMPAD_MAXBYTES 4
#endif

#if !(defined(CF_DMPMAP_INDEX) \
	|| defined(CF_DMPMAP_SEARCH) \
	|| defined(CF_DMPMAP_NONE) \
	|| defined(CF_DMPMAP_NAME) \
	)

#define CF_DMPMAP_INDEX 0
#define CF_DMPMAP_SEARCH 1
#define CF_DMPMAP_NONE 0

#else  /* CF_DMPMAP_xxxx all undefined */

/* assume one must be defined and default others to 0 */

#ifndef CF_DMPMAP_INDEX
#define CF_DMPMAP_INDEX 0
#endif

#ifndef CF_DMPMAP_SEARCH
#define CF_DMPMAP_SEARCH 0
#endif

#ifndef CF_DMPMAP_NONE
#define CF_DMPMAP_NONE 0
#endif

#endif

#ifndef CF_DMPON_SDT
#define CF_DMPON_SDT CF_SDT
#endif

#ifndef CF_DMPON_TCP
/* currently unsupported though there are some hooks */
#define CF_DMPON_TCP (!CF_DMPON_SDT)
#endif

#ifndef CF_DMP_MULTITRANSPORT
#define CF_DMP_MULTITRANSPORT (CF_DMPON_TCP && CF_DMPON_SDT)
#endif

#ifndef CF_DMP_RMAXCXNS
#define CF_DMP_RMAXCXNS 4
#endif

#ifndef CF_PROPEXT_FNS
#define CF_PROPEXT_FNS 0
#endif

#endif  /* CF_DMP */

/**********************************************************************/
/*
	macros: DDL
	
	CF_DDL - Enable DDL code

	DDL Parsing is rather open ended. We have various levels starting at
	a basic parse which extracts only DMPproperty map - even this level
	needs to support parameters and includedevs

	CF_DDLACCESS_DMP - Compile DDL for DMP access
	CF_DDLACCESS_EPI26 - Compile DDL for EPI26 (DMX/E1.31) access

	DDL describes how to access network devices using an access protocol.
	It is currently defined for two access protocols, DMP and EPI26 (E1.31/DMX512)
	and may be extended to others.

	CF_DDL_BEHAVIORS - Parse and apply DDL behaviors
	CF_DDL_IMMEDIATEPROPS - Parse and record values for immediate 
	properties.
	CF_DDL_MAXNEST - Maximum XML nesting level within a single 
	DDL module.
	CF_DDL_MAXTEXT - Size allocated for parsing text nodes.

*/

#ifndef CF_DDL
#define CF_DDL 	   1
#endif

#if CF_DDL

#ifndef CF_DDLACCESS_DMP
#define CF_DDLACCESS_DMP   1
#endif

#ifndef CF_DDLACCESS_EPI26
#define CF_DDLACCESS_EPI26  0
#endif

#ifndef CF_DDL_BEHAVIORS
#define CF_DDL_BEHAVIORS   1
#endif

#ifndef CF_DDL_IMMEDIATEPROPS
#define CF_DDL_IMMEDIATEPROPS   1
#endif

#ifndef CF_DDL_STRINGS
#define CF_DDL_STRINGS   1
#endif

#ifndef CF_DDL_MAXNEST
#define CF_DDL_MAXNEST 256
#endif

#ifndef CF_STR_FOLDSPACE
#define CF_STR_FOLDSPACE 0
#endif
/*
FIXME: this should be done more elegantly - behaviorset::p nodes can get
big
*/
#ifndef CF_DDL_MAXTEXT
#define CF_DDL_MAXTEXT 512
#endif

/*
macro: CF_MAPGEN

Enable static map generation extensions
*/
#ifndef CF_MAPGEN
#define CF_MAPGEN 0
#endif

#endif /* CF_DDL */

/**********************************************************************/
/*
	macros: E1.31
	
	CF_E131 - Enable E1.31 code

	CF_E131_RX - Generate receive code
	CF_E131_TX - generate transmit code
	
	We have separate configures for transmit and receive as they are
	frequently different components and do not share much code.

	E131MEM_MAXUNIVS - Maximum number of universes to track

	CF_E131_ZSTART_ONLY - Drop ASC packets on
	read and automatically add a zero start on write

	CF_E131_IGNORE_PREVIEW - Drop preview packets on read and 
	never set preview on write. All the while PREVIEW is the only 
	option flag apart from Terminate, this also cuts passing of 
	options to and from the app altogether.
*/

#ifndef CF_E131
/* currently unsupported or incomplete */
#define CF_E131            0
#endif

#if CF_E131

#ifndef CF_E131_RX
#define CF_E131_RX         1
#endif

#ifndef CF_E131_TX
#define CF_E131_TX         1
#endif

#ifndef E131MEM_MAXUNIVS
#define E131MEM_MAXUNIVS       4
#endif

#ifndef CF_E131_ZSTART_ONLY
#define CF_E131_ZSTART_ONLY        1
#endif

#ifndef CF_E131_IGNORE_PREVIEW
#define CF_E131_IGNORE_PREVIEW     1
#endif

#endif /* CF_E131 */

/**********************************************************************/
/*
macros: EPIs

Conformance to specific EPIs. This is the complete set of EPIs for 
ACN 2010 p;us some defined in ANSI E1.30. They are defined (or not) for 
completeness but some have no effect.

*Note:* Turning some of these options off may just mean the system 
will not build since there are currently no alternatives available.

CF_EPI10 - Multicast address allocation
CF_EPI11 - DDL Retrieval
CF_EPI12 - Requirements on Homogeneous Ethernet Networks
CF_EPI13 - IPv4 Addresses. Superseded by EPI29
CF_EPI15 - Multicast allocation infrastructure
CF_EPI16 - ESTA/PLASA Identifiers
CF_EPI17 - Root layer protocol for UDP
CF_EPI18 - Requirements for SDT on UDP
CF_EPI19 - Discovery using RLP
CF_EPI20 - MTU
CF_EPI26 - DDL syntax for E1.31/DMX access
CF_EPI29 - IPv4 address assignment
*/

#ifndef CF_EPI10
#define CF_EPI10   1
#endif
#ifndef CF_EPI11
#define CF_EPI11   1
#endif
#ifndef CF_EPI12
#define CF_EPI12   1
#endif
#ifndef CF_EPI15
#define CF_EPI15   1
#endif
#ifndef CF_EPI16
#define CF_EPI16   1
#endif
#ifndef CF_EPI17
#define CF_EPI17   1
#endif
#ifndef CF_EPI18
#define CF_EPI18   1
#endif
#ifndef CF_EPI19
#define CF_EPI19   1
#endif
#ifndef CF_EPI20
#define CF_EPI20   1
#endif
#ifndef CF_EPI26
#define CF_EPI26   0
#endif
#ifndef CF_EPI29
#define CF_EPI29   1
#endif

/**********************************************************************/
/*
Derived macros (including defaults which depend on earlier 
definitions) and sanity checks for some illegal configurations.
*/

#if !defined(CF_RLP_CLIENTPROTO) && CF_RLP_MAX_CLIENT_PROTOCOLS == 1
#if CF_SDT
#define CF_RLP_CLIENTPROTO SDT_PROTOCOL_ID
#elif CF_E131
#define CF_RLP_CLIENTPROTO E131_PROTOCOL_ID
#else
#error "CF_RLP_CLIENTPROTO must be defined"
#endif
#endif  /* !defined(CF_RLP_CLIENTPROTO) && CF_RLP_MAX_CLIENT_PROTOCOLS == 1 */

#if !defined(CF_SDT_CLIENTPROTO) && CF_SDT_MAX_CLIENT_PROTOCOLS == 1
#if CF_DMP
#define CF_SDT_CLIENTPROTO DMP_PROTOCOL_ID
#else
#error "CF_SDT_CLIENTPROTO must be defined"
#endif
#endif

#define CF_DMPCOMP_Cx (CF_DMPCOMP_CD || CF_DMPCOMP_C_)
#define CF_DMPCOMP_xD (CF_DMPCOMP_CD || CF_DMPCOMP__D)

#if CF_DMP && (CF_DMPCOMP_CD + CF_DMPCOMP_C_ + CF_DMPCOMP__D) != 1
#error "DMP component: set exactly 1 of CF_DMPCOMP_CD CF_DMPCOMP_C_ CF_DMPCOMP__D"
#endif

#if CF_MULTI_COMPONENT
#define ifMC(...) __VA_ARGS__
#define ifnMC(...)
#else
#define ifMC(...)
#define ifnMC(...) __VA_ARGS__
#endif

#if CF_NET_IPV4
#define ifNETv4(...) __VA_ARGS__
#define ifnNETv4(...)
#else
#define ifNETv4(...)
#define ifnNETv4(...) __VA_ARGS__
#endif

#if CF_NET_IPV6
#define ifNETv6(...) __VA_ARGS__
#define ifnNETv6(...)
#else
#define ifNETv6(...)
#define ifnNETv6(...) __VA_ARGS__
#endif

#if CF_NET_IPV4 && CF_NET_IPV6
#define CF_NET_MULTI 1
#define ifNETMULT(...) __VA_ARGS__
#define ifnNETMULT(...)
#else
#define CF_NET_MULTI 0
#define ifNETMULT(...)
#define ifnNETMULT(...) __VA_ARGS__
#endif

#if CF_RLP_MAX_CLIENT_PROTOCOLS > 1
#define ifRLP_MP(...) __VA_ARGS__
#define ifnRLP_MP(...)
#else
#define ifRLP_MP(...)
#define ifnRLP_MP(...) __VA_ARGS__
#endif

#if CF_DMPCOMP_xD
#define ifDMP_D(...) __VA_ARGS__
#define ifnDMP_D(...)
#else
#define ifDMP_D(...)
#define ifnDMP_D(...) __VA_ARGS__
#endif

#if CF_DMPCOMP_Cx
#define ifDMP_C(...) __VA_ARGS__
#define ifnDMP_C(...)
#else
#define ifDMP_C(...)
#define ifnDMP_C(...) __VA_ARGS__
#endif

#if CF_DMPCOMP_Cx && CF_DMPCOMP_xD
#define ifDMP_CD(...) __VA_ARGS__
#define ifnDMP_CD(...)
#else
#define ifDMP_CD(...)
#define ifnDMP_CD(...) __VA_ARGS__
#endif

#endif /* __acncfg_h__ */
