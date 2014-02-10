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
header: tohex.h

Convert binary t o hexadecimal strings
*/

#ifndef __tohex_h__
#define __tohex_h__ 1

static inline char tohex(unsigned int nibble)
{
	char rslt;
   
	rslt = nibble + '0';
	if (rslt > '9') rslt += ('a' - 10 - '0');
	return rslt;
}

#endif	/* __tohex_h__ */
