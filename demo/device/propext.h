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
header: propext.h

XML Property extensions.
*/


#ifndef __propext_h__
#define __propext_h__ 1

/**********************************************************************/
/*
extensions
*/
struct dmprcxt_s;
struct dmpprop_s;
struct adspec_s;

typedef int dmprx_fn(struct dmprcxt_s *rcxt, const uint8_t *bp);

extern int setbar(struct dmprcxt_s *rcxt, const uint8_t *bp);
extern int getbar(struct dmprcxt_s *rcxt, const uint8_t *bp);
extern int subscribebar(struct dmprcxt_s *rcxt, const uint8_t *bp);
extern int unsubscribebar(struct dmprcxt_s *rcxt, const uint8_t *bp);
extern int getstrprop(struct dmprcxt_s *rcxt, const uint8_t *bp);
extern int setuacn(struct dmprcxt_s *rcxt, const uint8_t *bp);

extern char uacn[ACN_UACN_SIZE + 1];  /* allow for trailing newline */
extern const char hardversion[];
extern const char softversion[];
extern char serialno[];
extern uint16_t barvals[];

#endif  /* __propext_h__ */
