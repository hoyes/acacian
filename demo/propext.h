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
struct dmprcxt_s;
struct dmpprop_s;
struct adspec_s;

typedef int dmprx_fn(struct dmprcxt_s *rcxt,
						const struct dmpprop_s *dprop,
						struct adspec_s *ads);

typedef int dmprxd_fn(struct dmprcxt_s *rcxt,
						const struct dmpprop_s *dprop,
						struct adspec_s *ads,
						const uint8_t *data,
						bool dmany);

extern int setbar(struct dmprcxt_s *rcxt,
						const struct dmpprop_s *dprop,
						struct adspec_s *ads,
						const uint8_t *data,
						bool dmany);
extern int getbar(struct dmprcxt_s *rcxt,
						const struct dmpprop_s *dprop,
						struct adspec_s *ads);
extern int subscribebar(struct dmprcxt_s *rcxt,
						const struct dmpprop_s *dprop,
						struct adspec_s *ads);
extern int unsubscribebar(struct dmprcxt_s *rcxt,
						const struct dmpprop_s *dprop,
						struct adspec_s *ads);

extern int getstrprop(struct dmprcxt_s *rcxt,
						const struct dmpprop_s *dprop,
						struct adspec_s *ads);
extern int setuacn(struct dmprcxt_s *rcxt,
						const struct dmpprop_s *dprop,
						struct adspec_s *ads,
						const uint8_t *data,
						bool dmany);

extern char uacn[ACN_UACN_SIZE + 1];  /* allow for trailing newline */
extern const char hardversion[];
extern const char softversion[];
extern char serialno[];
extern uint16_t barvals[];

#endif  /* __propext_h__ */
