/**********************************************************************/
/*
Copyright (c) 2011, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3
*/
/**********************************************************************/
/*
	file: component.h
	
	Component management

	Utilities for management of local and remote components
*/

#ifndef __component_h__
#define __component_h__ 1

/*
	about: Local and remote components
	
	Structures for components

	Because component structures get passed up and down the layers a 
	lot, rather than elaborate cross linking between structures for 
	DMP, SDT RLP etc, we keep them all in sub-structures within a 
	single struct.
	
	This is of course open to abuse -- don't.
*/

/*
	struct: struct Lcomponent_s
	
	Local component
*/
struct Lcomponent_s {
	uint8_t uuid[UUID_SIZE];
	char uuidstr[UUID_STR_SIZE];
	unsigned usecount;
	const char *fctn;
	char *uacn;
	unsigned flags;
#if ACNCFG_EPI10
	struct epi10_Lcomp_s epi10;
#endif
#if ACNCFG_SDT
	struct sdt_Lcomp_s sdt;
#endif
#if ACNCFG_DMP
	struct dmp_Lcomp_s dmp;
#endif
};

enum Lcomp_flag_e {
	Lc_advert = 1,
};

#if ACNCFG_EPI19
/*
enum: slp_dmp_e
*/
enum slp_dmp_e {
	slp_found = 1,
	slp_ctl = 2,
	slp_dev = 4,
	slp_err = 8,
};

struct slp_dmp_s {
	uint16_t flags;
	char *fctn;
	char *uacn;
	uint8_t *dcid;
	acnTimer_t slpValidT;
};

#endif
/*
	struct: struct Rcomponent_s
	
	Remote component
	
	When there are multiple components using the same ACN instance, if
	one connects to another via ACN then both will create Rcomponent_s 
	as well as their Lcomponent_s.
*/
struct Rcomponent_s {
	uint8_t uuid[UUID_SIZE];
	unsigned usecount;
#if ACNCFG_EPI19
	struct slp_dmp_s slp;
#endif
#if ACNCFG_SDT
	struct sdt_Rcomp_s sdt;
#endif
#if ACNCFG_DMP
	struct dmp_Rcomp_s dmp;
#endif
};


/*
	vars: Local component(s)
	
	Local component or component set
	
	localComponent - single instance if ACNCFG_SINGLE_COMPONENT true
	Lcomponents - a struct uuidset_s if ACNCFG_SINGLE_COMPONENT is false
	
	When ACNCFG_SINGLE_COMPONENT is true we have a single global 
	struct Lcomponent_s, Macros should be used to hide the specifics so 
	that code works whether ACNCFG_SINGLE_COMPONENT is true or false.
*/
#if !ACNCFG_MULTI_COMPONENT
extern struct Lcomponent_s localComponent;
#else
extern struct uuidset_s Lcomponents;
#endif

/*
	var: Rcomponents
	Remote component set

	The set of remote components which we are communicating with. These
	are managed by the geenric UUID tracking code of uuid.h.
*/
extern struct uuidset_s Rcomponents;

/**********************************************************************/
static inline struct Lcomponent_s *
findLcomp(const uint8_t *uuid)
{
#if !ACNCFG_MULTI_COMPONENT
	if (uuidsEq(uuid, localComponent.uuid) && localComponent.usecount > 0)
		return &localComponent;
	return NULL;
#else
	return container_of(finduuid(&Lcomponents, uuid), struct Lcomponent_s, uuid[0]);
#endif
}

/**********************************************************************/
#if !ACNCFG_MULTI_COMPONENT
#define releaseLcomponent(Lcomp) (Lcomp->usecount ? --Lcomp->usecount : 0)
#else
static inline void
releaseLcomponent(struct Lcomponent_s *Lcomp)
{
	unlinkuuid(&Lcomponents, Lcomp->uuid);
	if (--Lcomp->usecount == 0) free(Lcomp);
}
#endif    /* !ACNCFG_SINGLE_COMPONENT */

/**********************************************************************/
#if ACNCFG_MULTI_COMPONENT
static inline int
addLcomponent(struct Lcomponent_s *Lcomp)
{
	return adduuid(&Lcomponents, Lcomp->uuid);
}
#endif

/**********************************************************************/
static inline struct Rcomponent_s *
findRcomp(const uint8_t *uuid)
{
	return container_of(finduuid(&Rcomponents, uuid), struct Rcomponent_s, uuid[0]);
}

/**********************************************************************/
static inline void
releaseRcomponent(struct Rcomponent_s *Rcomp)
{
	unlinkuuid(&Rcomponents, Rcomp->uuid);
	if (--(Rcomp->usecount) == 0) free(Rcomp);
}
/**********************************************************************/
static inline int
addRcomponent(struct Rcomponent_s *Rcomp)
{
	return adduuid(&Rcomponents, Rcomp->uuid);
}

/**********************************************************************/
/*
prototypes:
*/
extern int components_init(void);
extern int initstr_Lcomponent(ifMC(struct Lcomponent_s *Lcomp,)
						const char* uuidstr);
extern int initbin_Lcomponent(ifMC(struct Lcomponent_s *Lcomp,)
						const uint8_t* uuid);

#endif  /* __component_h__ */
