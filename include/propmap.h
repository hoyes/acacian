/**********************************************************************/
/*

	Copyright (c) 2011, Philip Nye, Engineering Arts (UK) philip@engarts.com
	All rights reserved.

	Author: Philip Nye

	$Id$

*/
/*
#tabs=3
*/
/**********************************************************************/

#ifndef __propmap_h__
#define __propmap_h__ 1

/*
addrfind_s comes in a sorted arrays which are used for rapid
finding of a property from it's address
*/

union proportest_u {
	prop_t *prop;	/* pointer to the property data */
	struct addrtest_s *test;
};

struct addrtest_s {
	struct prop_s *prop;
	union proportest_u nxt;
};

struct addrfind_s {
	uint32_t adlo;	/* lowest address of the region */
	uint32_t adhi;	/* highest address */
	int ntests; /* true if address range is packed (no holes) */
	union proportest_u p;
};

struct addrmapheader_s {
	unsigned int mapsize;
	unsigned int count;
};

struct addrmap_s {
   struct addrmapheader_s h;
	struct addrfind_s map[];
};

#endif /*  __propmap_h__       */

