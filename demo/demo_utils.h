/**********************************************************************/
/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (c) 2013, Acuity Brands, Inc.

Author: Philip Nye <philip.nye@engarts.com>

#tabs=3
*/
/**********************************************************************/
/*
about: Acacian

Acacian is a full featured implementation of ANSI E1.17 2012
Architecture for Control Networks (ACN) from Acuity Brands

header: demo_utils.h

Header for <demo_utils.c>
*/

#ifndef __demo_utils_h__
#define __demo_utils_h__ 1

/**********************************************************************/
/*
prototypes
*/
extern char uacn[ACN_UACN_SIZE + 1];
void uacn_init(const char *cidstr);
void uacn_change(const uint8_t *dp, int size);
void uacn_close(void);

#endif  /* __demo_utils_h__ */
