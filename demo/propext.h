/**********************************************************************/
/*
Copyright (c) 2013, Philip Nye
All rights reserved.

	$Id$

#tabs=3t
*/
/**********************************************************************/

#ifndef __propext_h__
#define __propext_h__ 1

/**********************************************************************/
/*
extensions
*/
struct dmpprop_s;

/*
typedef int get_fn(struct dmpprop_s *, uint32_t, uint32_t, uint32_t, uint8_t *, unsigned int);
typedef int set_fn(struct dmpprop_s *, uint32_t, uint32_t, uint32_t, uint8_t *, unsigned int);
typedef int sub_fn(struct dmpprop_s *, uint32_t, uint32_t, uint32_t);

extern int getpersist(struct dmpprop_s *, uint32_t, uint32_t, uint32_t, uint8_t *, unsigned int);
extern int getconst(struct dmpprop_s *, uint32_t, uint32_t, uint32_t, uint8_t *, unsigned int);
extern int setpersist(struct dmpprop_s *, uint32_t, uint32_t, uint32_t, uint8_t *, unsigned int);
*/
extern char UACN[ACN_UACN_SIZE];
extern const char hardversion[];
extern const char softversion[];
extern char serialno[];
extern uint16_t barvals[];

#endif  /* __propext_h__ */
