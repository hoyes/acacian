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
#ifndef __acnmem_h__
#define __acnmem_h__ 1
/**********************************************************************/
/*
header: acnmem.h

Memory management macros and inline functions:

All code uses functions or macros defined here to allocate memory. If
you need to use special allocation functions override these.
*/


#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define startACNmem() (0)

/*
func: mallocx

Allocate memory or exit on failure.

Deprecated: Code should check for failure and recover more gracefully.
*/
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

/*
func: mallocxz

Allocate and zero memory or exit on failure.

Deprecated: Code should check for failure and recover more gracefully.
*/
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

/*
func: reallocx

Re-allocate memory or exit on failure.

Deprecated: Code should check for failure and recover more gracefully.
*/
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

/*
macros: memory allocation wrappers

acnNew(type) - create a new variable of the given type, zero it and
return a pointer to it.

acnalloc(size) - a wrapper for malloc which allows overriding with
custom functions.

acnfree(ptr) - a wrapper for free which allows overriding with
custom functions.
*/

#define acnNew(type) ((__typeof__(type) *)mallocxz(sizeof(type)))
#define acnalloc(x) malloc(x)
#define acnfree(x) free(x)

#endif /* __acnmem_h__ */
