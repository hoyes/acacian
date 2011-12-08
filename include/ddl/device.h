/************************************************************************/
/*
Copyright (c) 2010, Engineering Arts (UK)
All rights reserved.

  $Id$

#tabs=3s
*/
/************************************************************************/
#ifndef __ddl_parse_h__
#define __ddl_parse_h__ 1

typedef XML_Char ddlchar_t;

/*
MAXATTS is the greatest number of known attributes in an attribute 
parse string (see use in el_start). It does not include 
xxx.paramname attributes which are automatically handled and do not 
appear in the parse string

Currently the max is in propref_DMP_atts
*/
#define MAXATTS 9

/*
Enumerate the elements.
*/
enum element_e {
	ELx_ROOT,
	EL_DDL,
	EL_UUIDname,
	EL_alternatefor,
	EL_behavior,
	EL_behaviordef,
	EL_behaviorset,
	EL_childrule_DMP,
	EL_choice,
	EL_device,
	EL_extends,
	EL_hd,
	EL_EA_propext,
	EL_includedev,
	EL_label,
	EL_language,
	EL_languageset,
	EL_maxinclusive,
	EL_mininclusive,
	EL_p,
	EL_parameter,
	EL_property,
	EL_propertypointer,
	EL_propmap_DMX,
	EL_propref_DMP,
	EL_protocol,
	EL_refinement,
	EL_refines,
	EL_section,
	EL_setparam,
	EL_string,
	EL_useprotocol,
	EL_value,
	EL_MAX,
};

/**********************************************************************/
/*
Property tree
*/
typedef struct prop_s prop_t;

typedef enum vtype_e {
	VT_NULL,
	VT_imm_unknown,
	VT_implied,
	VT_network,
	VT_include,
	VT_imm_uint,
	VT_imm_sint,
	VT_imm_float,
	VT_imm_string,
	VT_imm_object,
} vtype_t;

enum proptype_e {   /* encoding type */
    etype_none = 0,
    etype_boolean,
    etype_sint,
    etype_uint,
    etype_float,
    etype_UTF8,
    etype_UTF16,
    etype_UTF32,
    etype_string,
    etype_enum,
    etype_opaque,
    etype_uuid,
    etype_bitmap
};

struct netprop_s {
	prophd_t hd;
//	propaccess_fn *access;
    enum proptype_e etype;
	unsigned int flags;
	unsigned int size;
	void *accessref;
};

struct impliedprop_s {
	unsigned int flags;
	uintptr_t val;
};

#define MAXSETPARAMLEN 62

struct param_s {
	struct param_s *nxt;
	const ddlchar_t *name;
	const ddlchar_t *subs;
};

struct device_s {
	uuid_t uuid;
	struct prop_s *nxtdev;
	struct param_s *params;
	struct uuidalias_s *aliases;
};

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
		ddlchar_t **Astr;
		struct immobj_s *Aobj;
	} t;
};

struct proptask_s;

struct prop_s {
	prop_t *parent;
	prop_t *siblings;
	prop_t *children;
	uint32_t childaddr;
	uint32_t array;
	uint32_t arraytotal;
	uint32_t childinc;
	struct proptask_s *tasks;
	const ddlchar_t *id;
    enum propflags_e flags;
	vtype_t vtype;
	union {
		const ddlchar_t *subs;
		struct netprop_s net;
		struct impliedprop_s impl;
		struct immprop_s imm;
		struct device_s dev;
	} v;
};

typedef struct uuidalias_s uuidalias_t;
typedef struct dcxt_s dcxt_t;
typedef struct behavior_s behavior_t;


/**********************************************************************/
struct uuidalias_s {
	struct uuidalias_s *next;
	uuid_t uuid;
	const ddlchar_t *alias;
};


/**********************************************************************/
#define MAX_NEST 256

struct dcxt_s {
	int nestlvl;
	uint8_t elestack[MAX_NEST];
	int skip;
	int nprops;
	struct prop_s rootprop;
	struct prop_s *curdev;
	struct prop_s *lastdev;
	struct prop_s *curprop;
	int elcount;
	XML_Parser parser;
	int txtlen;
	union {
		const ddlchar_t *p;
		ddlchar_t ch[512];
	} txt;
	/* struct strbuf_s *strs; */
};


typedef void proptask_fn(struct dcxt_s *dcxp, struct prop_s *pp, void *ref);
void add_proptask(struct prop_s *prop, proptask_fn *task, void *ref);
unsigned int savestr(const ddlchar_t *str, const ddlchar_t **copy);
void parsedevx(struct dcxt_s *dcxp);
void freedev(struct dcxt_s *dcxp);

#endif  /* __ddl_parse_h__ */
