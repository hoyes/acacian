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
header: acncommon.h

Utility macros which are common across Acacian code:

Macros like <ARRAYSIZE()> and <container_of()> adhere to syntax and
definitions encountered in many systems and are only defined here
if there is not a definition already in force.
*/
#ifndef __acncommon_h__
#define __acncommon_h__ 1

/*
macro: container_of(ptr, type, member)

Find the containing structure of a member.

This may already be defined in your programming environment. If not 
it is defined here.

Given a pointer to a member of a structure, this macro will return a pointer
to the parent structure.

Args:
	ptr - pointer to to the member
	ptype - the type of the parent structure
	member - the name of the member
*/

#ifndef container_of
#if defined __GNUC__
#define container_of(ptr, type, member) ({			\
	const __typeof__(((type *)0)->member) *__mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member));})
#else
#define container_of(ptr, ptype, field) \
	((ptype *)((char *)(ptr) - offsetof(ptype, member)))
#endif
#endif

/*
macro: ARRAYSIZE(array)

The number of elements in array.
*/
#ifndef ARRAYSIZE
/* the number of elements in an array */
#define ARRAYSIZE(array) (sizeof(array)/sizeof(array[0]))
#endif

/*
macro: STRINGIFY(s)

Turn the expansion of a macro into a string
*/
#define _STRINGIFY_(s) # s
#define STRINGIFY(s) _STRINGIFY_(s)

/*
macro: ZEROTOEND(pstruct, member)

zero out a structure starting at member through to the end
*/
#define ZEROTOEND(pstruct, member) \
	memset(&(pstruct)->member, 0, \
	(void *)((pstruct) + 1) - (void *)(&(pstruct)->member))

/*
macro: nbits(x)

Number of bits required to contain an integer.

This is not efficient for variables but works fine for literal constants.

For x = 0 returns 1

Example:

  nbits(1023) compiles to 10,
  nbits(1024) compiles to 11,
  nbits(1025) compiles to 11
*/
#define nbits(x) (\
	((unsigned int)(x) < 0x00000001) ?  0 :\
	((unsigned int)(x) < 0x00000002) ?  1 :\
	((unsigned int)(x) < 0x00000004) ?  2 :\
	((unsigned int)(x) < 0x00000008) ?  3 :\
	((unsigned int)(x) < 0x00000010) ?  4 :\
	((unsigned int)(x) < 0x00000020) ?  5 :\
	((unsigned int)(x) < 0x00000040) ?  6 :\
	((unsigned int)(x) < 0x00000080) ?  7 :\
	((unsigned int)(x) < 0x00000100) ?  8 :\
	((unsigned int)(x) < 0x00000200) ?  9 :\
	((unsigned int)(x) < 0x00000400) ? 10 :\
	((unsigned int)(x) < 0x00000800) ? 11 :\
	((unsigned int)(x) < 0x00001000) ? 12 :\
	((unsigned int)(x) < 0x00002000) ? 13 :\
	((unsigned int)(x) < 0x00004000) ? 14 :\
	((unsigned int)(x) < 0x00008000) ? 15 :\
	((unsigned int)(x) < 0x00010000) ? 16 :\
	((unsigned int)(x) < 0x00020000) ? 17 :\
	((unsigned int)(x) < 0x00040000) ? 18 :\
	((unsigned int)(x) < 0x00080000) ? 19 :\
	((unsigned int)(x) < 0x00100000) ? 20 :\
	((unsigned int)(x) < 0x00200000) ? 21 :\
	((unsigned int)(x) < 0x00400000) ? 22 :\
	((unsigned int)(x) < 0x00800000) ? 23 :\
	((unsigned int)(x) < 0x01000000) ? 24 :\
	((unsigned int)(x) < 0x02000000) ? 25 :\
	((unsigned int)(x) < 0x04000000) ? 26 :\
	((unsigned int)(x) < 0x08000000) ? 27 :\
	((unsigned int)(x) < 0x10000000) ? 28 :\
	((unsigned int)(x) < 0x20000000) ? 29 :\
	((unsigned int)(x) < 0x40000000) ? 30 :\
	((unsigned int)(x) < 0x80000000) ? 31 :\
	32)
/*
macro: clog2(x)

log2 of integer x rounded up to nearest integer. Returns -1 if x <= 0

Example:

  clog2(1023) compiles to 10,
  clog2(1024) compiles to 10,
  clog2(1025) compiles to 11

*/
#define clog2(x) (((x) <= 0) ? (unsigned) -1 : nbits((x)-1))

/*
macro: cpwr2(x)

Round up to nearest power of 2 larger than or equal to x
This is the same as (1 << clog2(x))

Example:

  cpwr2(1023) compiles to 1024,
  cpwr2(1024) compiles to 1024,
  cpwr2(1025) compiles to 2048
*/
#define cpwr2(x) (1 << clog2(x))

/*
macros: Avoiding compiler warnings.

UNUSED - explicitly mark a variable or argument as unused to avoid 
"unused variable" compiler warnings.
INITIALIZED - explicitly mark a variable to avoid "may be used 
un-initialized" compiler warnings.
*/
#if defined(__GNUC__)
#define UNUSED __attribute__ ((unused))
#define INITIALIZED(var) var = var
#else
#define UNUSED
#define INITIALIZED(var)
#endif

#endif /* __acncommon_h__ */
