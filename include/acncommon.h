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

#tabs=3s
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
   ((x) < 2) ? 1 :\
   ((x) < 4) ? 2 :\
   ((x) < 8) ? 3 :\
   ((x) < 16) ? 4 :\
   ((x) < 32) ? 5 :\
   ((x) < 64) ? 6 :\
   ((x) < 128) ? 7 :\
   ((x) < 256) ? 8 :\
   ((x) < 512) ? 9 :\
   ((x) < 1024) ? 10 :\
   ((x) < 2048) ? 11 :\
   ((x) < 4096) ? 12 :\
   ((x) < 8192) ? 13 :\
   ((x) < 16384) ? 14 :\
   ((x) < 32768) ? 15 :\
   16)
/*
macro: clog2

log2 of integer x rounded up to nearest integer

Exaample:

  clog2(1023) compiles to 10
  clog2(1024) compiles to 10
  clog2(1025) compiles to 11
*/
#define clog2(x) nbits((x)-1)

/*
macro: cpwr2

Round up to nearest power of 2 larger than or equal to x (up to 16 bits only)

Exaample:

  cpwr2(1023) compiles to 1024
  cpwr2(1024) compiles to 1024
  cpwr2(1025) compiles to 2048
*/
#define cpwr2(x) (\
   ((x) <= 1) ? 1 :\
   ((x) <= 2) ? 2 :\
   ((x) <= 4) ? 4 :\
   ((x) <= 8) ? 8 :\
   ((x) <= 16) ? 16 :\
   ((x) <= 32) ? 32 :\
   ((x) <= 64) ? 64 :\
   ((x) <= 128) ? 128 :\
   ((x) <= 256) ? 256 :\
   ((x) <= 512) ? 512 :\
   ((x) <= 1024) ? 1024 :\
   ((x) <= 2048) ? 2048 :\
   ((x) <= 4096) ? 4096 :\
   ((x) <= 8192) ? 8192 :\
   ((x) <= 16384) ? 16384 :\
   ((x) <= 32768) ? 32768 :\
   65536)

#if defined(__GNUC__)
#define UNUSED __attribute__ ((unused))
#define INITIALIZED(var) var = var
#else
#define UNUSED
#define INITIALIZED(var)
#endif

/* ACN flags/length word */
#define getpdulen(pdup) (unmarshalU16(pdup) & LENGTH_MASK)
/* OFS_VECTOR applies at any PDU layer and is the offset to the vector
field from start of PDU*/
#define OFS_VECTOR     2

#endif /* __acncommon_h__ */
