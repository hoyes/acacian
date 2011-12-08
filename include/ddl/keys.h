/************************************************************************/
/*
Copyright (c) 2011, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3s
*/
/************************************************************************/
#ifndef __ddlkeys_h__
#define __ddlkeys_h__ 1


typedef struct ddlkey_s ddlkey_t;
/* used to test bits in the radix tree may vary with optimization 
for different architectures */
typedef unsigned int keytst_t;

struct ddlkey_s {
	const uint8_t *uuid;
	const ddlchar_t *name;
	unsigned int namelen;
	keytst_t tstloc;
	struct ddlkey_s *nxt[2];
};

ddlkey_t *findkey(ddlkey_t **set, const uuid_t uuid, const ddlchar_t *name);
int findornewkey(ddlkey_t **set, uuid_t uuid, const ddlchar_t *name, ddlkey_t **rslt, size_t size);
int addkey(ddlkey_t **set, ddlkey_t *np);
int unlinkkey(ddlkey_t **set, ddlkey_t *uup);
int delkey(ddlkey_t **set, ddlkey_t *uup, size_t size);

#endif  /* __ddlkeys_h__ */
