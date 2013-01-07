/**********************************************************************/
/*
	Copyright (c) 2007-2010, Engineering Arts (UK)

#tabs=3t
*/
/**********************************************************************/

#ifndef __acncfg_h__
#define __acncfg_h__ 1
#include "acncfg_local.h"

/**********************************************************************/
/*
file: acncfg.h

Configuration Definitions

This file provides default values for ACN build-time configuration macros.
Configuration macros *must* be defined. most are booleans and should be 
defined either 1 (true) or 0 (false). The tests for macros in the code 
mostly look like this:

> #if CONFIG_FOO

and not

> #if defined(CONFIG_FOO)

If CONFIG_FOO is undefined then results may not be as unexpected.

Note:
	DO NOT EDIT THIS HEADER

	You should not need to edit this header: If you just want to 
	create your own tailored build you should 
	put all your local configuration options into the header 
	"acncfg_local.h" where the compiler will find it.

	This header (acncfg.h) includes your acncfg_local.h first and only 
	provides default values if options have not been defined there.

	You can refer to this header to see which options are available 
	and what  they do. Note that options may not be implemented, may  
	only work for certain builds or may only work in specific 
	combinations.
*/
/**********************************************************************/
/*
macro: CONFIG_ACN_VERSION

An integer which represents the ACN revision to be
compiled.

These parts of the original 2006 standard were revised in 2010::

o SDT
o DMP
o DDL
o EPI-10
o EPI-11
o EPI-18
o EPI-19
o EPI-22

EPIs which were not included in the original ACN suite have their own
standardization process and will need their own version numbers as
necessary.


Expect earlier versions of ACN to become deprecated over time.

Allowable values are::

20060000 - the original ANSI ESTA E1.17-2006 version
20100000 - the revised version ANSI ESTA E1.17-2010

*Note:* As of Apr 2012 only 20100000 is supported
*/
/**********************************************************************/

#ifndef CONFIG_ACN_VERSION
#define CONFIG_ACN_VERSION 20100000
#endif

/**********************************************************************/
/*
macros: ACN protocols and standards

Define which protocols and EPIs to build or conform to.

CONFIG_RLP - Root Layer Protocol
CONFIG_SDT - Session Data Transport
CONFIG_DMP - Device Management Protocol
CONFIG_DDL - Device Description Language
	
*/
/**********************************************************************/

/*
Core ACN protocols
*/
#ifndef CONFIG_RLP
#define CONFIG_RLP     1
#endif
#ifndef CONFIG_SDT
#define CONFIG_SDT     1
#endif
#ifndef CONFIG_DMP
#define CONFIG_DMP     1
#endif
#ifndef CONFIG_E131
#define CONFIG_E131    0
#endif
#ifndef CONFIG_DDL
#define CONFIG_DDL 	  CONFIG_DMP
#endif

/*
macros: EPIs

Conformance to specific EPIs

CONFIG_EPI10 - Multicast address allocation
CONFIG_EPI11 - DDL Retrieval
CONFIG_EPI12 - Requirements on Homogeneous Ethernet Networks
CONFIG_EPI13 - IPv4 Addresses. Superseded by EPI29
CONFIG_EPI15 - Multicast allocation infrastructure
CONFIG_EPI16 - ESTA/PLASA Identifiers
CONFIG_EPI17 - Root layer protocol for UDP
CONFIG_EPI18 - Requirements for SDT on UDP
CONFIG_EPI19 - Discovery using RLP
CONFIG_EPI20 - MTU
CONFIG_EPI29 - IPv4 address assignment

*Note:* Turning some of these options off may just 
mean the system will not build since there are 
currently no alternatives available.
*/
#ifndef CONFIG_EPI10
#define  CONFIG_EPI10   (CONFIG_NET_IPV4 && CONFIG_SDT)
#endif
#ifndef CONFIG_EPI11
#define  CONFIG_EPI11   1
#endif
#ifndef CONFIG_EPI12
#define  CONFIG_EPI12   1
#endif
#ifndef CONFIG_EPI13
#define  CONFIG_EPI13   0
#endif
#ifndef CONFIG_EPI15
#define  CONFIG_EPI15   CONFIG_NET_IPV4
#endif
#ifndef CONFIG_EPI16
#define  CONFIG_EPI16   1
#endif
#ifndef CONFIG_EPI17
#define  CONFIG_EPI17   1
#endif
#ifndef CONFIG_EPI18
#define  CONFIG_EPI18   1
#endif
#ifndef CONFIG_EPI19
#define  CONFIG_EPI19   1
#endif
#ifndef CONFIG_EPI20
#define  CONFIG_EPI20   CONFIG_NET_IPV4
#endif
#ifndef CONFIG_EPI29
#define  CONFIG_EPI29   CONFIG_NET_IPV4
#endif
/**@}*/
/**@}*/

/**********************************************************************/
/*
macros: C Compiler

Normally autodetected

We can autodetect this from predefined macros
but these vary from system to system so use our own macro
names and allow user override in acncfg_local.h

CONFIG_GNUCC - Gnu Compiler
CONFIG_MSVC  - Microsoft Visual C

*Note:* The only recently tested compiler (Apr 2012) is GCC
*/
/**********************************************************************/

/* Best to use GCC if available */
#ifndef CONFIG_GNUCC
#ifdef __GNUC__
#define CONFIG_GNUCC 1
#else
#define CONFIG_GNUCC 0
#endif
#endif

/* MS Visual C - only for Windows native targets */
#ifndef CONFIG_MSVC
#ifdef _MSC_VER
#define CONFIG_MSVC _MSC_VER
#else
#define CONFIG_MSVC 0
#endif
#endif

/**********************************************************************/
/*
macros: CPU Architecture

Normally autodetected

we can autodetect a lot of this from predefined macros
but these vary from system to system so use our own macro
names and allow user override in acncfg_local.h.

ARCH_x86_64 - x86_64 architecture
ARCH_i386   - "Intel" i386 architecture
ARCH_h8300  - H8 processors from Renesas
ARCH_h8300s - variant of H8
ARCH_arm - ARM, there are lots!
ARCH_thumb - ARM in thumb mode
ARCH_m68k - from Freescale
ARCH_coldfire - from Freescale
*/
/**********************************************************************/

#ifndef ARCH_x86_64
#if defined(__x86_64__) || defined(_M_X64)
#define ARCH_x86_64 1
#else
#define ARCH_x86_64 0
#endif
#endif

#ifndef ARCH_i386
#if defined(__i386__) || defined(_M_IX86)
#define ARCH_i386 1
#else
#define ARCH_i386 0
#endif
#endif

#ifndef ARCH_h8300
#if defined(__H8300__)
#define ARCH_h8300 1
#else
#define ARCH_h8300 0
#endif
#endif

#ifndef ARCH_h8300s
#if defined(__H8300S__)
#define ARCH_h8300s 1
#else
#define ARCH_h8300s 0
#endif
#endif

#ifndef ARCH_thumb
#if defined(__thumb__)
#define ARCH_thumb 1
#else
#define ARCH_thumb 0
#endif
#endif

#ifndef ARCH_arm
#if defined(__arm__)
#define ARCH_arm 1
#else
#define ARCH_arm 0
#endif
#endif

#ifndef ARCH_coldfire
#if defined(__mcoldfire__)
#define ARCH_coldfire 1
#else
#define ARCH_coldfire 0
#endif
#endif

#ifndef ARCH_m68k
#if defined(__m68k__)
#define ARCH_m68k 1
#else
#define ARCH_m68k 0
#endif
#endif

/**********************************************************************/
/*
macro: USER_DEFINE_INTTYPES

Standard type names (e.g. uint16_t etc.)::

These are standard names in ISO C99 defined in inttypes.h. If your
compiler is C99 compliant or nearly so, it should pick this up
automatically.

For archaic ISO C89 compilers (Windows and *old* GCC) it will attempt to
generate these types using typefromlimits.h and C89 standard 
header limits.h.

If your compiler is not C99 compliant but nevertheless has a good
inttypes.h header available, then define HAVE_INT_TYPES_H to 1

Finally if you are providing your own definitions for these types.
Define USER_DEFINE_INTTYPES to 1 and provide your own definitions in
user_types.h.

Leaving the compiler to sort it out from default values is likely to
be far more portable, but defining your own may be cleaner and
easier for deeply embedded builds.

See acnstdtypes.h for more info.
*/
/**********************************************************************/

#ifndef USER_DEFINE_INTTYPES
#define USER_DEFINE_INTTYPES 0
#endif

/**********************************************************************/
/*
macros: Posix and Gnu source

System information. These almost certainly onfigure themselves so 
you don't need to.

ACN_POSIX - System is POSIX compliant or nearly so
CONFIG_ACN_GNU - GNU extensions are available
*/
/**********************************************************************/
#ifndef ACN_POSIX
#if defined(_POSIX_C_SOURCE)
#define ACN_POSIX _POSIX_C_SOURCE
#elif defined(__linux__) || defined(__linux) ||  defined(__unix__) ||  defined(__unix)
#define ACN_POSIX 1
#else
#define ACN_POSIX 0
#endif
#endif

#ifndef CONFIG_ACN_GNU
#if CONFIG_GNUCC
#define CONFIG_ACN_GNU 1
#define _GNU_SOURCE 1
#else
#define CONFIG_ACN_GNU 0
#endif
#endif

/**********************************************************************/
/*
about: Networking

Select the underlying transport and stack.
*/
/**********************************************************************/

/*
	macros: Transport selection
	
	IP version or other transport
	
	Picking more than one makes code more complex so rely on IPv6 and a 
	hybrid stack if you can. However, this isn't so well tested.
	
	CONFIG_NET_IPV4 - IP version 4, the default
	CONFIG_NET_IPV6 - IP version 6. Partially tested
*/
#ifndef CONFIG_NET_IPV4
#define  CONFIG_NET_IPV4  1
#endif

#ifndef CONFIG_NET_IPV6
#define  CONFIG_NET_IPV6  0
#endif

/*
	macros: Network stack and API
	
	Pick just one. Because an option is here doesn't mean it works. 
	Currently the only stack tested is BSD
	
	CONFIG_STACK_BSD - BSD sockets
	CONFIG_STACK_WIN32 - Winsock sockets
	CONFIG_STACK_LWIP - LightweightIP (LWIP) stack

*/

/* BSD sockets */
#ifndef CONFIG_STACK_BSD
#define  CONFIG_STACK_BSD       0
#endif
/* Winsock sockets */
#ifndef CONFIG_STACK_WIN32
#define  CONFIG_STACK_WIN32   0
#endif
/* LightweightIP (LWIP) stack */
#ifndef CONFIG_STACK_LWIP
#define  CONFIG_STACK_LWIP     0
#endif

/*
	macro: STACK_RETURNS_DEST_ADDR
	
	Filter by incoming address
	
	If the stack has the ability to return the (multicast) destination
	address of incoming packets then RLP may be able to filter and demultiplex
	them by multicast group. Otherwise, the filtering is only done
	by port and callbacks to the same socket will get all socket messages
	regardless of the mulitcast address registered. This may or may not 
	be significantly more efficient.
	
	See platform/linux.netxface.c for
	discussion of issues with multicast addresses.
*/

#ifndef STACK_RETURNS_DEST_ADDR
#if (CONFIG_STACK_CYGWIN || CONFIG_STACK_PATHWAY)
/*
These are broken stacks!
*/
#define STACK_RETURNS_DEST_ADDR 0
#else
#define STACK_RETURNS_DEST_ADDR 1
#endif
#endif

/*
	macro: CONFIG_LOCALIP_ANY
	
	Specifying interfaces
	
	In hosts with multiple interfaces (including the loopback interface)
	it is normal to accept packets received on any interface and to leave
	it to the stack to select the interface for outgoing packets - in BSD
	this is done by binding sockets to INADDR_ANY, other stacks have
	similar mechanisms, or may even be incapable of any other behavior.
	
	If CONFIG_LOCALIP_ANY is set, RLP and SDT rely entirely on the stack
	to handle IP addresses and interfaces, the API does not allow local
	interfaces to be specified and only stores port information. This
	saves using resources tracking redundant interface information.
	
	If CONFIG_LOCALIP_ANY is false then the API allows higher layers to
	specify individual interfaces (by their address) at the expense of
	slightly more code and memory. This setting still allows the value
	NETI_INADDR_ANY to be used as required.
*/

#ifndef CONFIG_LOCALIP_ANY
#define  CONFIG_LOCALIP_ANY       1
#endif

/*
	macro: CONFIG_MULTICAST_TTL
	
	IP multicast TTL value
	
	Note the Linux manual (man 7 ip) states "It is very important for
	multicast packets  to set the smallest TTL possible" but this
	conflicts with rfc2365 and SLP defaults to 255.
	We follow the RFC thinking by default - many routers will not pass
	multicast without explicit configuration anyway.
*/
#ifndef CONFIG_MULTICAST_TTL
#define CONFIG_MULTICAST_TTL 255
#endif

/*
	macro: CONFIG_JOIN_TX_GROUPS
	
	Joining our Own Multicast Groups
	
	Ideally we don't want to join all the outgoing groups we transmit on
	as this just means we get our own messages back. However, joining a
	group prompts the stack to emit the correct IGMP messages for group
	subscription and unless we do that, many switches will block any
	messages we transmit to that group. So turn this on by default.
*/
#ifndef CONFIG_JOIN_TX_GROUPS
#define CONFIG_JOIN_TX_GROUPS 1
#endif

/**********************************************************************/
/*
	macros: Logging

	Logging options

	These are currently compile-time options so logging cannot be changed
	in running code

	CONFIG_ACNLOG - determine how messages are logged.
	CONFIG_LOGLEVEL - determine what level of messages are logged.

	Options for *CONFIG_ACNLOG* are

	ACNLOG_OFF      - All logging is compiled out
	ACNLOG_SYSLOG   - Log using POSIX Syslog
	ACNLOG_STDOUT   - Log to standard output (default)
	ACNLOG_STDERR   - Log to standard error

	Syslog handles logging levels itself and CONFIG_LOGLEVEL is ignored.
	For other options Messages up to CONFIG_LOGLEVEL are logged & levels
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
	CONFIG_LOGLEVEL was LOG_INFO or higher:

	> acn_log(LOG_RLP | LOG_INFO, "I do not like errors");
*/
/**********************************************************************/

/*
Targets for logging
*/
#define ACNLOG_OFF   0
#define ACNLOG_SYSLOG 1
#define ACNLOG_STDOUT 2
#define ACNLOG_STDERR 3

/* Default to stdout */
#ifndef CONFIG_ACNLOG
#define CONFIG_ACNLOG ACNLOG_STDOUT
#endif

/*
Value to turn logging off
*/
#define LOG_OFF (-1)

/*
define our default facility for LOG_ON
Facilities are only relevant when using syslog
*/
#ifndef LOG_ON
#if CONFIG_ACNLOG == ACNLOG_SYSLOG
#define LOG_ON LOG_USER
#else
#define LOG_ON 0
#endif
#endif

/*
Log level defaults to LOG_CRIT
*/
#ifndef CONFIG_LOGLEVEL
#define CONFIG_LOGLEVEL LOG_CRIT
#endif

/*
	macro: CONFIG_LOGFUNCS
	
	Log function entry and exit

	Useful for deep debugging but very verbose, function start (and 
	end) logging gets its own config so it can be turned off separately.
*/
#ifndef CONFIG_LOGFUNCS
#define CONFIG_LOGFUNCS ((LOG_ON) | LOG_DEBUG)
#endif

/*
macros: Log settings for ACN modules

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
*/
#ifndef LOG_RLP
	#define LOG_RLP LOG_OFF
#endif
#ifndef LOG_SDT
	#define LOG_SDT LOG_OFF
#endif
#ifndef LOG_NETX
	#define LOG_NETX LOG_OFF
#endif
#ifndef LOG_DMP
	#define LOG_DMP LOG_OFF
#endif
#ifndef LOG_DDL
	#define LOG_DDL LOG_OFF
#endif
#ifndef LOG_MISC
	#define LOG_MISC LOG_OFF
#endif
#ifndef LOG_EVLOOP
	#define LOG_EVLOOP LOG_OFF
#endif
#ifndef LOG_E131
	#define LOG_E131 LOG_OFF
#endif
#ifndef LOG_APP
	#define LOG_APP LOG_OFF
#endif
#ifndef LOG_SESS
	#define LOG_SESS (LOG_NOTICE | LOG_SDT)
#endif

/**********************************************************************/
/*
	macro: CONFIG_STRICT_CHECKS
	
	Extra error checking

	We can spend a lot of time checking for unlikely errors
	Turn on CONFIG_STRICT_CHECKS to enable a bunch 
	of more esoteric and paranoid tests.
*/
#ifndef CONFIG_STRICT_CHECKS
#define CONFIG_STRICT_CHECKS 0
#endif

/*
	macro: RECEIVE_DEST_ADDRESS

	See <STACK_RETURNS_DEST_ADDR> and <netxface.h>
*/

#ifndef RECEIVE_DEST_ADDRESS
#define RECEIVE_DEST_ADDRESS (STACK_RETURNS_DEST_ADDR && CONFIG_STRICT_CHECKS)
#endif

/**********************************************************************/
/*
	about: Component Model
*/
/**********************************************************************/
/*
	macro: CONFIG_SINGLE_COMPONENT

	One or many components?

	A large number of applications (probably the majority) only 
	define a single component, so including code to register and 
	maintain many of them is wasteful. If your application 
	implements a single component then set this true.
*/
#ifndef CONFIG_SINGLE_COMPONENT
#define  CONFIG_SINGLE_COMPONENT   0
#endif

/*
	macros: Derivatives of CONFIG_SINGLE_COMPONENT

	These are used for inline conditionals. They expand to X or 
	disappear altogether depending on CONFIG_SINGLE_COMPONENT
	
	if_ONECOMP(x) - expand if CONFIG_SINGLE_COMPONENT is true
	if_MANYCOMP(x) - expand if CONFIG_SINGLE_COMPONENT is false
*/

#if CONFIG_SINGLE_COMPONENT
#define if_ONECOMP(...) __VA_ARGS__
#define if_MANYCOMP(...)
#else
#define if_ONECOMP(...)
#define if_MANYCOMP(...) __VA_ARGS__
#endif

/*
	macros: Component name strings
	
	FCTN & UACN

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

	ACN_FCTN_SIZE - Length (in bytes) assigned for FCTN
	ACN_UACN_SIZE - Length (in bytes) assigned for UACN
*/

#ifndef ACN_FCTN_SIZE
#define ACN_FCTN_SIZE 128  /* arbitrary */
#endif
#ifndef ACN_UACN_SIZE
#define ACN_UACN_SIZE 190  /* allow for null terminator */
#endif

/**********************************************************************/
/*
	macro: CONFIG_MARSHAL_INLINE

	Data Marshalling

	Inline functions for marshaling data are efficient and typecheck
	the code. If the compiler supports inline code then they are
	preferable.

	If you do not want to compile inline, then setting this false 
	uses macros instead, but these eveluate their arguments multiple 
	times and do not check their types so beware.
*/
/**********************************************************************/

#ifndef CONFIG_MARSHAL_INLINE
#define CONFIG_MARSHAL_INLINE 1
#endif

/**********************************************************************/
/*
	UUID tracking
*/
/**********************************************************************/
/*
Options for primary UUID tracking
*/
#define UUIDS_HASH 1
#define UUIDS_RADIX 2

#ifndef CONFIG_UUIDTRACK
#define CONFIG_UUIDTRACK UUIDS_RADIX
#endif

/*
Some tracking methods allow inlining of all functions
Define CONFIG_UUIDTRACK_INLINE to enable
*/
#ifndef CONFIG_UUIDTRACK_INLINE
#define CONFIG_UUIDTRACK_INLINE 0
#endif

#if CONFIG_UUIDTRACK == UUIDS_HASH
/*
Component hash table sizes see uuid.h
*/
#ifndef CONFIG_R_HASHBITS
#define CONFIG_R_HASHBITS   7
#endif

#ifndef CONFIG_L_HASHBITS
#define CONFIG_L_HASHBITS   3
#endif
#endif /* CONFIG_UUIDTRACK == UUIDS_HASH */

/**********************************************************************/
/*
Use ACN provided event loop and timing services?
*/
/**********************************************************************/
#ifndef CONFIG_ACN_EVLOOP
#define CONFIG_ACN_EVLOOP 1
#endif

#define TIME_ms 1
#define TIME_POSIX_timeval 2
#define TIME_POSIX_timespec 3

#ifndef CONFIG_TIMEFORMAT
#define CONFIG_TIMEFORMAT TIME_ms
#endif

/**********************************************************************/
/*
	Root Layer Protocol
*/
/**********************************************************************/
#if CONFIG_RLP

/*
	The default is to build a generic RLP for multiple client protocols.
	However, efficiency gains can be made if RLP is built for only one
	client protocol (probably SDT or E1.31), in this case set
	CONFIG_RLP_SINGLE_CLIENT to 1 and define the the protocol ID of that
	client (in acncfg_local.h) as CONFIG_RLP_CLIENTPROTO
	
	e.g. For E1.31 only support
		#define CONFIG_RLP_SINGLE_CLIENT 1
		#define CONFIG_RLP_CLIENTPROTO E131_PROTOCOL_ID
*/
#ifndef CONFIG_RLP_SINGLE_CLIENT
#define CONFIG_RLP_SINGLE_CLIENT 0
#endif

#if CONFIG_RLP_SINGLE_CLIENT && !defined(CONFIG_RLP_CLIENTPROTO)
#define CONFIG_RLP_CLIENTPROTO SDT_PROTOCOL_ID
#endif

#if CONFIG_RLP_SINGLE_CLIENT
#define if_RLP_ONECLIENT(...) __VA_ARGS__
#define if_RLP_MANYCLIENT(...)
#else
#define if_RLP_ONECLIENT(...)
#define if_RLP_MANYCLIENT(...) __VA_ARGS__
#endif

/*
Maximum number of client protocols RLP can deal with
- typically very few needed
*/
#ifndef MAX_RLP_CLIENT_PROTOCOLS
#define MAX_RLP_CLIENT_PROTOCOLS (CONFIG_SDT + CONFIG_E131)
#endif

/*
	If we want RLP to optimize PDU packing for size of packets (instead of
	handling speed) define CONFIG_RLP_OPTIMIZE_PACK true.
*/
#ifndef CONFIG_RLP_OPTIMIZE_PACK
#define CONFIG_RLP_OPTIMIZE_PACK 0
#endif

#endif  /* CONFIG_RLP */

/**********************************************************************/
/*
	SDT
*/
/**********************************************************************/
#if CONFIG_SDT

/*
	Client protocols

	The default is to build a generic SDT for multiple client protocols.
	However, efficiency gains can be made if SDT is built for only one
	client protocol (probably DMP), in this case set
	CONFIG_SDT_SINGLE_CLIENT to 1 and define the the protocol ID of that
	client (in acncfg_local.h) as CONFIG_SDT_CLIENTPROTO
	
	e.g.
		#define CONFIG_SDT_SINGLE_CLIENT 1
		#define CONFIG_SDT_CLIENTPROTO DMP_PROTOCOL_ID
*/
#ifndef CONFIG_SDT_SINGLE_CLIENT
#define CONFIG_SDT_SINGLE_CLIENT 0
#endif

#if CONFIG_SDT_SINGLE_CLIENT && !defined(CONFIG_SDT_CLIENTPROTO)
#define CONFIG_SDT_CLIENTPROTO DMP_PROTOCOL_ID
#endif

#if CONFIG_SDT_SINGLE_CLIENT
#define if_SDT_ONECLIENT(...) __VA_ARGS__
#define if_SDT_MANYCLIENT(...)
#else
#define if_SDT_ONECLIENT(...)
#define if_SDT_MANYCLIENT(...) __VA_ARGS__
#endif

#ifndef CONFIG_MAX_SDT_CLIENTS
#define CONFIG_MAX_SDT_CLIENTS 4
#endif

#ifndef CONFIG_RX_AUTOCALL
#define CONFIG_RX_AUTOCALL 1
#endif

#endif  /* CONFIG_SDT */

/**********************************************************************/
/*
	DMP
*/
/**********************************************************************/
#if CONFIG_DMP

#ifndef DMP_MAX_SUBSCRIPTIONS
#define DMP_MAX_SUBSCRIPTIONS       100
#endif

/*
Many components need to implement both device and controller functions,
but if they only do one or the other then a bunch of code can be omitted

At least one of CONFIG_DMP_DEVICE and CONFIG_DMP_CONTROLLER must be set
both may be set.
*/

#ifndef CONFIG_DMP_DEVICE
#define CONFIG_DMP_DEVICE 1
#endif

#ifndef CONFIG_DMP_CONTROLLER
#define CONFIG_DMP_CONTROLLER 1
#endif

/*
For DMP devices where it is known that all addresses fall within a 
one or two byte range, Creducing ONFIG_DMPAD_MAXBYTES to 1 or 2
may enable some simplifications.
*/
#ifndef CONFIG_DMPAD_MAXBYTES
#define CONFIG_DMPAD_MAXBYTES 4
#endif

#ifndef CONFIG_DMPISONLYCLIENT
#define CONFIG_DMPISONLYCLIENT (\
						CONFIG_SDT_SINGLE_CLIENT\
						&& CONFIG_SDT_CLIENTPROTO == DMP_PROTOCOL_ID\
						)
#endif

/*
DMP usually uses property maps which can be generated from DDL 
descriptions to screen for bad addresses, a number of 
access issues, and to tie incoming messages to device properties.

CONFIG_DMPMAP_SEARCH uses a binary search to identify the property 
associated with an address. This is the most generic form suitable 
for use in general purpose controllers or code serving multiple 
complex devices.

CONFIG_DMPMAP_INDEX uses a simple array of property pointers uses 
the address as an index into this array. It is faster and simpler 
but only suitable for applications where all device types are known 
and whose property addresses are packed close enough to fit within a 
directly addressed array. This is ideal for many device 
implementations wher there is just one known property map. It is 
also good for controllers which are restricted to specific known 
devices which fit the same model.

CONFIG_DMPMAP_NONE eliminates all address checking code in the DMP 
layer and simply passes up all addresses unchecked to the application.

Finally for a single device or a controller matched to a single 
device type, defining CONFIG_DMPMAP_NAME to the name of the 
statically defined map structure will save a lot of passing pointers 
and references to maps around and allow compiler optimizations. 
Unusually for config parameters CONFIG_DMPMAP_NAME should not be 
defined at all if not used rather than being defined to 0.
*/

#if !defined(CONFIG_DMPMAP_NONE)
#define CONFIG_DMPMAP_NONE  0
#endif

#if !defined(CONFIG_DMPMAP_INDEX)
#define CONFIG_DMPMAP_INDEX 0
#endif

#if !defined(CONFIG_DMPMAP_SEARCH)
#if (CONFIG_DMPMAP_NONE + CONFIG_DMPMAP_INDEX) == 0
#define CONFIG_DMPMAP_SEARCH 1
#else
#define CONFIG_DMPMAP_SEARCH 0
#endif
#endif

#if (CONFIG_DMPMAP_SEARCH + CONFIG_DMPMAP_INDEX + CONFIG_DMPMAP_NONE) > 1

#error Must define just one of CONFIG_DMPMAP_xxx options

#endif  /* CONFIG_DMPMAP_SEARCH + CONFIG_DMPMAP_INDEX + CONFIG_DMPMAP_NONE) > 1 */

#endif  /* CONFIG_DMP */

/**********************************************************************/
/*
	DDL

	DDL Parsing is rather open ended. We have various levels starting at
	a basic parse which extracts only DMPproperty map - even this level
	needs to support parameters and includedevs
*/
/**********************************************************************/
#if CONFIG_DDL

#ifndef CONFIG_DDL_CHAR_T
#define CONFIG_DDL_CHAR_T unsigned char
#endif

#ifndef CONFIG_DDL_BASIC
#define CONFIG_DDL_BASIC   0
#endif

/*
DDL describes how to access network devices using an access protocol.
It is currently defined for two access protocols, DMP and EPI26 (E1.31/DMX512)
and may be extended to others.
*/
#ifndef CONFIG_DDLACCESS_DMP
#define CONFIG_DDLACCESS_DMP   1
#endif

#ifndef CONFIG_DDLACCESS_EPI26
#define CONFIG_DDLACCESS_EPI26  0
#endif

/*
If the application can only handle a single access protocol then defining
CONFIG_DDLACCESS_SINGLE simplifies the code
*/
#ifndef CONFIG_DDLACCESS_SINGLE
#define CONFIG_DDLACCESS_SINGLE   (CONFIG_DDLACCESS_DMP + CONFIG_DDLACCESS_EPI26 == 1)
#endif

/*
The DDL parser process builds a tree of DDL properties
as it goes and then extracts the property map from
this tree. If DDL_KEEPTREE is true, the tree is preserved
after parsing and is passed to the application. Otherwise it is
freed after the property map is generated
*/
#ifndef CONFIG_DDL_KEEPTREE
#define CONFIG_DDL_KEEPTREE   1
#endif

/*
IF DDL_BEHAVIORS is not set then behaviors are ignored
*/
#ifndef CONFIG_DDL_BEHAVIORS
#define CONFIG_DDL_BEHAVIORS   1
#endif

/*
Behaviorflags adds flags to the property map which are
set by some basic behaviors such as constant, vloatile, persistent etc.
These flags can be handled even in BASIC mode but require behavior
processing to be turned on
*/
#ifndef CONFIG_DDL_BEHAVIORFLAGS
#define CONFIG_DDL_BEHAVIORFLAGS   1
#endif

/*
DDL_BEHAVIORTYPES adds an enumeration to the information for each property
which specifies the datatype/encoding. This is generated by the behaviors
of the property
*/
#ifndef CONFIG_DDL_BEHAVIORTYPES
#define CONFIG_DDL_BEHAVIORTYPES   1
#endif

/*
Parse and record values for immediate properties
*/
#ifndef CONFIG_DDL_IMMEDIATEPROPS
#define CONFIG_DDL_IMMEDIATEPROPS   1
#endif

/*
Parse and record implied properties
*/
#ifndef CONFIG_DDL_IMPLIEDPROPS
#define CONFIG_DDL_IMPLIEDPROPS   1
#endif

#ifndef CONFIG_DDL_LABELS
#define CONFIG_DDL_LABELS   0
#endif

/*
Maximum XML nesting level within a single DDL module
*/
#ifndef CONFIG_DDL_MAXNEST
#define CONFIG_DDL_MAXNEST 256
#endif

/*
Size allocated for parsing text nodes
FIXME: this should be done more elegantly - behaviorset::p nodes can get
big
*/

#ifndef CONFIG_DDL_MAXTEXT
#define CONFIG_DDL_MAXTEXT 512
#endif

#endif /* CONFIG_DDL */
/**********************************************************************/
/*
	E1.31

	We have separate configures for transmit and receive as they are
	frequently different components and do not share much code.
*/
/**********************************************************************/
#if CONFIG_E131

#ifndef CONFIG_E131_RX
#define CONFIG_E131_RX         1
#endif

#ifndef CONFIG_E131_TX
#define CONFIG_E131_TX         1
#endif

/*
	Follow generic memory rules by default
*/
#ifndef CONFIG_E131_MEM
#define CONFIG_E131_MEM        CONFIG_MEM
#endif

/*
	Set CONFIG_E131_ZSTART_ONLY true to have the code drop ASC packets on
	read and automatically add a zero start on write
*/
#ifndef CONFIG_E131_ZSTART_ONLY
#define CONFIG_E131_ZSTART_ONLY        0
#endif

/*
	Set CONFIG_E131_IGNORE_PREVIEW true to have the code drop preview
	packets on read and never set preview on write. All the while PREVIEW
	is the only option flag apart from Terminate, this also cuts passing
	of options to and from the app altogether.
*/
#ifndef CONFIG_E131_IGNORE_PREVIEW
#define CONFIG_E131_IGNORE_PREVIEW     0
#endif

#ifndef E131MEM_MAXUNIVS
#define E131MEM_MAXUNIVS     4
#endif

#endif  /* CONFIG_E131 */

/**********************************************************************/
/*
	Sanity checks

	The following are sanity checks on preceding options and some
	derivative configuration values. They are not user options
*/
/**********************************************************************/
/*
	check on transport selection
*/
#if (CONFIG_NET_IPV4 + CONFIG_NET_IPV6) <= 0
#error Need to select at least one transport
#elif (CONFIG_NET_IPV4 + CONFIG_NET_IPV6) == 1
#define CONFIG_MULTI_NET 0
#else
#define CONFIG_MULTI_NET 1
#endif

/*
	checks on network stack
*/
#if (CONFIG_STACK_BSD + CONFIG_STACK_WIN32 + CONFIG_STACK_PATHWAY + CONFIG_STACK_LWIP + CONFIG_STACK_NETBURNER + CONFIG_STACK_CYGWIN) != 1
#error Need to select exactly one network stack
#endif

#if CONFIG_STACK_PATHWAY && (!CONFIG_NET_IPV4 || CONFIG_MULTI_NET)
#error Pathway stack only supports IPv4
#endif

/*
	checks for CONFIG_RLP_SINGLE_CLIENT
*/
#if ((CONFIG_SDT + CONFIG_E131) > 1 && (CONFIG_RLP_SINGLE_CLIENT) != 0)
#error Cannot support both SDT and E1.31 if CONFIG_RLP_SINGLE_CLIENT is set
#endif

/*
If we,re doing DMP we must have a device or a controller (or both)
*/
#if CONFIG_DMP && (CONFIG_DMP_DEVICE + CONFIG_DMP_CONTROLLER == 0)
#error Must set either DMP_DEVICE or DMP_CONTROLLER or both
#endif

#if CONFIG_DMPAD_MAXBYTES != 4 && CONFIG_DMPAD_MAXBYTES != 2 && CONFIG_DMPAD_MAXBYTES != 1
#error CONFIG_DMPAD_MAXBYTES must be 1, 2 or 4
#endif

#endif /* __acncfg_h__ */
