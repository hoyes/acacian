/**********************************************************************/
/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (c) 2013, Acuity Brands, Inc.

Author: Philip Nye <philip.nye@engarts.com>

#tabs=3
*/
/**********************************************************************/
/*
about: Acacian

Acacian is a full featured implementation of ANSI E1.17 2012
Architecture for Control Networks (ACN) from Acuity Brands

header: acnstdtypes.h

Standardized type names used throughout Acacian
*/

#ifndef __acnstdtypes_h__
#define __acnstdtypes_h__ 1

/*
  Type definitions for fixed size types in 8, 16, 32 bits and booleans.

  These are a subset of the ISO C99 types and should be kept to the 
  standard. For ISO C99 compilers (or if HAVE_INT_TYPES_H is 
  defined) we just pull in C99 standard headers inttypes.h and 
  stdbool.h. 

  HAVE_INT_TYPES_H
  If USER_DEFINE_INTTYPES is set the user wants to define these 
  themselves.

  Otherwise we assume a C89 compiler and try and deduce them mostly 
  from limits.h which is a C89 header  and present on most systems.

*/

#ifdef USER_DEFINE_INTTYPES
#include "user_types.h"

#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L || defined(HAVE_INT_TYPES_H)
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

/*
_MSC_VER is still defined when VisualC is not in ISO mode
(and limits.h still works).
*/
#else  /* assume C89 */
#include "typefromlimits.h"

/*
  Booleans
  These definitions are the same as stdbool.h (C99 standard header)
*/
#ifndef bool
#define bool _Bool
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif
#define __bool_true_false_are_defined	1

#endif  /* defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L || defined(HAVE_INT_TYPES_H) */

#define PACKED __attribute__((__packed__))

#define _Bool int8_t

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define ZEROARRAY
#else
#define ZEROARRAY 1
#endif

#endif /* __acnstdtypes_h__ */
