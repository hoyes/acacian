/*
	Copyright (c) 2010, Philip Nye, Engineering Arts (UK) philip@engarts.com

#tabs=3t
*/
/**********************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include "acn.h"
/**********************************************************************/
#define lgFCTY LOG_MISC
//#define lgFCTY LOG_OFF

/**********************************************************************/

void
randomize(bool force)
{
	static bool initialized = false;
	int fd;
	unsigned int seed;
	int rslt;

	if (initialized && !force) return;

	if ((fd = open("/dev/urandom", O_RDONLY)) < 0
		|| (rslt = read(fd, &seed, sizeof(seed))) < 0)
	{
		acnlogerror(lgERR);
	} else {
		srandom(seed);
		initialized = true;
	}

	if (fd >= 0) close(fd);
	
}
