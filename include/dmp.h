
/************************************************************************/
/*

   Copyright (c) 2011, Philip Nye, Engineering Arts (UK) philip@engarts.com
   All rights reserved.

   Author: Philip Nye

   $Id$

*/
/************************************************************************/
/*
#tabs=3
*/

#ifndef __dmp_h__
#define __dmp_h__ 1

#include "acncommon.h"
#include "component.h"
#include "acnstd/dmp.h"
/* #include "sdt.h" */

#define IS_RANGE(type) (((type) & DMPAD_TYPEMASK) != DMPAD_SINGLE)
#define IS_RELADDR(type) (((type) & DMPAD_R) != 0)
#define IS_MULTIDATA(type) (((type) & DMPAD_RANGE_ARRAY) != 0)
#define IS_SINGLEDATA(type) (((type) & DMPAD_RANGE_ARRAY) != 0)
/************************************************************************/
/*
Packet structure lengths
*/
#define DMP_BLOCK_MIN (OFS_VECTOR + DMP_VECTOR_LEN + DMP_HEADER_LEN)

typedef struct dmp_Lcomp_s dmp_Lcomp_t;
typedef struct dmp_Rcomp_s dmp_Rcomp_t;

struct dmp_Lcomp_s {
	int dummy;
};

struct dmp_Rcomp_s {
	int dummy;
};
#endif /* __dmp_h__ */
