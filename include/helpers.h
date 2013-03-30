/**********************************************************************/
/*

	Copyright (c) 2011, Philip Nye, Engineering Arts (UK) philip@engarts.com
	All rights reserved.

	Author: Philip Nye

	$Id$

#tabs=3t
*/
/**********************************************************************/

#if ACN_POSIX
/* on linux systems you may need to #define _XOPEN_SOURCE = 600 here */
#include <stdlib.h>
#define getrandomshort() (random() & 0xffff)
#else
#define getrandomshort() ((rand() >> 15) & 0xffff)
#endif

void randomize(void);
int getcid(uint8_t *cid, const char *uuidstr);
