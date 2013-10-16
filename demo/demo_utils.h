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

#define MAXINTERFACES 8
extern const char *interfaces[MAXINTERFACES];
extern int ifc;

int slp_startSA(ifMC(struct Lcomponent_s *Lcomp,) uint16_t port);
void slp_stopSA(void);

void uacn_init(const char *cidstr);
void uacn_change(const uint8_t *dp, int size);
void uacn_close(void);

#endif  /* __demo_utils_h__ */
