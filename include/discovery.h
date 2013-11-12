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

header: discovery.h

Header for <discovery.c>
*/

#ifndef __discovery_h__
#define __discovery_h__ 1

/**********************************************************************/
/*
prototypes
*/
struct Lcomponent_s;

extern const char * const slperrs[];

int slp_register(ifMC(struct Lcomponent_s *Lcomp));
void slp_deregister(ifMC(struct Lcomponent_s *Lcomp));
void discover(void);

#endif  /* __discovery_h__ */
