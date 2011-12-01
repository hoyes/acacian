
/************************************************************************/
/*
Copyright (c) 2010, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3s
*/
/************************************************************************/

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define startACNmem() (void)

static inline void *
mallocx(size_t size)
{
    void *m;
    if ((m = malloc(size)) == NULL) {
		acnlogerror(lgERR);
		exit(EXIT_FAILURE);
    }
    return m;
}

static inline void *
mallocxz(size_t size)
{
    void *m;
    if ((m = calloc(1, size)) == NULL) {
		acnlogerror(lgERR);
		exit(EXIT_FAILURE);
    }
    return m;
}

#define acnNew(type) ((type *)mallocxz(sizeof(type)))
