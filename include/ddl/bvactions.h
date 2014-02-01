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
header: bvactions.h

Handlers for specific DDL behaviors.

For each behavior action function defined in <bvactions.c>, define a
BVA_behaviorset_behaviorname macro
for every behavior which calls that action - these macros then 
create the appropriate entries in the known_bvs table.

*/

#ifndef __bvactions_h__
#define __bvactions_h__ 1

extern struct bv_s known_bvs[];

#endif /* __bvactions_h__ */
