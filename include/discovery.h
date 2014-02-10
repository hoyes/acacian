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

Utilities for SLP (Service Location Protocol) as specified in epi19:
*/

#ifndef __discovery_h__
#define __discovery_h__ 1

/**********************************************************************/
/*
prototypes
*/
struct Lcomponent_s;

/*
var: slperrs

Table of SLP error descriptions used for diagnostic messages
*/
extern const char * const slperrs[];

/*
func: slp_register

Register (or re-register) a local component for advertisement by SLP
service agent.
All the necessary information is part of the Lcomponent_s (which is
passed as the first arg if <CF_MULTICOMP> is set).
*/
int slp_register(ifMC(struct Lcomponent_s *Lcomp));

/*
func: slp_deregister

De-register a local component with SLP service agent.
*/
void slp_deregister(ifMC(struct Lcomponent_s *Lcomp));

/*
func: discover

Call openSLP to discover available acn.esta services.

This first builds a list of services, then queries each to find
their attributes. The callback for returned attributes <discAtt_cb> 
parses the attributes and adds or updates
suitable discovered components in the Remote component set.

In applications where large numbers of remote components are
discovered that are not connected to, it could be more efficient
to maintain seperate sets for discovered components and actively
connected components.
*/
void discover(void);

#endif  /* __discovery_h__ */
