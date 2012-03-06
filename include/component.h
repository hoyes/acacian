
/************************************************************************/
/*
Copyright (c) 2011, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3
*/
/************************************************************************/

#ifndef __component_h__
#define __component_h__ 1

#include "uuid.h"
#include "mcastalloc.h"
#if CONFIG_SDT
#include "sdt.h"
#endif
#if CONFIG_DMP
#include "dmp.h"
#endif

typedef struct Lcomponent_s Lcomponent_t;
typedef struct Rcomponent_s Rcomponent_t;

#define NCOMPONENTUSES (CONFIG_SDT + CONFIG_DMP + (defined(app_Lcomp_t)))
enum useflags_e {
	USEDBY_SDT = 1,	
	USEDBY_DMP = 2,	
	USEDBY_APP = 4,	
};

struct Lcomponent_s {
	uuidhd_t hd;
#if NCOMPONENTUSES > 1
	uint8_t useflags;
#endif
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

struct Rcomponent_s {
	uuidhd_t hd;
#if NCOMPONENTUSES > 1
	uint8_t useflags;
#endif
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

#if CONFIG_SINGLE_COMPONENT
extern Lcomponent_t localComponent;
#else
uuidset_t Lcomponents;
#endif
uuidset_t Rcomponents;

#if CONFIG_SINGLE_COMPONENT

#define findLcomp(cid) (&localComponent)
#define releaseLcomponent(Lcomp, useby) (Lcomp->useflags = 0)

#else   /* !CONFIG_SINGLE_COMPONENT */

static inline Lcomponent_t *
findLcomp(const uuid_t cid)
{
	return container_of(finduuid(&Lcomponents, cid), Lcomponent_t, hd);
}

static inline int
findornewLcomp(const uuid_t cid, Lcomponent_t **Lcomp)
{
	return findornewuuid(&Lcomponents, cid,
							(uuidhd_t **)Lcomp, sizeof(Lcomponent_t));
}

static inline void
releaseLcomponent(struct Lcomponent_s *Lcomp, enum useflags_e usedby)
{
#if NCOMPONENTUSES > 1
	Lcomp->useflags &= ~usedby;
   if (Lcomp->useflags) return;
#endif
   unlinkuuid(&Lcomponents, &Lcomp->hd);
   free(Lcomp);
}
#endif   /* !CONFIG_SINGLE_COMPONENT */


static inline Rcomponent_t *
findRcomp(const uint8_t *cid)
{
	return container_of(finduuid(&Rcomponents, cid), Rcomponent_t, hd);
}

static inline int
findornewRcomp(const uuid_t cid, Rcomponent_t **Rcomp)
{
	return findornewuuid(&Rcomponents, cid,
							(uuidhd_t **)Rcomp, sizeof(Rcomponent_t));
}

static inline void
releaseRcomponent(struct Rcomponent_s *Rcomp, enum useflags_e usedby)
{
#if NCOMPONENTUSES > 1
 	Rcomp->useflags &= ~usedby;
	if (Rcomp->useflags) return;
#endif
   unlinkuuid(&Rcomponents, &Rcomp->hd);
   free(Rcomp);
}

#endif  /* __component_h__ */
