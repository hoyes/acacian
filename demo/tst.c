/*
#tabs=3t
*/
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include "acn.h"
#include "getip.h"

extern const char *gipfnames[32];

/**********************************************************************/
int
main(int argc, char *argv[])
{
	int i;
	char *ifcs[32];
	uint32_t fmask, fmatch;
	char **ifcp;
	char **ipstrs;
	char **ipstrp;

	fmask = fmatch = GIPF_DEFAULT;
	ifcp = ifcs;
	while ((i = getopt(argc, argv, "i:m:f:n:")) > 0) switch (i) {
		case 'i':
			*ifcp++ = strdup(optarg);
			break;
		case 'm':
			fmask = strtoul(optarg, NULL, 0);
			break;
		case 'f':
			fmatch = strtoul(optarg, NULL, 0);
			break;
		default:
			fprintf(stderr, "Bad flag\n");
			break;
	}
	*ifcp = NULL;

	for (ifcp = ifcs; *ifcp; ++ifcp)
		printf("ifc %ld is \"%s\"\n", ifcp - ifcs, *ifcp);

	if (fmask == GIPF_DEFAULT) printf("default mask\n");
	else {
		printf("mask is:");
		for (i = 0; i < 32; ++i)
			if (fmask & GIPF_ALL & (1 << i)) printf(" %s", gipfnames[i]);
		printf("\n");
	}
	if (fmatch == GIPF_DEFAULT) printf("default match\n");
	else {
		printf("\nmatch is:");
		for (i = 0; i < 32; ++i)
			if (fmatch & GIPF_ALL & (1 << i)) printf(" %s", gipfnames[i]);
		printf("\n");
	}

	ifcp = ifcs;
	if (*ifcp == NULL) ifcp = NULL;

	ipstrs = netx_getmyipstr(ifcp, fmask, fmatch, ACNCFG_MAX_IPADS);
	i = 0;
	for (ipstrp = ipstrs; *ipstrp; ++ipstrp) {
		printf("Address %d: %s\n", ++i, *ipstrp);
	}
	netx_freeipstr(ipstrs);
	return 0;
}
