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
#if !defined(ACNCFG_MULTI_COMPONENT)
	struct {
		uuid_t uuid;  /**< Header just contains UUID */
	} hd;
#else
	struct uuidhd_s hd;  /**< Header tracks by UUID */
#endif
	unsigned usecount;
#if defined(ACNCFG_EPI10)
	struct epi10_Lcomp_s epi10;
#endif
#if defined(ACNCFG_SDT)
	struct sdt_Lcomp_s sdt;
#endif
#if defined(ACNCFG_DMP)
	struct dmp_Lcomp_s dmp;
#endif
#if defined(struct app_Lcomp_s)
	struct app_Lcomp_s app;
#endif
};

/*
	struct: struct Rcomponent_s
	
	Remote component
	
	When there are multiple components using the same ACN instance, if
	one connects to another via ACN then both will create Rcomponent_s 
	as well as their Lcomponent_s.
*/
struct Rcomponent_s {
	struct uuidhd_s hd;
	unsigned usecount;
#if defined(ACNCFG_SDT)
	sdt_Rcomp_t sdt;
#endif
#if defined(ACNCFG_DMP)
	dmp_Rcomp_t dmp;
#endif
#if defined(app_Rcomp_t)
	app_Rcomp_t app;
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
#if !defined(ACNCFG_MULTI_COMPONENT)
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
findLcomp(const uuid_t cid)
{
#if !defined(ACNCFG_MULTI_COMPONENT)
	if (uuidsEq(cid, localComponent.hd.uuid) && localComponent.usecount > 0)
		return &localComponent;
	return NULL;
#else
	return container_of(finduuid(&Lcomponents, cid), struct Lcomponent_s, hd);
#endif
}

/**********************************************************************/
static inline int
findornewLcomp(const uuid_t cid, struct Lcomponent_s **Lcomp)
{
#if !defined(ACNCFG_MULTI_COMPONENT)
	*Lcomp = &localComponent;
	return (localComponent.usecount <= 0);
#else
	return findornewuuid(&Rcomponents, cid, (struct uuidhd_s **)Lcomp, sizeof(struct Lcomponent_s));
#endif
}

/**********************************************************************/
#if !defined(ACNCFG_MULTI_COMPONENT)
#define releaseLcomponent(Lcomp, useby) \
						(--Lcomp->usecount)
#else
static inline void
releaseLcomponent(struct Lcomponent_s *Lcomp)
{
	if (--(Lcomp->usecount) > 0) return;
	unlinkuuid(&Lcomponents, &Lcomp->hd);
	free(Lcomp);
}
#endif    /* !ACNCFG_SINGLE_COMPONENT */

/**********************************************************************/
typedef void Lcompiterfn(struct Lcomponent_s *);

#if !defined(ACNCFG_MULTI_COMPONENT)
static inline void
foreachLcomp(Lcompiterfn *fn)
{
	(*fn)(&localComponent);
}
#else
static inline void
foreachLcomp(Lcompiterfn *fn)
{
	_foreachuuid(&Lcomponents.first, (uuiditerfn *)fn);
}
#endif    /* !ACNCFG_SINGLE_COMPONENT */

/**********************************************************************/
static inline struct Rcomponent_s *
findRcomp(const uint8_t *cid)
{
	return container_of(finduuid(&Rcomponents, cid), struct Rcomponent_s, hd);
}

/**********************************************************************/
static inline int
findornewRcomp(const uuid_t cid, struct Rcomponent_s **Rcomp)
{
	return findornewuuid(&Rcomponents, cid, (struct uuidhd_s **)Rcomp, sizeof(struct Rcomponent_s));
}

/**********************************************************************/
static inline void
releaseRcomponent(struct Rcomponent_s *Rcomp)
{
	if (--(Rcomp->usecount) > 0) return;
	unlinkuuid(&Rcomponents, &Rcomp->hd);
	free(Rcomp);
}

/**********************************************************************/
typedef void Rcompiterfn(struct Rcomponent_s *);

static inline void
foreachRcomp(Rcompiterfn *fn)
{
	_foreachuuid(&Rcomponents.first, (uuiditerfn *)fn);
}

/**********************************************************************/
/*
	func: init_Lcomponent
	
	Initialize a struct Lcomponent_s and assign a CID
*/

int init_Lcomponent(struct Lcomponent_s *Lcomp, const char *cidstr);

/*
	func: components_init
	
	Initialize component handling
	
	return:
	0 on success, -1 on fail
*/
extern int components_init(void);

#endif  /* __component_h__ */
