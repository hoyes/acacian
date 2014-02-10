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
header: parse.h

DDL parser
*/

#ifndef __ddl_parse_h__
#define __ddl_parse_h__ 1

#include <expat.h>

/**********************************************************************/
typedef XML_Char ddlchar_t;

#if CF_OS_LINUX || CF_OS_BSD || CF_OS_OSX
#define PATHSEP ':'
#define DIRSEP '/'
#endif

/**********************************************************************/
typedef uint8_t tok_t;

struct allowtok_s {
	tok_t ntoks;
	tok_t toks[];
};

enum token_e;

/**********************************************************************/
/*
MAXATTS is the greatest number of known attributes for an element.

Currently the max is in propref_DMP
*/
#define MAXATTS (atts_propref_DMP.ntoks)

/**********************************************************************/
/*
pre-declare some structures
*/
struct pool_s;
struct pblock_s;
struct bv_s;
struct dcxt_s;
struct devtask_s;

/**********************************************************************/

struct hashtab_s {
	const ddlchar_t ***v;
	struct pool_s *pool;
	unsigned char power;
	size_t used;
};

//const ddlchar_t **findkey(struct hashtab_s *table, const ddlchar_t *name);
const ddlchar_t **findornewkey(struct hashtab_s *table,
			const ddlchar_t *name, size_t *create);

#define findkey(table, key) findornewkey((table), (key), NULL)

#define KEY_NOMEM -1
#define KEY_ALREADY 1

int addkey(struct hashtab_s *table, const ddlchar_t **entry);

/**********************************************************************/
struct pool_s {
	ddlchar_t *nxtp;
	ddlchar_t *endp;
	ddlchar_t *ptr;
	struct pblock_s *blocks;
};

void pool_init(struct pool_s *pool);
void pool_reset(struct pool_s *pool);
const ddlchar_t *pool_appendstr(struct pool_s *pool, const ddlchar_t *s);
const ddlchar_t *pool_appendn(struct pool_s *pool, const ddlchar_t *s, int n);
void pool_dumpstr(struct pool_s *pool);
const ddlchar_t *pool_addstr(struct pool_s *pool, const ddlchar_t *s);
const ddlchar_t *pool_termstr(struct pool_s *pool);
const ddlchar_t *pool_addstrn(struct pool_s *pool, const ddlchar_t *s, int n);
const ddlchar_t *pool_addfoldsp(struct pool_s *pool, const ddlchar_t *s);
/* can't currently delete a single string */
#define pool_delstr(pool, str)

/**********************************************************************/
#define MAX_REFINES 6

typedef void bvaction(struct dcxt_s *dcxp, const struct bv_s *bv);

struct bv_s {
	const ddlchar_t *name;
	bvaction *action;
	int nrefines;
	struct bv_s **refa;
};
/*
macros: Special values for nrefines

BV_NULL - No refinements and no action.
BV_NEW - Behavior has been created because it was referenced 
but has not yet been parsed.
*/
#define BV_NEW -1
#define BV_NULL -2
#define BV_FINAL -3

struct bvinit_s {
	const char *name;
	bvaction *action;
};

struct bvset_s {
	uint8_t uuid[UUID_SIZE];
	bool parsed;
	struct hashtab_s hasht;
};

extern struct uuidset_s kbehaviors;
extern const struct bv_s *findbv(const uint8_t *uuid, const ddlchar_t *name, struct bvset_s **bvset);
/**********************************************************************/
/*
All module types can contain aliases
*/

struct uuidalias_s {
//	struct uuidalias_s *next;
	const ddlchar_t *alias;
	const ddlchar_t *uuidstr;
};

/**********************************************************************/
#if 0
struct string_s {
	ddlchar_t *langkey;  /* langkey = "<lang_index><key>" */
	ddlchar_t *text;
};

struct language_s {
	ddlchar_t *tag;
	uint16_t nkeys;
	uint8_t altlang;
};

struct lset_s {
	uint8_t uuid[UUID_SIZE];
	struct hashtab_s hasht;
	uint8_t nlangs;
	uint8_t dfltlang;
	uint8_t user1;
	uint8_t user2;
	struct language_s langs[MAXLANGS];
};
#else
#define LSET_MAXLANGS 16
#define NO_LANG 0xffff

struct string_s {
	const ddlchar_t *key;
	const ddlchar_t *text[LSET_MAXLANGS];
};

struct language_s {  /* hash as "@en-GB"? */
	const ddlchar_t *tag;
	int16_t altlang;
	unsigned int nkeys;
};

struct lset_s {
	uint8_t uuid[UUID_SIZE];
	struct hashtab_s hasht;
	struct language_s *languages;  /* nlangs = languages->index */
	int16_t nlangs;
	int16_t userlang;
};
#endif
/**********************************************************************/
/*
Property tree
*/
enum vtype_e {
	/*
	WARNING: order of non-pseudo types must exactly match lexical 
	order in proptype_allow (see parse.c)
	*/
	VT_NULL,
	VT_imm_unknown,
	VT_implied,
	VT_network,
	/* Remainder are pseudo types */
	VT_include,
	VT_device,
	VT_alias,
	/*
	Immediate pseudo types
	WARNING: order of these pseudo-types must exactly match lexical 
	order in valtype_allow (see parse.c)
	*/
	VT_imm_float,
	VT_imm_object,
	VT_imm_sint,
	VT_imm_string,
	VT_imm_uint,
	VT_maxtype
};
#define VT_imm_FIRST VT_imm_float

typedef uint16_t vtype_t;
/* define VT_maxtype rather than including in enumeration to avoid
lots of warnings of incomplete enumeration switches */
//#define VT_maxtype (VT_imm_object + 1)

extern const ddlchar_t *modnames[];
extern const ddlchar_t *ptypes[];
extern const ddlchar_t *etypes[];

/**********************************************************************/
/*
Property type specific info
*/

struct impliedprop_s {
	unsigned int flags;
	uintptr_t val;
};

/**********************************************************************/
#if !CF_DDLACCESS_DMP
define _DMPPROPSIZE 0
#endif

#if CF_DDLACCESS_EPI26
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
immediate properties have a range of data types and may be arrays. 
Simple values are stored in the structure while arbitrary objects, 
strings or arrays get linked to it by reference.
*/
struct immobj_s {
	int size;
	uint8_t *data;
};

struct immprop_s {
	uint32_t count;
	union {
		uint32_t ui;
		int32_t si;
		double f;
		const ddlchar_t *str;
		struct immobj_s obj;
		uint32_t *Aui;
		int32_t *Asi;
		double *Af;
		const ddlchar_t **Astr;
		struct immobj_s *Aobj;
		void *ptr;
	} t;
};

struct id_s {
	const ddlchar_t *id;
	struct ddlprop_s *prop;
};

struct idlist_s {
	struct id_s id;
	struct idlist_s *nxt;
};

/* Create a "pseudo" property type for each includedev/subdevice */
#define MAXDEVPATH 128

struct param_s;
struct device_s {
	uint8_t dcid[UUID_SIZE];
//	struct ddlprop_s *nxtdev;
	struct param_s *params;
};

//#define MAXSETPARAMLEN 62
/* Store list of parameters for each includedev/subdevice */
struct param_s {
	struct param_s *nxt;
	const ddlchar_t *name;
	const ddlchar_t *subs;
};

struct label_s {
	struct lset_s *set;
	const ddlchar_t *txt;  /* txt is the literal if set is NULL, else the key */
};

/**********************************************************************/
/* Property structure - build a tree of these */
struct ddlprop_s {
	struct ddlprop_s *parent;
	struct ddlprop_s *siblings;
	struct ddlprop_s *children;
	struct ddlprop_s *arrayprop;   /* points up the tree to nearest ancestral array prop */
	struct bv_s **bva;
	const ddlchar_t *id;
#if CF_DDL_STRINGS
	struct label_s label;
#endif
	uint32_t array;
#if CF_DDLACCESS_DMP
	int32_t inc;
	uint32_t childaddr;
	uint32_t childinc;
#endif
	uint16_t pnum;
	uint16_t vtype;
	union {
		struct {
#if CF_DDLACCESS_DMP
			struct dmpprop_s *dmp;
#endif
#if CF_DDLACCESS_EPI26
			struct dmxprop_s *dmx;
#endif
		} net;
		struct impliedprop_s impl;
		struct immprop_s imm;
		struct device_s dev;
		struct ddlprop_s *alias;
	} v;
};

/**********************************************************************/
/*
macro: FOR_EACH_PROP

The structure of the tree allows us to iterate over all properties 
without recursion or using a stack. This works because all properties 
along the sibling axis point to the same parent.
*/
#define FOR_EACH_PROP(pp)\
{int _pdepth = 0;\
	while (1) {
		/* do stuff here */

#define NEXT_PROP(pp)\
		if (pp->children) {\
			pp = pp->children;\
			++_pdepth;\
			continue;\
		}\
__prop_iter:\
		if (pp->siblings) {\
			pp = pp->siblings;\
			continue;\
		}\
		if (_pdepth-- == 0) break;\
		pp = pp->parent; goto __prop_iter;\
	}}

/**********************************************************************/
/*
rootprop is the root of a device component and includes some extra
information
*/
struct rootdev_s {
	uint8_t dcid[UUID_SIZE];
	struct ddlprop_s *ddlroot;
	struct pool_s strpool;
	struct hashtab_s idtab;
#if CF_DDLACCESS_DMP
	union addrmap_u *amap;
	struct dmpprop_s *dmpprops;
	int nnetprops;
	int nflatprops;
	uint32_t maxaddr;
	uint32_t minaddr;
#endif
};

/**********************************************************************/
/*
During parsing, tasks can be added for execution when the root 
device is completed (including subdevices). This aids implementation 
of behaviors which cannot be evaluated until the entire content of 
the property is available.
Two user reference pointers are provided since tasks will often need
a reference to a property as well as to some task related info and this
avoids creating yet another small structure.
*/

typedef void devtask_fn(struct rootdev_s *rootdev, void *taskdata);

struct devtask_s {
	struct devtask_s *nxt;
	devtask_fn *task;
	char taskdata[];
};

void *add_devtask(struct dcxt_s *dcxp, devtask_fn *task, size_t size);

/**********************************************************************/

struct qentry_s;  /* defined in parse.c */

#define BV_MAXREFINES 32
#define PROP_MAXBVS 32
#define MAXALIASES 20
/**********************************************************************/
/*
DDL parse context structure
*/

struct dcxt_s {
	struct qentry_s *queuehead;
	struct qentry_s *queuetail;
	int nestlvl;
	int skip;
	tok_t elestack[CF_DDL_MAXNEST];
	tok_t elprev;
	int elcount;
	XML_Parser parser;
	int txtlen;
	union {
		const ddlchar_t *p;
		ddlchar_t ch[CF_DDL_MAXTEXT];
	} txt;
	struct pool_s parsepool;
	struct pool_s modulepool;
	int naliases;
	struct uuidalias_s aliases[MAXALIASES];
	struct devtask_s *tasks;
	unsigned int arraytotal;
	struct rootdev_s *rootdev;
	unsigned int subdevno;
	union {
		struct {
			struct bvset_s *curset;
			struct bv_s *curbv;
			int nrefines;
			const struct bv_s *refines[BV_MAXREFINES];
		} bset;
		struct {
			struct lset_s *curset;
			struct string_s *curstr;
#if CF_STR_FOLDSPACE
			bool foldsp;
#endif
			unsigned int nkeys;
			int16_t nlangs;
			int16_t curlang;
			struct language_s languages[LSET_MAXLANGS];
		} lset;
		struct {
			struct ddlprop_s *curprop;
			int nbvs;
			const struct bv_s *bvs[PROP_MAXBVS];
			uint16_t propnum;
		} dev;
	} m;
};
#define NOSKIP (-1)
#define SKIPPING(dcxp) ((dcxp)->skip >= 0)
#define SKIPON(dcxp) ((dcxp)->skip = (dcxp)->nestlvl)

/**********************************************************************/
extern struct uuidset_s langsets;
extern struct uuidset_s behaviorsets;
extern struct uuidset_s devtrees;
/**********************************************************************/

struct rootdev_s *parseroot(const char *dcidstr);
void freerootdev(struct rootdev_s *root);
char *flagnames(uint32_t flags, const char **names, char *buf, const char *format);
struct ddlprop_s *itsdevice(struct ddlprop_s *prop);
const ddlchar_t *resolveuuid(struct dcxt_s *dcxp, const ddlchar_t *name, uint8_t *dcid);

enum pname_flags_e {
	pn_translate = 1,
	pn_path = 2,
};
const char *propname(struct ddlprop_s *pp, enum pname_flags_e flags);
#define propcname(pp) propname((pp), pn_translate| pn_path)
#define propxname(pp) propname((pp), 0)
#define propxpath(pp) propname((pp), pn_path)

void setlang(const ddlchar_t **ltags);
const ddlchar_t *lblookup(struct label_s *lbl);

#endif  /* __ddl_parse_h__ */
