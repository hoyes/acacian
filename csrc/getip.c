/*
	Copyright (c) 2010, Philip Nye, Engineering Arts (UK) philip@engarts.com

#tabs=3t
*/
/**********************************************************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "acn.h"

/**********************************************************************/
#define lgFCTY LOG_NETX

/**********************************************************************/
/*
func: netx_getmyip
*/
ip4addr_t netx_getmyip(netx_addr_t *destaddr UNUSED)
{
/*
TODO: Should find the local IP address which would be used to send to
			the given remote address. For now we just get the first address
Hint: to find all interfaces and IPv4 addresses use ioctl(s, SIOCGIFCONF, (char *)&ifc)
see man 7 netdevice for more info.

*/
	int fd;
	struct ifreq ifr;

	//UNUSED_ARG(destaddr)

	LOG_FSTART(lgFCTY);
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
	ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);

	LOG_FEND(lgFCTY);
	return ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;


/* this works too */
/*
	char    s[256];
	struct  hostent *local_host;
	struct  in_addr *in;

	UNUSED_ARG(destaddr)

	gethostname(s, 256);
	local_host = gethostbyname(s);
	if (local_host) {
		in = (struct in_addr *) (local_host->h_addr_list[0]);
		return (in->s_addr);
	}
	return 0;
*/
/* this may be better as it rerturs a list of addresses and is IPv6 compatible */
/*
	char name[128];
	in_addr_t rslt;
	struct addrinfo *aip;
	struct addrinfo *ai = NULL;

	if (gethostname(name, sizeof(name)) < 0) {
		perror("gethostname");
		exit(EXIT_FAILURE);
	}
	if (getaddrinfo(NULL, sdt_port, &hint, &ai) < 0) {
		perror("getaddrinfo");
		exit(EXIT_FAILURE);
	}
	for (aip = ai; aip != NULL; aip = aip->ai_next) {
		struct in_addr ip;
		
		ip = ((struct sockaddr_in*)(aip->ai_addr))->sin_addr;
		printf("Host: %s, address %s\n", name, inet_ntoa(ip));
		if (aip == ai) rslt = ip.s_addr;
	}
	freeaddrinfo(ai);
	return rslt;
*/
}


/************************************************************************/
/*
	netx_getmyipmask()
	Note: this only returns the fisrt one found and may not be correct if there are multple NICs
*/
#if 0
ip4addr_t netx_getmyipmask(netx_addr_t *destaddr UNUSED)
{
	int fd;
	struct ifreq ifr;
	//UNUSED_ARG(destaddr)

	LOG_FSTART(lgFCTY);
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
	ioctl(fd, SIOCGIFNETMASK, &ifr);
	close(fd);

	LOG_FEND(lgFCTY);
	return ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
}
#endif
