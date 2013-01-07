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
Predeclare incomplete structs and types so we can make pointers to them
*/

typedef struct Lcomponent_s Lcomponent_t;
typedef struct Rcomponent_s Rcomponent_t;

/*
	about: Local and remote components
	
	Structures for components

	Because component structures get passed up and down the layers a 
	lot, rather than elaborate cross linking between structures for 
	DMP, SDT RLP etc, we keep them all in sub-structures within a 
	single struct.
	
	This is of course open to abuse -- don't.
*/

enum useflags_e {
	USEDBY_NONE = 0,
	USEDBY_SDT = 1,
	USEDBY_DMP = 2,
	USEDBY_APP = 4,
};
#define USEDBY_ANY (~(enum useflags_e)0)

/*
	struct: struct Lcomponent_s
	
	Local component
*/
struct Lcomponent_s {
#if CONFIG_SINGLE_COMPONENT
	struct {
		uuid_t uuid;  /**< Header just contains UUID */
	} hd;
#else
	uuidhd_t hd;  /**< Header tracks by UUID */
#endif
	enum useflags_e useflags;
#if CONFIG_EPI10
	epi10_Lcomp_t epi10;
#endif
#if CONFIG_SDT
	sdt_Lcomp_t sdt;
#endif
#if CONFIG_DMP
	dmp_Lcomp_t dmp;
#endif
#if defined(app_Lcomp_t)
	app_Lcomp_t app;
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
	uuidhd_t hd;
	enum useflags_e useflags;
#if CONFIG_SDT
	sdt_Rcomp_t sdt;
#endif
#if CONFIG_DMP
	dmp_Rcomp_t dmp;
#endif
#if defined(app_Rcomp_t)
	app_Rcomp_t app;
#endif
};

/*
	vars: Local component(s)
	
	Local component or component set
	
	localComponent - single instance if CONFIG_SINGLE_COMPONENT true
	Lcomponents - a uuidset_t if CONFIG_SINGLE_COMPONENT is false
	
	When CONFIG_SINGLE_COMPONENT is true we have a single global 
	Lcomponent_t, Macros should be used to hide the specifics so 
	that code works whether CONFIG_SINGLE_COMPONENT is true or false.
*/
#if CONFIG_SINGLE_COMPONENT
extern Lcomponent_t localComponent;
#else
uuidset_t Lcomponents;
#endif

/*
	var: Rcomponents
	Remote component set

	The set of remote components which we are communicating with. These
	are managed by the geenric UUID tracking code of uuid.h.
*/
uuidset_t Rcomponents;

/**********************************************************************/
static inline Lcomponent_t *
findLcomp(const uuid_t cid, enum useflags_e usedby)
{
#if CONFIG_SINGLE_COMPONENT
	if (uuidsEq(cid, localComponent.hd.uuid)
								&& (localComponent.useflags & usedby))
		return &localComponent;
	return NULL;
#else
	Lcomponent_t *Lcomp;
	Lcomp = container_of(finduuid(&Lcomponents, cid), Lcomponent_t, hd);
	if (Lcomp && (Lcomp->useflags & usedby)) return Lcomp;
	else return NULL;
#endif
}

/**********************************************************************/
static inline int
findornewLcomp(const uuid_t cid, Lcomponent_t **Lcomp, enum useflags_e usedby)
{
#if CONFIG_SINGLE_COMPONENT
	int isnew;

	*Lcomp = &localComponent;
	isnew = !localComponent.useflags;
	localComponent.useflags |= usedby;
	return isnew;
#else
	Lcomponent_t *lc;
	uuidhd_t *uuidp;
	int isnew;

	findornewuuid(&Lcomponents, cid, &uuidp, sizeof(Lcomponent_t));
	lc = container_of(uuidp, Lcomponent_t, hd);
	isnew = (lc->useflags & usedby) == 0;
	lc->useflags |= usedby;
	*Lcomp = lc;
	return isnew;
#endif
}

/**********************************************************************/
#if CONFIG_SINGLE_COMPONENT
#define releaseLcomponent(Lcomp, useby) \
						(Lcomp->useflags &= ~(useby))
#else
static inline void
releaseLcomponent(struct Lcomponent_s *Lcomp, enum useflags_e usedby)
{
	Lcomp->useflags &= ~usedby;
   if (Lcomp->useflags) return;
   unlinkuuid(&Lcomponents, &Lcomp->hd);
   free(Lcomp);
}
#endif    /* !CONFIG_SINGLE_COMPONENT */

/**********************************************************************/
typedef void Lcompiterfn(Lcomponent_t *);

#if CONFIG_SINGLE_COMPONENT
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
#endif    /* !CONFIG_SINGLE_COMPONENT */

/**********************************************************************/
static inline Rcomponent_t *
findRcomp(const uint8_t *cid, enum useflags_e usedby)
{
	Rcomponent_t *Rcomp;
	Rcomp = container_of(finduuid(&Rcomponents, cid), Rcomponent_t, hd);
	if (Rcomp && (Rcomp->useflags & usedby)) return Rcomp;
	else return NULL;
}

/**********************************************************************/
static inline int
findornewRcomp(const uuid_t cid, Rcomponent_t **Rcomp, enum useflags_e usedby)
{
	Rcomponent_t *rc;
	uuidhd_t *uuidp;
	int isnew;

	findornewuuid(&Rcomponents, cid, &uuidp, sizeof(Rcomponent_t));
	rc = container_of(uuidp, Rcomponent_t, hd);
	isnew = (rc->useflags & usedby) == 0;
	rc->useflags |= usedby;
	*Rcomp = rc;
	return isnew;
}

/**********************************************************************/
static inline void
releaseRcomponent(struct Rcomponent_s *Rcomp, enum useflags_e usedby)
{
 	Rcomp->useflags &= ~usedby;
	if (Rcomp->useflags) return;
   unlinkuuid(&Rcomponents, &Rcomp->hd);
   free(Rcomp);
}

/**********************************************************************/
typedef void Rcompiterfn(Rcomponent_t *);

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

int init_Lcomponent(Lcomponent_t *Lcomp, uuid_t cid);

/*
	func: components_init
	
	Initialize component handling
	
	return:
	0 on success, -1 on fail
*/
extern int components_init(void);

#endif  /* __component_h__ */
