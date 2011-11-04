
/************************************************************************/
/*
Copyright (c) 2010, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3s
*/
/************************************************************************/

#define t0size    25
#define t1size    42
#define t2size    64
#define t3size    102
#define t4size    170
#define t5size    256
#define t6size    409
#define t7size    682
#define t8size    1024
#define t9size    1638
#define t10size   2730
#define t11size   4096
#define t12size   65536
#define t13size   (65536 * sizeof(void *))

/*
preferred power of two allocations for rapidly growing arrays e.g. used
by memberspace
*/
#define pp0size t5size
#define pp1size t11size
#define pp2size t12size
#define pp3size t13size


/* align to pointer type */
#define palign(n) ((n) - ((n) % sizeof(void *)))

#define mtnum(x) (\
   ((x) <= palign(t0size))? 0:\
   ((x) <= palign(t1size))? 1:\
   ((x) <= palign(t2size))? 2:\
   ((x) <= palign(t3size))? 3:\
   ((x) <= palign(t4size))? 4:\
   ((x) <= palign(t5size))? 5:\
   ((x) <= palign(t6size))? 6:\
   ((x) <= palign(t7size))? 7:\
   ((x) <= palign(t8size))? 8:\
   ((x) <= palign(t9size))? 9:\
   ((x) <= palign(t10size))? 10:\
   11)

extern int startACNmem(void);

/* functions with sizex arg take a size index */
extern void *_acnAllocx(unsigned int sizex);
extern void *_acnAllocxz(unsigned int sizex);
extern void _acnFreex(void *vp, unsigned int sizex);

/* these functions take a size and calculate the required index */
extern void *_acnAlloc(unsigned int size);
extern void _acnFree(void *vp, unsigned int size);

/*
NOTE
If the size to be allocated can be calculated at compile time (e.g.
using sizeof(x)) then function mtnum will optimize away to a constant
and these macros are the most efficient calls.

If size cannot be calculated at compile time, use _acnAlloc and 
_acnFree which uses a more efficient calculation _mtnum
*/
#define acnNew(type) (type *)_acnAllocxz(mtnum(sizeof(type)))
#define acnFree(ptr, type) _acnFreex((void *)(ptr), mtnum(sizeof(type)))

#define acnAlloc(size) (uint8_t *)_acnAllocx(mtnum(size))
#define acnAllocz(size) (uint8_t *)_acnAllocxz(mtnum(size))
#define acnDealloc(ptr, size) _acnFreex((void *)(ptr), mtnum(size))
