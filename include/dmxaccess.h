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

header: dmxaccess.h

DMX512 and E1.31 access in DDL descriptions
*/

#ifndef __dmxaccess_h__
#define __dmxaccess_h__ 1


#if CONFIG_DDLACCESS_EPI26
struct dmxprop_s {
	struct dmxbase_s *baseaddr;
	unsigned int size;
	dmxaddr_fn *setfn;
};
#endif

#endif /* __dmxaccess_h__ */
