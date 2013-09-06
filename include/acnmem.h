
/************************************************************************/
/*
Copyright (c) 2010, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3s
*/
/************************************************************************/

#ifndef __acnmem_h__
#define __acnmem_h__ 1

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define startACNmem() (0)

static inline void *
mallocx(size_t size)
{
    void *m;
    if ((m = malloc(size)) == NULL) {
		acnlogerror(LOG_ON | LOG_CRIT);
		exit(EXIT_FAILURE);
    }
    return m;
}

static inline void *
mallocxz(size_t size)
{
    void *m;
    if ((m = calloc(1, size)) == NULL) {
		acnlogerror(LOG_ON | LOG_CRIT);
		exit(EXIT_FAILURE);
    }
    return m;
}

static inline void *
reallocx(void * ptr, size_t size)
{
    void *m;
    if ((m = realloc(ptr, size)) == NULL) {
		acnlogerror(LOG_ON | LOG_CRIT);
		exit(EXIT_FAILURE);
    }
    return m;
}

#define acnNew(type) ((__typeof__(type) *)mallocxz(sizeof(type)))

#endif /* __acnmem_h__ */
