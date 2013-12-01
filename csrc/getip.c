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
file: getip.c

Utilities to find our own IP addresses
*/

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
 #include <sys/ioctl.h>
#include <net/if.h>
#include <stdbool.h>
#include <fnmatch.h>

#include "acn.h"
#include "getip.h"

#define NREQS 50

/**********************************************************************/
#define lgFCTY LOG_NETX
//#define lgFCTY LOG_OFF

/**********************************************************************/
#if ACNCFG_OS_LINUX

const char *gipfnames[32] = {
	[clog2(GIPF_UP)]           = "UP",
	[clog2(GIPF_BROADCAST)]    = "BROADCAST",
	[clog2(GIPF_LOOPBACK)]     = "LOOPBACK",
	[clog2(GIPF_POINTOPOINT)]  = "POINTOPOINT",
	[clog2(GIPF_PROMISC)]      = "PROMISC",
	[clog2(GIPF_ALLMULTI)]     = "ALLMULTI",
	[clog2(GIPF_MULTICAST)]    = "MULTICAST",
	[clog2(GIPF_IPv4)]         = "IPv4",
	[clog2(GIPF_IPv6)]         = "IPv6"
};

/**********************************************************************/
const uint32_t default_flagmask = (0
									| GIPF_UP
									| GIPF_LOOPBACK
									| GIPF_MULTICAST
									ifNETv4(| GIPF_IPv4)
									ifNETv6(| GIPF_IPv6)
								);
									
const uint32_t default_flagmatch = (0
									| GIPF_UP
									| GIPF_MULTICAST
									ifNETv4(| GIPF_IPv4)
									ifNETv6(| GIPF_IPv6)
								);
#define IFCMATCHFLAGS 0

/**********************************************************************/
char *
flags2str(uint32_t flagword, const char *namestrings[])
{
	static char namebuf[128];
	char * const bufend = namebuf + sizeof(namebuf) - 1;
	char *dp;
	const char *sp;

	dp = namebuf;
	while (flagword) {
		if ((flagword & 1)) {
			sp = *namestrings;
			if (sp == NULL) sp = "?";
			while (*sp && dp < bufend) *dp++ = *sp++;
			*dp++ = ' ';
		}
		++namestrings;
		flagword >>= 1;
	}
	if (dp > namebuf) --dp; /* back up over training space */
	*dp = 0;
	return namebuf;
}

/**********************************************************************/
/*
func: netx_getmyip
Return IP address(es)

Getting our IP address is non-trivial. Most personal computers have 
multiple interfaces (e.g. wired and WiFi) and frequently have 
multiple active addresses on one interface (e.g. DHCP falling back 
to Avahi). Also modern stacks often run both IPv4 and IPv6 side by 
side.

The traditional method of calling gethostname then gethostbyname 
(superseded on Linux by getaddrinfo()) relies on functioning name 
resolution which is not the case on many of the types of network 
where ACN is used.

For purposes of advertisement (SLP) we can advertise the same 
service over multiple interfaces so returning multiple addresses is 
desirable, but rules of epi29 (epi13) expect prioritization (e.g. so 
link local addresses are only advertised when no routable address is 
available).

For the multicast generation method of epi10 "the (unicast) IP 
address of the interface to be used" is required, but this is simply 
a way to reduce the probability of conflicting addresses and the 
protocol will not fail if the wrong address is used so a best guess 
is sufficient.

The strategy we use is to provide a set of filters and potentially 
return a multiple of addresses.

arguments:

	interfaces - null-terminated array of hardware interface names to 
	select. Wildcards are allowed. e.g. "eth0*" matches "eth0", 
	"eth0:1" etc. If NULL then all interfaces are considered. 
	Negative matches, preceeded by '!' are permitted, e.g. "!lo". 
	When comparing a name against the array, searching stops with 
	success at the fisrt positive match and stops with failure at the 
	first negative match. This allows partial selections using 
	wildcards by preceeding the more general wildcard by a more 
	specific one. e.g. "!eth0:foo", "eth0*" which selects any eth0 
	interface except eth0:foo.

	flagmask, flagmatch - interface flags must match according to the
	expression (interfaceflags & flagmask) == (flagmatch & 
	flagmask). See getipflag_e for available flags.

	addrlist - pointer to area to put matching addresses
	size - size in bytes of the addrlist area
*/

/**********************************************************************/
int
netx_getmyip(
	const char *interfaces[],
	uint32_t flagmask,
	uint32_t flagmatch,
	void *addrlist,
	size_t size
)
{
	netx_addr_t *adp;
	int adcount;
	int matches = 0;
	int fd = -1;

	adp = (netx_addr_t *)addrlist;
	adcount = size / sizeof(*adp);
	matches = 0;

	if ((flagmask & flagmatch) == GIPF_DEFAULT) {
		flagmask = default_flagmask;
		flagmatch = default_flagmatch;
	}

#if ACNCFG_NET_IPV4
	if ((~flagmask | flagmatch) & GIPF_IPv4) {
		struct ifreq *ifrp;
		struct ifreq ifr;
		char buf[NREQS * sizeof(struct ifreq)];
		struct ifconf ifc;
		int ifcount;
		char addrstr[128];

		fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (fd < 0) {
			acnlogerror(lgERR);
			matches = -1;
			goto fnexit;
		}

		ifc.ifc_len = sizeof(buf);
		ifc.ifc_buf = buf;
		if (ioctl(fd, SIOCGIFCONF, &ifc) < 0) {
			acnlogerror(lgERR);
			matches = -1;
			goto fnexit;
		}
		ifrp = (struct ifreq *)buf;
		ifcount = ifc.ifc_len / sizeof(struct ifreq);

		for (; ifcount-- && matches < adcount; ++ifrp) {
			const char **fp;
			const char *cp;
			bool match = false;
			uint32_t flagword;

			if (ifrp->ifr_addr.sa_family == AF_INET) flagword = GIPF_IPv4;
			else flagword = 0;

			acnlogmark(lgDBUG, "Interface %s, address %s",
				ifrp->ifr_name,
				inet_ntop(
							ifrp->ifr_addr.sa_family,
							&((struct sockaddr_in *)&ifrp->ifr_addr)->sin_addr,
							addrstr,
							sizeof(addrstr)
							)
			);

			if (interfaces == NULL || *interfaces == NULL) match = true;
			else for (fp = interfaces; (cp = *fp) != NULL; ++fp) {
				bool namematch;
				bool negmatch;

				negmatch = (*cp == '!');
				cp += negmatch;
				namematch = fnmatch(cp, ifrp->ifr_name, IFCMATCHFLAGS) == 0;

				match = namematch ^ negmatch;

				acnlogmark(lgDBUG, "  \"%s\" %s", *fp, match ? "matches" : "fails");
				/* break if we have a positive or negative match */
				if (namematch) break;
			}
			acnlogmark(lgDBUG, "  interface match %s", match ? "pass" : "fail");

			if (match) {
				strncpy(ifr.ifr_name, ifrp->ifr_name, IFNAMSIZ);
				if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
					acnlogerror(lgERR);
					matches = -1;
					goto fnexit;
				}
				flagword |= ifr.ifr_flags & GIPF_ALL;
				match = ((flagword ^ flagmatch) & flagmask) == 0;

				acnlogmark(lgDBUG, "  flag match <%s> %s",
					flags2str(flagword, gipfnames),
					(match ? "pass" : "fail")
				);
			}

			if (match) {
				memcpy(adp, &ifrp->ifr_addr, sizeof(ifrp->ifr_addr));
				++matches;
				++adp;
			}
		}
		close(fd);
		fd = -1;
	}
#endif /* ACNCFG_NET_IPV4 */
#if ACNCFG_NET_IPV6
/*
FIXME: implement for IPv6 - The method for IPv4 using ioctl calls 
does not work for IPv6. See man netdevice(7) which says "Local IPv6 
IP addresses can be found via /proc/net or via rtnetlink(7)."
*/

#endif /* ACNCFG_NET_IPV6 */
fnexit:
	if (fd >= 0) close(fd);
	acnlogmark(lgDBUG, "returning %d IP address(es)", matches);
	return matches;
}
/**********************************************************************/
#define MAX_MAXADDRS 64
/*
func: netx_getmyipstr

Get a list of IP addresses in string format.
*/
int
netx_getmyipstr(
	const char *interfaces[],
	uint32_t flagmask,
	uint32_t flagmatch,
	char **ipstrs,
	int maxaddrs
)
{
	netx_addr_t *ipads;
	int nipads;
	int i;
	char **strp;

	if (maxaddrs <= 0 || maxaddrs > MAX_MAXADDRS) {
		acnlogmark(lgERR, "bad argument %d", maxaddrs);
		return -1;
	}
	ipads = mallocx(maxaddrs * sizeof(netx_addr_t));

	/*
	Get suitable addresses
	*/
	nipads = netx_getmyip(interfaces, flagmask, flagmatch,
									(void *)ipads, maxaddrs * sizeof(netx_addr_t));
	//acnlogmark(lgDBUG, "found %d IP address(es)", nipads);
	if (nipads <= 0) {
		free(ipads);
		acnlogmark(lgWARN, "No IP address(es) found");
		return -1;
	}

	strp = ipstrs;
	for (i = 0; i < nipads; ++i) {
		char ipstr[sizeof("aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa")];

		if (!inet_ntop(netx_TYPE(ipads + i), &netx_SINADDR(ipads + i), ipstr, sizeof(ipstr))) {
			acnlogerror(lgERR);
			continue;
		}
		if (netx_TYPE(ipads + i) == AF_INET6) {
			*strp = mallocx(strlen(ipstr) + 3);
			sprintf(*strp, "[%s]", ipstr);
		} else {
			*strp = strdup(ipstr);
		}
		acnlogmark(lgDBUG, "Adding address %s", *strp);
		++strp;
	}
	free(ipads);
	return strp - ipstrs;
}

/**********************************************************************/
/*
func: netx_freeipstr

Free the strings allocated by <netx_getmyipstr>
*/
void
netx_freeipstr(char **strs)
{
	char **strp;

	for (strp = strs; *strp; ++strp) free(*strp);
	free(strs);
}
#else  /* ACNCFG_OS_LINUX */
#error "Unsupported operating system configured"
#endif  /* ACNCFG_OS_LINUX */
