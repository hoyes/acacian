/**********************************************************************/
/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (c) 2013, Acuity Brands, Inc.

Author: Philip Nye <philip.nye@engarts.com>

This file forms part of Acacian a full featured implementation of 
ANSI E1.17 Architecture for Control Networks (ACN)

#tabs=3
*/
/**********************************************************************/
/*
header: acnlists.h

Generic handling for single and double linked lists.
*/

#ifndef __acnlists_h__
#define __acnlists_h__ 1

/************************************************************************/
/*
   macros:  Single linked lists

   Each list member structure contains one pointer lnk.r pointing to its
   right neighbour. The last item in the list has a NULL pointer. The
   head of list pointer used to reach the list identifes the head item
   and the list progresses to the right.

   A pointer to any list member can be used as an insertion point for
   new items. The macro slInsertR() adds an item immediately to the
   right of this member and evaluates as a pointer to the new member.

   To unlink an item is expensive if the list is long and requires a
   temporary pointer - the type parameter is the type of the linked
   structure e.g. "struct mylinkedstruct"
  
slLink - declare a link structure
slInsertR - insert an object after itemp
slAddHead - insert an object at the head of the list
slUnlink - unlink an object
*/
#define slLink(type, lnk) struct {type *r;} lnk

#define slInsertR(itemp, newp, lnk) (newp->lnk.r = (itemp)->lnk.r, (itemp)->lnk.r = newp)
#define slAddHead(headp, newp, lnk) (newp->lnk.r = headp, headp = newp)
#define slUnlink(type, headp, itemp, lnk) \
   {  if (headp == (itemp)) headp = (itemp)->lnk.r;\
      else {\
         type *lp = headp;\
         for (lp = headp; lp; lp = lp->lnk.r)\
            if (lp->lnk.r == (itemp)) {\
               lp->lnk.r = (itemp)->lnk.r;\
               break;\
            }\
      }\
   }

/*
   macros: Single linked lists with pointer to tail

   Each list member structure contains one pointer lnk.r pointing to its
   right neighbour. The last item in the list has a NULL pointer. The
   head of list pointer used to reach the list identifes the head item
   and the list progresses to the right. A tail pointer identifies the
   last item.

   A pointer to any list member can be used as an insertion point for
   new items. The macro slInsertR() adds an item immediately to the
   right of this member and evaluates as a pointer to the new member.

   To unlink an item is expensive if the list is long and requires a
   temporary pointer - the type parameter is the type of the linked
   structure e.g. "struct mylinkedstruct"

stlLink - declare a link structure
stlInsertR - insert an object after itemp
stlAddHead - insert an object at the head of the list
stlAddTail - insert an object at the tail of the list
stlUnlink - unlink an object
*/
#define stlLink(type, lnk) struct {type *r;} lnk

#define stlInsertR(itemp, tailp, newp, lnk) {\
      if (((newp)->lnk.r = (itemp)->lnk.r) == NULL) (tailp) = (newp);\
      (itemp)->lnk.r = (newp);\
   }

#define stlAddHead(headp, tailp, newp, lnk) {\
      if (((newp)->lnk.r = (headp)) == NULL) (tailp) = (newp);\
      headp = newp;\
   }

#define stlAddTail(headp, tailp, newp, lnk) {\
      (newp)->lnk.r = NULL;\
      if (tailp) (tailp)->lnk.r = (newp);\
      else (headp) = (newp);\
      (tailp) = (newp);\
   }

#define stlUnlink(type, headp, tailp, itemp, lnk) {\
      if ((headp) == (itemp)) {\
         if (((headp) = (itemp)->lnk.r) == NULL) (tailp) = NULL;\
      else {\
         type *lp;\
         for (lp = (headp); lp; lp = lp->lnk.r)\
            if (lp->lnk.r == (itemp)) {\
               if ((lp->lnk.r = (itemp)->lnk.r) == NULL)\
                  (tailp) = lp;\
               break;\
            }\
      }\
   }

/*
   macros: Double linked lists

   Each list member structure contains two pointers lnk.r and lnk.l
   pointing to its right and left neighbours. These pointers form a ring
   so that the tail of the list joins to the head. The head of list
   pointer used to reach the list identifes the head item and the list
   progresses to the right, so the tail item is the left neighbour of
   the head (the head/tail break comes between the head item and its
   left neighbour.

   A pointer to any list member can be used as a handle on the list and
   defines an insertion point for new items. The macros dlInsertL() and
   dlInsertR() add an item immediately to the left or right of this
   member and evaluate as a pointer to the new member.

dlLink - declare a link structure
dlInsertR - insert an object to the right of itemp
dlInsertL - insert an object to the left of itemp
dlAddHead - insert an object at the head of the list
dlAddTail - insert an object at the tail of the list
dlUnlink - unlink an object
*/

#define dlLink(type, lnk) struct {type *l; type *r;} lnk

#define dlInsertR(itemp, newp, lnk) (newp->lnk.l = (itemp), (itemp)->lnk.r = (newp->lnk.r = (itemp)->lnk.r)->lnk.l = newp) 
#define dlInsertL(itemp, newp, lnk) (newp->lnk.r = (itemp), (itemp)->lnk.l = (newp->lnk.l = (itemp)->lnk.l)->lnk.r = newp) 

#define dlAddHead(headp, newp, lnk) (headp = (headp) ? dlInsertL(headp, newp, lnk) : (newp->lnk.l = newp->lnk.r = newp))
#define dlAddTail(headp, newp, lnk) (headp ? dlInsertL(headp, newp, lnk) : (headp = newp->lnk.l = newp->lnk.r = newp))

#define dlUnlink(headp, itemp, lnk) \
   {  if (headp == (itemp)) \
         headp = ((itemp)->lnk.r == (itemp)) ? NULL : (itemp)->lnk.r;\
      ((itemp)->lnk.r->lnk.l = (itemp)->lnk.l)->lnk.r = (itemp)->lnk.r;\
   }

#endif /* __acnlists_h__ */
