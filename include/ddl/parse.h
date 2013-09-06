/**********************************************************************/
/*
Copyright (c) 2010, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3t
*/
/**********************************************************************/
#ifndef __ddl_parse_h__
#define __ddl_parse_h__ 1

#include <expat.h>

/**********************************************************************/
typedef XML_Char ddlchar_t;

#if ACNCFG_OS_LINUX || ACNCFG_OS_BSD || ACNCFG_OS_OSX
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
Rather than parsing multiple modules recursively which can overload 
lightweight systems they are queued up (in the ddl parse context 
structure) to be parsed sequentially.
*/
struct dcxt_s;  /* defined below */

struct qentry_s {
	struct qentry_s *next;
	tok_t modtype;
	void *ref;
	ddlchar_t name[];
};

void queue_dcid(struct dcxt_s *dcxp, struct qentry_s *qentry);

/**********************************************************************/
/*
MAXATTS is the greatest number of known attributes for an element.

Currently the max is in propref_DMP
*/
#define MAXATTS (atts_propref_DMP.ntoks)

/**********************************************************************/
/*
All module types can contain aliases
*/

struct uuidalias_s {
	struct uuidalias_s *next;
	const ddlchar_t *alias;
	ddlchar_t uuidstr[UUID_STR_SIZE];
};

/**********************************************************************/
/*
Languageset
*/
/**********************************************************************/
struct lsetparse_s {
	/* not yet implemented */
};


/**********************************************************************/
/*
Behaviorset
*/
/**********************************************************************/
struct bsetparse_s {
	/* not yet implemented */
};

/**********************************************************************/
/*
Device
*/
/**********************************************************************/
typedef struct prop_s prop_t;	/* defined below */

#define PROP_MAXBVS 32

struct devparse_s {
//	struct prop_s *curdev;
	struct prop_s *curprop;
	struct rootprop_s *root;
	int nbvs;
#if ACNCFG_MAPGEN
	unsigned int propnum;
	unsigned int subdevno;
#endif
	const struct bv_s *bvs[PROP_MAXBVS];
};

/**********************************************************************/
struct langstring_s {
	struct string_s *fallback;
	ddlchar_t *lang;
	ddlchar_t str[];
};

struct string_s {
	struct langstring_s *strs;
	ddlchar_t key[];
};

struct lset_s {
	uint8_t uuid[UUID_SIZE];
	unsigned int nstrings;
	struct string_s *strings;
};

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
#if !ACNCFG_DDLACCESS_DMP
define _DMPPROPSIZE 0
#endif

#if ACNCFG_DDLACCESS_EPI26
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
	struct prop_s *prop;
};

struct idlist_s {
	struct id_s id;
	struct idlist_s *nxt;
};

/* Create a "pseudo" property type for each includedev/subdevice */
#define MAXDEVPATH 128

struct param_s;
struct device_s {
//	uint8_t uuid[UUID_SIZE];
//	struct prop_s *nxtdev;
	struct param_s *params;
	struct uuidalias_s *aliases;
	/*
	nids is negative if ids are in list form (while parsing the subdevice)
	and positive if in array form (after subdevice is complete).
	*/
	int nids;
	struct id_s *ids;
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

struct proptask_s;

/**********************************************************************/
/* Property structure - build a tree of these */
struct prop_s {
	prop_t *parent;
	prop_t *siblings;
	prop_t *children;
	prop_t *arrayprop;   /* points up the tree to nearest ancestral array prop */
	uint32_t childaddr;
	uint32_t array;

	uint32_t childinc;
	struct proptask_s *tasks;
	const ddlchar_t *id;
#if ACNCFG_MAPGEN
	ddlchar_t *cname;
#endif
#if ACNCFG_DDL_LABELS
	struct label_s label;
#endif
	uint16_t pnum;
	uint16_t vtype;
	union {
		struct {
#if ACNCFG_DDLACCESS_DMP
			struct dmpprop_s *dmp;
#endif
#if ACNCFG_DDLACCESS_EPI26
			struct dmxprop_s *dmx;
#endif
		} net;
		struct impliedprop_s impl;
		struct immprop_s imm;
		struct device_s dev;
	} v;
};

#define MAXPROPNAME 32
/**********************************************************************/
/*
rootprop is the root of a device component and includes some extra
information
*/
struct rootprop_s {
	struct prop_s prop;
	uint8_t dcid[UUID_SIZE];
	int nnetprops;
	int nflatprops;
	uint32_t maxaddr;
	uint32_t minaddr;
	union addrmap_u *amap;
};

typedef struct rootprop_s rootprop_t;

/**********************************************************************/
/*
During parsing the content of a property, tasks can be added for 
execution when the property is completed (end tag reached). This 
aids implementation of many behaviors which cannot be evaluated until
the entire content of the propoerty is available.
*/

typedef void proptask_fn(struct dcxt_s *dcxp, struct prop_s *pp, void *ref);

struct proptask_s {
	struct proptask_s *next;
	proptask_fn *task;
	void *ref;
};

void add_proptask(struct prop_s *prop, proptask_fn *task, void *ref);

/**********************************************************************/
/*
DDL parse context structure
*/

struct dcxt_s {
	struct qentry_s *queuehead;
	struct qentry_s *queuetail;
	int nestlvl;
	int skip;
	tok_t elestack[ACNCFG_DDL_MAXNEST];
	tok_t elprev;
	int elcount;
	unsigned int arraytotal;
	XML_Parser parser;
	int txtlen;
	union {
		const ddlchar_t *p;
		ddlchar_t ch[ACNCFG_DDL_MAXTEXT];
	} txt;
	union {
		struct devparse_s dev;
		struct lsetparse_s lset;
		struct bsetparse_s bset;
	} m;
	/* struct strbuf_s *strs; */
};
#define NOSKIP (-1)
#define SKIPPING(dcxp) ((dcxp)->skip >= 0)

/**********************************************************************/
#define savestr(s) strdup(s)
#define freestr(s) free(s)

struct rootprop_s *parsedevice(const char *dcidstr);
void freeprop(struct prop_s *prop);
void freerootprop(struct rootprop_s *root);
char *flagnames(uint32_t flags, const char **names, char *buf, const char *format);
struct prop_s *itsdevice(struct prop_s *prop);

#endif  /* __ddl_parse_h__ */
