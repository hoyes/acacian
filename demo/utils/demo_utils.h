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
header: demo_utils.h

Header for <demo_utils.c>
*/

#ifndef __demo_utils_h__
#define __demo_utils_h__ 1

/**********************************************************************/
struct ayindex_s {
	int iterdim;
	uint32_t inc;
	uint32_t count;
	uint32_t ixs[];
};
/**********************************************************************/
/*
prototypes
*/
extern char uacn[ACN_UACN_SIZE + 1];
int addr2ofs(const struct dmpprop_s *dprop, struct adspec_s *dmpads, struct adspec_s *ofsads);
void ofs2addr(const struct dmpprop_s *dprop, struct adspec_s *ofsads, struct adspec_s *dmpads);
struct ayindex_s *addr2index(const struct dmpprop_s *dprop, struct adspec_s *dmpads);
void uacn_init(const char *cidstr);
void uacn_change(const uint8_t *dp, int size);
void uacn_close(void);

#endif  /* __demo_utils_h__ */
