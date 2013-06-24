/************************************************************************/
/*
Copyright (c) 2007, Engineering Arts (UK)

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

 * Redistributions of source code must retain the above copyright
	notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.
 * Neither the name of Engineering Arts nor the names of its
	contributors may be used to endorse or promote products derived from
	this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  $Id$

#tabs=3t
*/
/************************************************************************/
/*
file: acncommon.h

Common macros and definitions

This header defines some utility macros which are common
across eaACN code.

*/
#ifndef __acncommon_h__
#define __acncommon_h__ 1

#ifndef container_of

/*
macro: container_of

Find the containing structure of a member.

Given a pointer to a member of a structure, this macro will return a pointer
to the parent structure.

Parameters:
	ptr - pointer to to the member
	ptype - the type of the parent structure
	member - the name of the member
*/

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
macro: ARRAYSIZE

The number of elements in an array
*/
#ifndef ARRAYSIZE
/* the number of elements in an array */
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

/*
macro: STRINGIFY

Turn the expansion of a macro into a string
*/
#define _STRINGIFY_(x) # x
#define STRINGIFY(x) _STRINGIFY_(x)

/*
macro: ZEROTOEND

zero out a structure starting at member through to the end
*/
#define ZEROTOEND(structp, member) \
	memset(&(structp)->member, 0, (void *)((structp) + 1) - (void *)(&(structp)->member))

/*
macro: nbits(x)

Number of bits required to contain an integer

Works for 0 <= x < 65536

For x = 0 returns 1

For arguments larger than 65535 returns 16

Exaample:

  nbits(1023) compiles to 10
  nbits(1024) compiles to 11
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
macro: clog2

log2 of integer x rounded up to nearest integer

Exaample:

  clog2(1023) compiles to 10
  clog2(1024) compiles to 10
  clog2(1025) compiles to 11

*/
#define clog2(x) (((x) <= 0) ? (unsigned) -1 : nbits((x)-1))

/*
macro: cpwr2

Round up to nearest power of 2 larger than or equal to x
This is the same as (1 << clog2(x))

Exaample:

  cpwr2(1023) compiles to 1024
  cpwr2(1024) compiles to 1024
  cpwr2(1025) compiles to 2048
*/
#define cpwr2(x) (1 << clog2(x))

/*
macros:

UNUSED - explicitly mark a variable or argument as unused to avoid "unused variable" compiler warnings.
INITIALIZED - explicitly mark a variable to avoid "may be used un-initialized" compiler warnings.
*/
#if defined(__GNUC__)
#define UNUSED __attribute__ ((unused))
#define INITIALIZED(var) var = var
#else
#define UNUSED
#define INITIALIZED(var)
#endif

#endif /* __acncommon_h__ */
