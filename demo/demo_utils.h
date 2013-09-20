/**********************************************************************/
/*
Copyright (c) 2013, Philip Nye
All rights reserved.

	$Id$

#tabs=3t
*/
/**********************************************************************/

#ifndef __demo_utils_h__
#define __demo_utils_h__ 1

/**********************************************************************/
/*
prototypes
*/
struct Lcomponent_s;

int slp_start_sa(
	ifMC(struct Lcomponent_s *Lcomp,)
	uint16_t port,
	ifDMP_D(const char *dcidstr,)
	const char *interfaces[]
);

#endif  /* __demo_utils_h__ */
