/**********************************************************************/
/*

	Copyright (c) 2011, Philip Nye, Engineering Arts (UK) philip@engarts.com
	All rights reserved.

	Author: Philip Nye

	$Id$

#tabs=3t
*/
/**********************************************************************/

#include "acn.h"

/**********************************************************************/
/*
Some helper functions
*/

/**********************************************************************/
/*
For random numbers we use random() which is a POSIX.1-2001 function 
in preference to the C standard rand() which has many very poor 
implementations. However, on systems where random() is not available 
substituting rand()/srand() is fairly trivial.
Either way, the pseudo random sequence should be seeded with 
something unpredictable which varies from instance to instance and 
from run to run. This is not an issue of security but to ensure that 
lock-step situations where two components in the system select the 
same sequence at the same time are very unlikely.
*/

void
randomize(void)
{
	unsigned int seed;
	int fd;

	if (
			(fd = open("/dev/urandom", O_RDONLY)) < 0
		|| read(fd, &seed, sizeof(seed)) <= 0
	) {
		acnlogmark(lgNTCE, "unable to randomize");
		seed = 2624393690;
	}
	if (fd > 0) close(fd);
	srandom(seed);
}

/**********************************************************************/
/*
getcid(const char *uuidstr, uuid_t cid) allocates a CID for a component.

If successful, the resulting uuid is placed in cid and getcid returns 0.

In normal operation this function reads the persistent CID from the 
file specified in cidpath. If this fails - e.g. because cidpath does 
not exist - then a new CID is allocated and stored in cidpath.

This mechanism can be overidden (e.g. by a command line option) by 
supplying a cid (in text format) in the uuidstr parameter (which should
otherwise be NULL).

Allocation of new CIDs is done by reading from the linux kernel 
supplied uuid generator. Many other systems have alternate methods 
for supplying uuids which can be substituted here. For example by 
calling the external program uuidgen or using some locally provided 
system call.
*/
static const char uuidpath[] = "/var/local/consoled_id";
static const char getuuid[]  = "/proc/sys/kernel/random/uuid";

static const char *uuidpaths[] = {
	"~/acn_Lcomp_id",
	"/var/local/acn_Lcomp_id",
	"/proc/sys/kernel/random/uuid",
};

char *
getcid(char *cidstr)
{
	int fd;
	int i;
	int try;
	const char *fname;

	for (try = 0; ; ++try) {
		if (try >= ARRAYSIZE(uuidpaths)) return NULL;
		fname = uuidpaths[try];
		if ((fd = open(fname, O_RDONLY)) >= 0) {
			i = read(fd, cidstr, UUID_STR_SIZE - 1);
			close(fd);
			if (i < 0) {
				acnlogmark(lgWARN, "read UUID %s", strerror(errno));
				*cidstr = 0;
			} else {
				cidstr[i] = 0;
				if (i == (UUID_STR_SIZE - 1) && quickuuidOKstr(buf))
					break;
				acnlogmark(lgNTCE, "invalid UUID \"%s\"", buf);
			}
		} else if (errno != ENOENT) {
			acnlogmark(lgNTCE, "open uuidfile: %s", strerror(errno));
		}
	}
	if (try == ARRAYSIZE(uuidpaths) - 1) {
		/*
		Got new dynamic CID - try and save it for next time
		*/
		fd = open(uuidpaths[0], O_WRONLY | O_CREAT | O_TRUNC, 
											S_IRUSR | S_IRGRP | S_IROTH);
		if (fd < 0) {
			acnlogmark(lgWARN, "creating ID file %s", strerror(errno));
		} else {
			cidstr[UUID_STR_SIZE - 1] = '\n';
			write(fd, buf, UUID_STR_SIZE);
			close(fd);
			cidstr[UUID_STR_SIZE - 1] = 0;
		}
	}

	return cidstr;
}

/**********************************************************************/
int
acn_start(const char *uuidstr, uint16_t dmpport)
{
	netx_addr_t addr;
	netx_addr_t *addrp = NULL;
	char cid[UUID_STR_SIZE];

	acnlogmark(lgDBUG, "Starting ACN");

	randomize();
	if (startACNmem() < 0) return -1;
	if (uuidstr == NULL && (uuidstr = getcid(cid)) == NULL)
		return -1;
	if (init_Lcomponent(&localComponent, uuidstr) < 0)
		return -1;
/*
	Register with SLP or other advertising mechanisms
*/
#if CONFIG_SDT
	sdt_startup();
#endif
	
}

/**********************************************************************/
void
acn_stop()
{
	acnlogmark(lgDBUG, "Stopping ACN");
}

