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
file: random.c

Random numbers are used in ACN occasionally. random.c and random.h
attempt to ensure that numbers differ from component to component and
from run to run.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "acn.h"
/**********************************************************************/
#define lgFCTY LOG_MISC
//#define lgFCTY LOG_OFF

/**********************************************************************/
/*
func: randomize

Initialize the random library function to generate a fresh sequence that
is unlikely to match other components and unlikely to repeat on another
run.

For Linux we use /dev/urandom to seed the generator.
*/
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
