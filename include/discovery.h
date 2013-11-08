/**********************************************************************/
/*
Copyright (c) 2013, Philip Nye
All rights reserved.

	$Id$

#tabs=3t
*/
/**********************************************************************/

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
