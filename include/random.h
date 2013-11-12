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

header: random.h

Declarations and macros for pseudo random numbers. See <random.c>
*/

/* on linux systems you may need to #define _XOPEN_SOURCE = 600 here */
#include <stdlib.h>
#define getrandU16() (random() & 0xffff)

void randomize(bool force);
