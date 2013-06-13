/*
#tabs=3t
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
//#include "acn.h"

static const struct addrinfo hint = {
	.ai_flags = AI_PASSIVE,
	.ai_family = AF_UNSPEC /* AF_INET AF_INET6 AF_UNSPEC netx_FAMILY */,
	.ai_socktype = SOCK_DGRAM,
	.ai_protocol = 0
};
const char sdt_port[] = "5568";
//const char sdt_port[] = STRINGIFY(SDT_MULTICAST_PORT);

int
main(int argc, char *argv[])
{
	char nbuf[128];
	char *name;
	char addrstr[128];
	struct addrinfo *aip;
	struct addrinfo *ai = NULL;
	int rslt;

	name = argv[1];

	if (name == NULL) {
		name = nbuf;
		if (gethostname(nbuf, sizeof(nbuf)) < 0) {
			perror("gethostname");
			exit(EXIT_FAILURE);
		}
	} else if (strcmp(name, "-n") == 0) {
		name = NULL;
	}
	printf("Hostname is \"%s\"\n", name);

	if ((rslt = getaddrinfo(name, sdt_port, &hint, &ai)) < 0) {
		fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(rslt));
		exit(EXIT_FAILURE);
	}
	for (aip = ai; aip != NULL; aip = aip->ai_next) {
		switch (aip->ai_family) {
		case AF_INET:
			printf("IPv4: flags=%x, protocol=%s, canonname=\"%s\", addr=\"%s\", port=%u\n",
						aip->ai_flags,
						getprotobynumber(aip->ai_protocol)->p_name,
						aip->ai_canonname,
						inet_ntop(aip->ai_family,
									&((struct sockaddr_in *)aip->ai_addr)->sin_addr,
									addrstr,
									sizeof(addrstr)
									),
						ntohs(((struct sockaddr_in *)aip->ai_addr)->sin_port)
					);
			break;
		case AF_INET6:
			printf("IPv6: flags=%x, protocol=%s, canonname=\"%s\", addr=\"%s\", port=%u\n",
						aip->ai_flags,
						getprotobynumber(aip->ai_protocol)->p_name,
						aip->ai_canonname,
						inet_ntop(aip->ai_family,
									&((struct sockaddr_in6 *)aip->ai_addr)->sin6_addr,
									addrstr,
									sizeof(addrstr)
									),
						ntohs(((struct sockaddr_in6 *)aip->ai_addr)->sin6_port)
					);
			break;
		case AF_UNSPEC:
			printf("Unspecified family: flags=%x, protocol=%s, canonname=\"%s\"\n",
						aip->ai_flags,
						getprotobynumber(aip->ai_protocol)->p_name,
						aip->ai_canonname
					);
			break;
		default:
			printf("Unknown family %d: flags=%x, protocol=%s, canonname=\"%s\"\n", aip->ai_family,
						aip->ai_flags,
						getprotobynumber(aip->ai_protocol)->p_name,
						aip->ai_canonname
					);
			break;
		}
	}
	freeaddrinfo(ai);

	return 0;
}
