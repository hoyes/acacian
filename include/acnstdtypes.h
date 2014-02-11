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
#ifndef __acnstdtypes_h__
#define __acnstdtypes_h__ 1

/*
header: acnstdtypes.h

Standardized type names used throughout Acacian:

#include acnstdtypes.h

	Type definitions for fixed size types used in acacian including 
	booleans.
	
	These are all standard types in ISO C99 defined in standard headers.
	If your compiler library appear ISO C99 compliant then the standard
	headers are included and it should just work.

	If you have a non-standard setup, there are various options in
	order of preference:
	
	o Your system may have the necessary headers anyway despite appearing
	non-compliant. Try defining HAVE_INT_TYPES_H which will attempt to 
	include those headers.
	
	o If your compiler/library includes `limits.h` then this header 
	will include `typefromlimits.h` which deduces and defines the 
	necessary types from there.

	o As a last resort define CF_USER_INTTYPES and create your own 
	header `user_types.h` to define the necessary types – look at 
	`typefromlimits.h` for guidance.
	
*/

#ifdef CF_USER_INTTYPES
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
