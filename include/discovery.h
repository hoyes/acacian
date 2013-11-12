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
