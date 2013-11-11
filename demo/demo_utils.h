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
extern char uacn[ACN_UACN_SIZE + 1];
void uacn_init(const char *cidstr);
void uacn_change(const uint8_t *dp, int size);
void uacn_close(void);

#endif  /* __demo_utils_h__ */
