
/************************************************************************/
/*
Copyright (c) 2010, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3
*/
/************************************************************************/

#ifndef __tohex_h__
#define __tohex_h__ 1

static inline char tohex(unsigned int nibble)
{
	char rslt;
   
	rslt = nibble + '0';
	if (rslt > '9') rslt += ('a' - 10 - '0');
	return rslt;
}

#endif	/* __tohex_h__ */
