/**********************************************************************/
/*

	Copyright (c) 2011, Philip Nye, Engineering Arts (UK) philip@engarts.com
	All rights reserved.

	Author: Philip Nye

	$Id$

#tabs=3t
*/
/**********************************************************************/

/* on linux systems you may need to #define _XOPEN_SOURCE = 600 here */
#include <stdlib.h>
#define getrandU16() (random() & 0xffff)

void randomize(bool force);
