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

#ifndef __dmpmap_h__
#define __dmpmap_h__ 1

enum netflags_e {
//	pflg_valid      = 1,
	pflg_read       = 2,
	pflg_write      = 4,
	pflg_event      = 8,
	pflg_vsize      = 16,
	pflg_abs        = 32,
	pflg_persistent = 64,
	pflg_constant   = 128,
	pflg_volatile   = 256,
};

#if CONFIG_DDLACCESS_DMP
struct dmpdim_s {
   int32_t i;  /* increment */
   uint32_t r; /* range (= count - 1) */
   int lvl; 	/* lvl shows original the tree order - 0 at the root */
};

struct dmpprop_s {
	enum netflags_e flags;
#if CONFIG_DDL_BEHAVIORTYPES
	enum proptype_e etype;
#endif
	uint32_t addr;
	int32_t inc;
	unsigned int size;
	int ndims;
	struct dmpdim_s dim[];
};
#define _DMPPROPSIZE sizeof(struct dmpprop_s)
#else
define _DMPPROPSIZE 0
#endif

#if CONFIG_DDLACCESS_EPI26
struct dmxprop_s {
	struct dmxbase_s *baseaddr;
	unsigned int size;
	dmxaddr_fn *setfn;
};
#define _DMXPROPSIZE sizeof(struct dmxprop_s)
#else
#define _DMXPROPSIZE 0
#endif


/*
addrfind_s comes in a sorted arrays which are used for rapid
finding of a property from it's address
*/

union proportest_u {
	struct prop_s *prop;	/* pointer to the property data */
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

#endif /*  __dmpmap_h__       */
