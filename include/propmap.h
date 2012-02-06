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

#define MAX_PROP_SIZE 256

struct proptab_s *makemap(rootprop_t *root);


#if 0
struct member_s;

struct array_def {
	struct array_def *parent;
	uint32_t count;
#if CONFIG_DDLACCESS_DMP
	int32_t inc;
#endif
};

struct proptablea_s {
	struct proptablea_s *nxt;
	int tabsize;
	uint32_t inc;
};

/* First table is a special case */
struct propref1_s {
	uint32_t lo;
	uint32_t hi;
	void *ref;
};

struct propmap_s {
	struct proptablea_s *nxt;
	int tabsize;
	struct propref1_s refs[];
};
#endif

#endif /*  __propmap_h__       */

