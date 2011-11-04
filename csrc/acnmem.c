/************************************************************************/
/*
Copyright (c) 2010, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3s
*/
/************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "acncommon.h"
#include "acnmem.h"
#include "acnlog.h"

/************************************************************************/
//#define lgFCTY LOG_NETX
#define lgFCTY LOG_OFF
/************************************************************************/

const uint16_t tsizes[] = {
   palign(t0size),
   palign(t1size),
   palign(t2size),
   palign(t3size),
   palign(t4size),
   palign(t5size),
   palign(t6size),
   palign(t7size),
   palign(t8size),
   palign(t9size),
   palign(t10size),
   palign(t11size)
};

static const uint16_t allocations[] = {
   128,
   128,
   512,
   512,
   512,
   2048,
   2048,
   2048,
   8192,
   8192,
   8192,
   32768
};

uint8_t *freelist[arraycount(tsizes)];

/************************************************************************/
int
startACNmem(void)
{
   return 0;
}

/************************************************************************/
/*
Allocate a block by size index
*/

void * __attribute__ ((__noinline__))
_acnAllocx(unsigned int sizex)
{
   uint8_t *vp;

   acnlog(lgDBUG, "+ %s size index %d", __func__, sizex);
   if ((vp = freelist[sizex]) == NULL) {
      if ((vp = malloc(allocations[sizex]))) {
         uint8_t *xp;
         int bsz = tsizes[sizex];

         acnlogmark(lgDBUG, "Adding new memory block %d/%d", allocations[sizex], bsz);
         for (xp = vp; xp < vp + allocations[sizex] - (2 * bsz - 1); xp += bsz)
            *(uint8_t **)xp = xp + bsz;
         *(uint8_t **)xp = NULL;
         freelist[sizex] = vp + bsz;
      } else {
#if CONFIG_EXIT_NOMEM
         exit(EXIT_FAILURE);
#else
         errno = ENOMEM;
#endif
      }
   } else {
      freelist[sizex] = *(uint8_t **)vp;
   }
   LOG_FEND(lgFCTY);
   return vp;
}

/************************************************************************/
/*
Same as _acnAllocx() but zero the block
*/

void *
_acnAllocxz(unsigned int sizex)
{
   uint8_t *vp;

   if ((vp = _acnAllocx(sizex))) memset(vp, 0, tsizes[sizex]);
   return vp;
}

/************************************************************************/
/*
Free a block by size index
*/

void
_acnFreex(void *vp, unsigned int sizex)
{
   *(void **)vp = freelist[sizex];
   freelist[sizex] = vp;
}

/************************************************************************/
static int
_mtnum(int size)
{
   int lo, hi, i;

   lo = 0; hi = arraycount(tsizes);
   do {
      i = (lo + hi) >> 1;
      if (size <= tsizes[i]) hi = i;
      else lo = i + 1;
   } while (lo < hi);
   return lo;
}

/************************************************************************/
/*
Allocate a block by size
*/
void *
_acnAlloc(unsigned int size)
{
   return _acnAllocx(_mtnum(size));
}

/************************************************************************/
/*
De-allocate a block by size
*/
void
_acnFree(void *vp, unsigned int size)
{
   _acnFreex(vp, _mtnum(size));
}

