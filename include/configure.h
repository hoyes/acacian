/**********************************************************************/
/*
	Copyright (c) 2007-2010, Engineering Arts (UK)

#tabs=3t
*/
/**********************************************************************/

#ifndef __acncfg_h__
#define __acncfg_h__ 1

/**********************************************************************/
/*
header: acncfg.h

Configuration Definitions
*/
/**********************************************************************/
/*
macros: Version

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
/**********************************************************************/
#define ACNCFG_VERSION 20100000

/**********************************************************************/
/*
	macros: Network and Transport Features
	
	IP version (or other transport)

	ACNCFG_NET_IPV4 - IP version 4
	ACNCFG_NET_IPV6 - IP version 6 (experimental)

	Picking more than one makes code more complex so rely on IPv6 and a 
	hybrid stack if you can. However, this isn't so well tested.
	
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

// #define  ACNCFG_NET_IPV4  1
#define  ACNCFG_NET_IPV6  1
#define  ACNCFG_LOCALIP_ANY       1
#define ACNCFG_MULTICAST_TTL 255
#define ACNCFG_JOIN_TX_GROUPS 1

// #define RECEIVE_DEST_ADDRESS 1

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

#define ACNCFG_ACNLOG ACNLOG_STDOUT
#define LOG_OFF (-1)

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

#define ACNCFG_LOGLEVEL LOG_DEBUG
#define ACNCFG_LOGFUNCS ((LOG_OFF) | LOG_DEBUG)

#define LOG_RLP LOG_OFF
#define LOG_SDT LOG_OFF
#define LOG_NETX LOG_OFF
#define LOG_DMP LOG_OFF
#define LOG_DDL LOG_OFF
#define LOG_MISC LOG_OFF
#define LOG_EVLOOP LOG_OFF
#define LOG_E131 LOG_OFF
#define LOG_APP LOG_OFF
#define LOG_SESS LOG_OFF

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
#define ACNCFG_MARSHAL_INLINE 1

/**********************************************************************/
/*
	macros: Error Checking
	
	ACNCFG_STRICT_CHECKS - Extra error checking

	We can spend a lot of time checking for unlikely errors
	Turn on ACNCFG_STRICT_CHECKS to enable a bunch 
	of more esoteric and paranoid tests.
*/
// #define ACNCFG_STRICT_CHECKS 1

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

#define ACNCFG_MULTI_COMPONENT   1
#define ACN_FCTN_SIZE 128  /* arbitrary */
#define ACN_UACN_SIZE 190  /* allow for null terminator */

/**********************************************************************/
/*
	macros: UUID tracking

	ACN uses UUIDs extensively and various structures including 
	components and DDL modules are indexed by UUID. eaACN implements 
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

	ACNCFG_UUIDTRACK_INLINE - Use inline functions to speed UUID tracking

	The number of functions are implemented inline and speed/memory
	trade-offs vary.
*/

#define ACNCFG_UUIDS_RADIX 1
// #define ACNCFG_UUIDS_HASH 1
// #define ACNCFG_R_HASHBITS   7
// #define ACNCFG_L_HASHBITS   3
// #define ACNCFG_UUIDTRACK_INLINE 1

/**********************************************************************/
/*
	macros: Event loop and timing

	ACNCFG_EVLOOP - Use ACN provided event loop and timing services

	eaACN provides a single threaded event loop using epoll. The application
	can register its own events in this loop if desired. Turn this off 
	to provide an alternative model

	CNCFG_TIME_ms - Use simple millisecond integers for times
	CNCFG_TIME_POSIX_timeval - Use POSIX timeval struictures for timing
	CNCFG_TIME_POSIX_timespec - Use timespec struictures for timing

	Millisecond counters are adequate (just) for SDT specifications.
*/

#define ACNCFG_EVLOOP 1
#define CNCFG_TIME_ms 1
// #define CNCFG_TIME_POSIX_timeval 1
// #define CNCFG_TIME_POSIX_timespec 1

/**********************************************************************/
/*
	macros: Root Layer Protocol

	ACNCFG_RLP - enable the ACN root layer
	
	Root layer is needed for UDP but may not be needed for other 
	transports.

	ACNCFG_RLP_CLIENTPROTO - Client protocol for single protocol 
	implementations

	The default is to build a generic RLP for multiple client protocols
	However, efficiency gains can be made if RLP is built for only one
	client protocol (probably SDT or E1.31), in this case set
	ACNCFG_RLP_CLIENTPROTO to the protocol ID of that
	client
	
	e.g. For E1.31 only support
		#define ACNCFG_RLP_CLIENTPROTO E131_PROTOCOL_ID
	
	or for SDT only support
		#define ACNCFG_RLP_CLIENTPROTO SDT_PROTOCOL_ID

	ACNCFG_MAX_RLP_CLIENTS - Number of client protocols to allocate
	space for
	
	Typically very few client protocols are used
	
	ACNCFG_RLP_OPTIMIZE_PACK - Optimize PDU packing in RLP (ath the cost of speed)
*/

#define ACNCFG_RLP     1
#define ACNCFG_MAX_RLP_CLIENTS 2
// #define ACNCFG_RLP_OPTIMIZE_PACK 1

/**********************************************************************/
/*
	macros: SDT

	ACNCFG_SDT - enable the SDT layer
	
	ACNCFG_SDT_CLIENTPROTO - Client protocol for single protocol 
	implementations
	
	The default is to build a generic SDT for multiple client protocols.
	However, efficiency gains can be made if SDT is built for only one
	client protocol (probably DMP), in this case set
	ACNCFG_SDT_CLIENTPROTO to the protocol ID of that
	client.

	e.g.
		#define ACNCFG_SDT_CLIENTPROTO DMP_PROTOCOL_ID

	ACNCFG_MAX_SDT_CLIENTS - Number of client protocols to allocate
	space for
	
	Typically very few client protocols are used

*/

#define ACNCFG_SDT     1
#define ACNCFG_SDT_CLIENTPROTO DMP_PROTOCOL_ID
#define ACNCFG_MAX_SDT_CLIENTS 4

/**********************************************************************/
/*
	macros: DMP

	ACNCFG_DMP - enable the DMP layer

	ACNCFG_DMP_DEVICE - Enable DMP device support
	ACNCFG_DMP_CONTROLLER - Enable DMP controller support

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
	ACNCFG_DMPMAP_NAME - For a single device or a controller matched to a single 
	device type, define to the name of the 
	statically defined map structure to save a lot of passing pointers 
	and references. 
*/

#define ACNCFG_DMP     1
#define DMP_MAX_SUBSCRIPTIONS       100
#define ACNCFG_DMP_DEVICE 1
#define ACNCFG_DMP_CONTROLLER 1
#define ACNCFG_DMPAD_MAXBYTES    4
// #define ACNCFG_DMPMAP_NONE  1
// #define ACNCFG_DMPMAP_INDEX 1
#define ACNCFG_DMPMAP_SEARCH 1

/**********************************************************************/
/*
	Dmacros: DL
	
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

#define ACNCFG_DDL 	   1
#define ACNCFG_DDLACCESS_DMP   1
// #define ACNCFG_DDLACCESS_EPI26  1
#define ACNCFG_DDL_BEHAVIORS   1

#define ACNCFG_DDL_BEHAVIORFLAGS   1
#define ACNCFG_DDL_BEHAVIORTYPES   1
#define ACNCFG_DDL_IMMEDIATEPROPS   1
#define ACNCFG_DDL_IMPLIEDPROPS   1
#define ACNCFG_DDL_MAXNEST 256

/*
FIXME: this should be done more elegantly - behaviorset::p nodes can get
big
*/
#define ACNCFG_DDL_MAXTEXT 512

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

// #define ACNCFG_E131            1
// #define ACNCFG_E131_RX         1
// #define ACNCFG_E131_TX         1
// #define E131MEM_MAXUNIVS       4
// #define ACNCFG_E131_ZSTART_ONLY        1
// #define ACNCFG_E131_IGNORE_PREVIEW     1

/**********************************************************************/
/*
macros: EPIs

Conformance to specific EPIs. This is the complete set of EPIs for 
ACN 2010. They are defined (or not) for completeness but some have 
no effect.

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
ACNCFG_EPI29 - IPv4 address assignment
*/
#define  ACNCFG_EPI10   1
#define  ACNCFG_EPI11   1
#define  ACNCFG_EPI12   1
#define  ACNCFG_EPI15   1
#define  ACNCFG_EPI16   1
#define  ACNCFG_EPI17   1
#define  ACNCFG_EPI18   1
#define  ACNCFG_EPI19   1
#define  ACNCFG_EPI20   1
#define  ACNCFG_EPI29   1

#endif /* __acncfg_h__ */
