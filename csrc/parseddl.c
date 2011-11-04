/**********************************************************************/
/*

	Copyright (C) 2011, Engineering Arts. All rights reserved.

	Author: Philip Nye

	$Id$

#tabs=3
*/
/**********************************************************************/

#include <stdio.h>
#include <errno.h>
#include <expat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include "acncommon.h"
#include "acnlog.h"
#include "acnmem.h"
#include "propmap.h"
#include "uuid.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL
/**********************************************************************/
const char ddl_location[] = "/home/philip/engarts/acn/ddl/modules/ddl-modules/%";

#define BUFF_SIZE 2048
#define MAX_NEST 256
#define BNAMEMAX 32
#define MAXIDLEN 64
/**********************************************************************/
/*
Grammar summary:
  "@ xxx" is a primary element
  "& xxx" are allowable sub-elements
  "# xxx" are allowable attributes

@ DDL
& behaviorset
& device
& languageset
# version
# xmlns:ea
# http://www.w3.org/XML/1998/namespace id

@ label
& -text-
# label.paramname
# set
# set.paramname
# key
# key.paramname
# http://www.w3.org/XML/1998/namespace id

@ alternatefor
# UUID
# UUID.paramname
# http://www.w3.org/XML/1998/namespace id

@ extends
# UUID
# UUID.paramname
# http://www.w3.org/XML/1998/namespace id

@ UUIDname
# name
# UUID
# http://www.w3.org/XML/1998/namespace id

@ languageset
& UUIDname
& label
& alternatefor
& extends
& language
# UUID
# provider
# date
# http://www.w3.org/XML/1998/namespace id

@ language
& label
& string
# lang
# altlang
# http://www.w3.org/XML/1998/namespace id

@ string
& -text-
# key
# http://www.w3.org/XML/1998/namespace id

@ behaviorset
& UUIDname
& label
& alternatefor
& extends
& behaviordef
# UUID
# provider
# date
# http://www.w3.org/XML/1998/namespace id

@ behaviordef
& label
& refines
& section
# name
# http://www.w3.org/XML/1998/namespace id

@ refines
# set
# name
# http://www.w3.org/XML/1998/namespace id

@ section
& hd
& section
& p
# http://www.w3.org/XML/1998/namespace id

@ hd
& -text-
# http://www.w3.org/XML/1998/namespace id

@ p
& -text-
# http://www.w3.org/XML/1998/namespace space
# http://www.w3.org/XML/1998/namespace id

@ device
& UUIDname
& parameter
& label
& alternatefor
& extends
& useprotocol
& property
& propertypointer
& includedev
# UUID
# provider
# date
# http://www.w3.org/XML/1998/namespace id

@ parameter
& label
& choice
& refinement
& mininclusive
& maxinclusive
# name
# http://www.w3.org/XML/1998/namespace id

@ choice
& -text-
# choice.paramname
# http://www.w3.org/XML/1998/namespace id

@ mininclusive
& -text-
# mininclusive.paramname
# http://www.w3.org/XML/1998/namespace id

@ maxinclusive
& -text-
# maxinclusive.paramname
# http://www.w3.org/XML/1998/namespace id

@ refinement
& -text-
# refinement.paramname
# http://www.w3.org/XML/1998/namespace id

@ property
& label
& behavior
& value
& protocol
& property
& propertypointer
& includedev
# array
# array.paramname
# valuetype
# valuetype.paramname
# sharedefine
# sharedefine.paramname
# http://www.w3.org/XML/1998/namespace id

@ behavior
# set
# set.paramname
# name
# name.paramname
# http://www.w3.org/XML/1998/namespace id

@ value
& -text-
# value.paramname
# type
# type.paramname
# http://www.w3.org/XML/1998/namespace id

@ propertypointer
# ref
# ref.paramname
# http://www.w3.org/XML/1998/namespace id

@ includedev
& label
& protocol
& setparam
# UUID
# UUID.paramname
# array
# array.paramname
# http://www.w3.org/XML/1998/namespace id

@ setparam
& -text-
# name
# setparam.paramname
# http://www.w3.org/XML/1998/namespace id

@ useprotocol
# name
# name.paramname
# http://www.w3.org/XML/1998/namespace id

@ protocol
& propref_DMP
& childrule_DMP
& http://www.engarts.com/namespace/2011/ddlx propext
& propmap_DMX
# name
# name.paramname
# http://www.w3.org/XML/1998/namespace id

@ propref_DMP
# loc
# loc.paramname
# abs
# abs.paramname
# inc
# inc.paramname
# size
# size.paramname
# read
# read.paramname
# write
# write.paramname
# event
# event.paramname
# varsize
# varsize.paramname
# http://www.w3.org/XML/1998/namespace id

@ childrule_DMP
# loc
# loc.paramname
# abs
# abs.paramname
# inc
# inc.paramname
# http://www.w3.org/XML/1998/namespace id

@ propmap_DMX
& -text-
# size
# size.paramname
# inc
# inc.paramname
# propmap_DMX.paramname
# http://www.w3.org/XML/1998/namespace id

@ http://www.engarts.com/namespace/2011/ddlx propext
# name
# value
# http://www.w3.org/XML/1998/namespace id

*/

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

typedef struct token_s {uint8_t tok; char str[];} token_t;

const token_t tok_DDL             = {EL_DDL, "DDL"};
const token_t tok_UUIDname        = {EL_UUIDname, "UUIDname"};
const token_t tok_alternatefor    = {EL_alternatefor, "alternatefor"};
const token_t tok_behavior        = {EL_behavior, "behavior"};
const token_t tok_behaviordef     = {EL_behaviordef, "behaviordef"};
const token_t tok_behaviorset     = {EL_behaviorset, "behaviorset"};
const token_t tok_childrule_DMP   = {EL_childrule_DMP, "childrule_DMP"};
const token_t tok_choice          = {EL_choice, "choice"};
const token_t tok_device          = {EL_device, "device"};
const token_t tok_extends         = {EL_extends, "extends"};
const token_t tok_hd              = {EL_hd, "hd"};
const token_t tok_EA_propext      = {EL_EA_propext, "http://www.engarts.com/namespace/2011/ddlx propext"};
const token_t tok_includedev      = {EL_includedev, "includedev"};
const token_t tok_label           = {EL_label, "label"};
const token_t tok_language        = {EL_language, "language"};
const token_t tok_languageset     = {EL_languageset, "languageset"};
const token_t tok_maxinclusive    = {EL_maxinclusive, "maxinclusive"};
const token_t tok_mininclusive    = {EL_mininclusive, "mininclusive"};
const token_t tok_p               = {EL_p, "p"};
const token_t tok_parameter       = {EL_parameter, "parameter"};
const token_t tok_property        = {EL_property, "property"};
const token_t tok_propertypointer = {EL_propertypointer, "propertypointer"};
const token_t tok_propmap_DMX     = {EL_propmap_DMX, "propmap_DMX"};
const token_t tok_propref_DMP     = {EL_propref_DMP, "propref_DMP"};
const token_t tok_protocol        = {EL_protocol, "protocol"};
const token_t tok_refinement      = {EL_refinement, "refinement"};
const token_t tok_refines         = {EL_refines, "refines"};
const token_t tok_section         = {EL_section, "section"};
const token_t tok_setparam        = {EL_setparam, "setparam"};
const token_t tok_string          = {EL_string, "string"};
const token_t tok_useprotocol     = {EL_useprotocol, "useprotocol"};
const token_t tok_value           = {EL_value, "value"};

/*
Although we use a brain dead linear string search, we base the list 
of strings to search on context (the content model) so it is never 
long.

There is some benefit to ordering each list with most frequent 
first, but it will work anyway.
*/
const token_t *content_ROOT[] = {
	&tok_DDL,
	NULL
};
const token_t *content_DDL[] = {
	&tok_device,
	&tok_behaviorset,
	&tok_languageset,
	NULL
};
const token_t *content_languageset[] = {
	&tok_UUIDname,
	&tok_label,
	&tok_language,
	&tok_alternatefor,
	&tok_extends,
	NULL
};
const token_t *content_language[] = {
	&tok_label,
	&tok_string,
	NULL
};
const token_t *content_behaviorset[] = {
	&tok_UUIDname,
	&tok_label,
	&tok_alternatefor,
	&tok_extends,
	&tok_behaviordef,
	NULL
};
const token_t *content_behaviordef[] = {
	&tok_label,
	&tok_refines,
	&tok_section,
	NULL
};
const token_t *content_section[] = {
	&tok_p,
	&tok_section,
	&tok_hd,
	NULL
};
const token_t *content_device[] = {
	&tok_UUIDname,
	&tok_parameter,
	&tok_property,
	&tok_includedev,
	&tok_label,
	&tok_propertypointer,
	&tok_useprotocol,
	&tok_alternatefor,
	&tok_extends,
	NULL
};
const token_t *content_parameter[] = {
	&tok_label,
	&tok_choice,
	&tok_refinement,
	&tok_mininclusive,
	&tok_maxinclusive,
	NULL
};
const token_t *content_property[] = {
	&tok_behavior,
	&tok_protocol,
	&tok_value,
	&tok_property,
	&tok_includedev,
	&tok_label,
	&tok_propertypointer,
	NULL
};
const token_t *content_includedev[] = {
	&tok_setparam,
	&tok_label,
	&tok_protocol,
	NULL
};
const token_t *content_protocol[] = {
	&tok_EA_propext,
	&tok_propref_DMP,
	&tok_childrule_DMP,
	&tok_propmap_DMX,
	NULL
};

/*
This table contains the element tokens to search for given a current 
tokens. This is therefore a (loose) content model for DDL.
*/

const token_t **content[] = {
	[ELx_ROOT] = content_ROOT,
	[EL_DDL] = content_DDL,
	[EL_label] = NULL,
	[EL_alternatefor] = NULL,
	[EL_extends] = NULL,
	[EL_UUIDname] = NULL,
	[EL_languageset] = content_languageset,
	[EL_language] = content_language,
	[EL_string] = NULL,
	[EL_behaviorset] = content_behaviorset,
	[EL_behaviordef] = content_behaviordef,
	[EL_refines] = NULL,
	[EL_section] = content_section,
	[EL_hd] = NULL,
	[EL_p] = NULL,
	[EL_device] = content_device,
	[EL_parameter] = content_parameter,
	[EL_choice] = NULL,
	[EL_mininclusive] = NULL,
	[EL_maxinclusive] = NULL,
	[EL_refinement] = NULL,
	[EL_property] = content_property,
	[EL_behavior] = NULL,
	[EL_value] = NULL,
	[EL_propertypointer] = NULL,
	[EL_includedev] = content_includedev,
	[EL_setparam] = NULL,
	[EL_useprotocol] = NULL,
	[EL_protocol] = content_protocol,
	[EL_propref_DMP] = NULL,
	[EL_childrule_DMP] = NULL,
	[EL_propmap_DMX] = NULL,
	[EL_EA_propext] = NULL,
};

/*
Enumerate the attributes.

enum attr_e {
	AT_UUID,
	AT_UUID_paramname,
	AT_abs,
	AT_abs_paramname,
	AT_altlang,
	AT_array,
	AT_array_paramname,
	AT_choice_paramname,
	AT_date,
	AT_event,
	AT_event_paramname,
	AT_XML_id,
	AT_XML_space,
	AT_inc,
	AT_inc_paramname,
	AT_key,
	AT_key_paramname,
	AT_label_paramname,
	AT_lang,
	AT_loc,
	AT_loc_paramname,
	AT_maxinclusive_paramname,
	AT_mininclusive_paramname,
	AT_name,
	AT_name_paramname,
	AT_propmap_DMX_paramname,
	AT_provider,
	AT_read,
	AT_read_paramname,
	AT_ref,
	AT_ref_paramname,
	AT_refinement_paramname,
	AT_set,
	AT_set_paramname,
	AT_setparam_paramname,
	AT_sharedefine,
	AT_sharedefine_paramname,
	AT_size,
	AT_size_paramname,
	AT_type,
	AT_type_paramname,
	AT_value,
	AT_value_paramname,
	AT_valuetype,
	AT_valuetype_paramname,
	AT_varsize,
	AT_varsize_paramname,
	AT_version,
	AT_write,
	AT_write_paramname,
};
*/

typedef struct prop_s prop_t;
typedef struct uuidalias_s uuidalias_t;
typedef struct dcxt_s dcxt_t;
typedef struct behavior_s behavior_t;


/**********************************************************************/
struct uuidalias_s {
	struct uuidalias_s *next;
	uuid_t uuid;
	char *alias;
};


/**********************************************************************/
typedef unsigned int strbtst_t;
struct keyhd_s {
	const char *key;
	strbtst_t tstloc;
	struct keyhd_t *nxt[2];
};

/**********************************************************************/
/*
Property tree
*/

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

struct netprop_s {
	prophd_t hd;
//	propaccess_fn *access;
	unsigned int flags;
	unsigned int size;
	void *accessref;
};

struct impliedprop_s {
	uintptr_t val;
};

#define MAXSETPARAMLEN 62

struct param_s {
	struct param_s *nxt;
	const char *name;
	const char *subs;
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
		char *str;
		struct immobj_s obj;
		uint32_t *Aui;
		int32_t *Asi;
		double *Af;
		char **Astr;
		struct immobj_s *Aobj;
	} t;
};

struct prop_s {
	prop_t *parent;
	prop_t *siblings;
	prop_t *children;
	uint32_t childaddr;
	uint32_t array;
	uint32_t arraytotal;
	uint32_t childinc;
	/* behavior_t *behaviors; */
	char *id;
	vtype_t vtype;
	union {
		const char *subs;
		struct netprop_s net;
		struct impliedprop_s impl;
		struct immprop_s imm;
		struct device_s dev;
	} v;
};

/**********************************************************************/
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
		const char *p;
		XML_Char ch[512];
	} txt;
	struct strbuf_s *strs;
};

/**********************************************************************/
#define STRBUFSIZE 800
struct strbuf_s {
	struct strbuf_s *nxt;
	uint16_t end;
	union {
		char c[STRBUFSIZE];
		uint8_t u8[STRBUFSIZE];
	} buf;
};

/**********************************************************************/
/*
Prototypes
*/

void parsedevx(struct dcxt_s *dcxp);

/**********************************************************************/
/*
A table is a simple NULL terminated array of char pointers. Each 
points to a string whose first character is the "token" to return 
and whose subsequent characters are the corresponding string to match.

Return the token if the string is matched  or -1 on failure.
*/

static int
gettok(const char *str, const token_t **table)
{
	if (table) {
		while (*table) {
			if (strcmp(str, (*table)->str) == 0) {
				return (*table)->tok;
			}
			++table;
		}
	}
	//acnlogmark(lgDBUG, "token fail %s", str);
	return -1;
}

/**********************************************************************/
/*
tokmatch(const char *tokens, const char *str)

tokens is a string consisting of tokens to be matched with a TOKTERM 
character after each token (including the last).

str is a string to be matched against the tokens.

returns the index (0, 1, ..n) of the token matched, or -1 if not found.

e.g. tokmatch("foo|barking|bar|", "bar") returns 2
*/

#define TOKTERM '|'
#define TOKTERMr '@'

int
tokmatch(const char *tokens, const char *str)
{
	int i;
	bool bad;
	const char *cp;
	const char *tp;

	i = 0;
	cp = str;
	bad = 0;
	for (tp = tokens; *tp; ++tp) {
		if (*tp == TOKTERM) {
			if (*cp == 0) return i;
			++i;
			cp = str;
			bad = 0;
		} else if (!bad) {
			bad = (*cp++ != *tp);
		}
	}
	return -1;
}
/**********************************************************************/
struct strbuf_s *
getstrbufx(struct dcxt_s *dcxp, int strsize)
{
	struct strbuf_s *sb;

	if ((strsize) > STRBUFSIZE/2) {
		acnlogmark(lgERR, "string overflow");
		exit(EXIT_FAILURE);
	}
	sb = dcxp->strs;
	if (sb == NULL || (sb->end + strsize) >= STRBUFSIZE) {

		sb = (struct strbuf_s *)acnAlloc(sizeof(struct strbuf_s));
		if (sb == NULL) {
			acnlogmark(lgERR, "out of mem");
			exit(EXIT_FAILURE);
		}
		sb->nxt = dcxp->strs;
		dcxp->strs = sb;
		sb->end = 0;
	}
	return sb;
}

/**********************************************************************/
char *
savestr(struct dcxt_s *dcxp, const char *str)
{
	struct strbuf_s *sb;
	char *cp;
	const char *rp;

	sb = getstrbufx(dcxp, strlen(str) + 1);   /* allow for terminator */
	rp = cp = sb->buf.c + sb->end;
	while ((*cp++ = *str++)) /* nothing */;
	sb->end = cp - sb->buf.c;
	return (char *)rp;
}

/**********************************************************************/
/*
check the format of an "object" as contained in a value field
return its size
*/

int
objlen(const char *str)
{
	int i;
	
	i = 0;
	while (*str) {
		switch (*str++) {
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		case '8': case '9':
		case 'A': case 'B': case 'C': case 'D':
		case 'E': case 'F':
		case 'a': case 'b': case 'c': case 'd':
		case 'e': case 'f':
			++i;
			/* fall through */

		/* ignore comma, period or hyphen */
		case ',': case '.': case '-':
		/* ignore whitespace FIXME: should be UTF8 defined space */
		case ' ': case '\t': case '\n': case '\r':
			break;
		default:
			return -1;
		}
	}
	if ((i & 1)) return -1;
	return i/2;
}
/**********************************************************************/
int
savestrasobj(struct dcxt_s *dcxp, const char *str, uint8_t **objp)
{
	struct strbuf_s *sb;
	uint8_t *cp;
	int len;
	int byte;
	int nibble;

	len = objlen(str);
	if (len < 0) return len;
	sb = getstrbufx(dcxp, len);
	*objp = cp = sb->buf.u8 + sb->end;

	byte = 1;  /* bit 0 is shift marker */
	while ((nibble = *str++) != 0) {
		switch (nibble) {
		/* ignore comma, period or hyphen */
		case ',':
		case '.':
		case '-':
		/* ignore whitespace FIXME: should be UTF8 defined space */
		case ' ':
		case '\t':
		case '\n':
		case '\r':
			continue;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			nibble -= '0';
			break;

		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			nibble += ('a' - 'A');
			/* fall through */
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			nibble -= ('a' - 10);
			break;
		default:
			return -1;
		}
		byte = (byte << 4) | nibble;
		if (byte >= 0x100) {
			*cp++ = (uint8_t)byte;
			byte = 1;  /* restore shift marker */
		}
	}
	if (cp - *objp != len) {
		acnlogmark(lgERR, "     Object length mismatch - was %d, now %ld", len, cp - *objp);
		exit(EXIT_FAILURE);
	}
	/* assert(cp - *objp == len); */
	sb->end = cp - sb->buf.u8;
	return len;
}


/**********************************************************************/
static void
resolveuuidx(struct dcxt_s *dcxp, const char *name, uuid_t uuid)
{
	struct uuidalias_s *alp;

	if (!quickuuidOKstr(name)) {
		for (alp = dcxp->curdev->v.dev.aliases; alp != NULL; alp = alp->next) {
			if (strcmp(alp->alias, name) == 0) {
				//acnlogmark(lgDBUG, "  = %s", alp->uuidstr);
				uuidcpy(uuid, alp->uuid);
				return;
			}
		}
	} else if (str2uuid(name, uuid) >= 0) return;
	acnlogmark(lgERR, "Can't resolve UUID \"%s\"", name);
	exit(EXIT_FAILURE);
}

/**********************************************************************/
void
str2uuidx(const char *uuidstr, uuid_t uuid)
{
	if (str2uuid(uuidstr, uuid) < 0) {
		acnlogmark(lgERR, "Can't parse uuid \"%s\"", uuidstr);
		exit(EXIT_FAILURE);
	}
}

/******************************************************************************/
/*
Very similar to str2uuid but takes a length argument. Can convert in
place (since resultant is always shorter than string we can do this)
*/
#if 0  /* unused at present */

int
str2obj(uint8_t *str, uint8_t *obj, int len)
{
	int i;
	int byte;
	int nibble;

	byte = 1;  /* bit 0 is shift marker */
	i = 0;
	for (; *str; ++str) {
		switch ((nibble = *str)) {
		/* ignore comma, period or hyphen */
		case ',':
		case '.':
		case '-':
		/* ignore whitespace FIXME: should be UTF8 defined space */
		case ' ':
		case '\t':
		case '\n':
		case '\r':
			continue;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			nibble -= '0';
			break;

		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			nibble += ('a' - 'A');
			/* fall through */
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			nibble -= ('a' - 10);
			break;
		default:
			return -1;
		}
		byte = (byte << 4) | nibble;
		if (byte >= 0x100) {
			*obj++ = (uint8_t)byte;
			byte = 1;  /* restore shift marker */
			if (++i >= len) break;
		}
	}
	if (byte >= 0x10) {
		acnlogmark(lgERR, "     odd hex character in object");
	}
	return i;
}
#endif

/******************************************************************************/
/*
Condense a string to binary in-place and return the length
*/

#if 0  /* unused at present */
int
str2bin(uint8_t *str)
{
	int byte;
	int nibble;
	uint8_t *cp, *bp;

	byte = 1;  /* bit 0 is shift marker */
	bp = cp = str;
	while (1) {
		switch ((nibble = *cp++)) {
		case 0:	/* terminator */
			if (byte >= 0x10) {
				acnlogmark(lgERR, "     odd hex character in object");
			}
			return bp - str;
		/* ignore comma, period or hyphen */
		case ',':
		case '.':
		case '-':
		/* ignore whitespace FIXME: should be UTF8 defined space */
		case ' ':
		case '\t':
		case '\n':
		case '\r':
			continue;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			nibble -= '0';
			break;

		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			nibble += ('a' - 'A');
			/* fall through */
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			nibble -= ('a' - 10);
			break;
		default:
			return -1;
		}
		byte = (byte << 4) | nibble;
		if (byte >= 0x100) {
			*bp++ = (uint8_t)byte;
			byte = 1;  /* restore shift marker */
		}
	}
}
#endif

/**********************************************************************/
int
subsparam(struct dcxt_s *dcxp, const char *name, const char **subs)
{
	struct param_s *parp;

	for (parp = dcxp->curdev->v.dev.params; parp; parp = parp->nxt) {
		if (strcmp(parp->name, name) == 0) {
			*subs = parp->subs;
			return 1;
		}
	}
	return 0;
}

/**********************************************************************/
static int
getboolatt(const char *str)
{
	if (!str || strcmp(str, "false") == 0) return 0;
	if (strcmp(str, "true") == 0) return 1;
	acnlogmark(lgERR, "     bad boolean \"%s\"", str);
	return 0;
}

/**********************************************************************/
int
gooduint(const char *str, uint32_t *rslt)
{
	char *eptr;
	uint32_t ival;

	if (str) {
		ival = strtoul(str, &eptr, 10);
		if (*str != 0 && *eptr == 0) {
			*rslt = ival;
			return 0;  /* this is the normal exit for a good string */
		}
	}
	acnlogmark(lgERR,
			"     bad format, expected unsigned int, got \"%s\"", str);
	return -1;
}

/**********************************************************************/
int
goodint(const char *str, int32_t *rslt)
{
	char *eptr;
	int32_t ival;

	ival = strtol(str, &eptr, 10);
	if (*str == 0 || *eptr != 0) {
		acnlogmark(lgERR,
				"     bad format, expected int, got \"%s\"", str);
		return -1;
	}
	*rslt = ival;
	return 0;
}

/**********************************************************************/
void
elem_text(void *data, const char *txt, int len)
{
	struct dcxt_s *dcxp = (struct dcxt_s *)data;
	int space;
	
	space = sizeof(dcxp->txt.ch) - dcxp->txtlen - 1;
	if (len > space) {
		len = space;
		acnlogmark(lgERR, "%4d value text too long - truncating", dcxp->elcount);
	}
	memcpy(dcxp->txt.ch + dcxp->txtlen, txt, len);
	dcxp->txtlen += len;
}

/**********************************************************************/
void
startText(struct dcxt_s *dcxp, const char *paramname)
{
	if (paramname && subsparam(dcxp, paramname, &dcxp->txt.p)) {
		dcxp->txtlen = -1;
	} else {
		dcxp->txtlen = 0;
		dcxp->txt.ch[0] = 0;
		XML_SetCharacterDataHandler(dcxp->parser, &elem_text);
	}
}

/**********************************************************************/
const char *
endText(struct dcxt_s *dcxp)
{
	if (dcxp->txtlen == -1) return dcxp->txt.p;

	XML_SetCharacterDataHandler(dcxp->parser, NULL);
	dcxp->txt.ch[dcxp->txtlen] = 0;  /* terminate */
	return dcxp->txt.ch;
}

/**********************************************************************/
struct prop_s *
newprop(struct dcxt_s *dcxp)
{
	struct prop_s *pp;
	struct prop_s *pdad;

	/* allocate a new property */
	pp = acnNew(struct prop_s);
	if (pp == NULL) {
		acnlogerror(lgERR);
		exit(EXIT_FAILURE);
	}
	pdad = dcxp->curprop;
	pp->parent = pdad;
	pp->siblings = pdad->children;
	pdad->children = pp;

	pp->childaddr = pdad->childaddr;  /* default - may change */
	dcxp->curprop = pp;
	return pp;
}

/**********************************************************************/
static struct prop_s *
reverseproplist(struct prop_s *plist) {
	struct prop_s *tp;
	struct prop_s *rlist = NULL;

	while ((tp = plist) != NULL) {
		plist = tp->siblings;
		tp->siblings = rlist;
		rlist = tp;
	}
	return rlist;
}

/**********************************************************************/
const char device_atts[] =
	/* 0 */   "UUID@"
	/* 1 */   "provider@"
	/* 2 */   "date@"
	/* 3 */   "http://www.w3.org/XML/1998/namespace id|"
;

void
dev_start(struct dcxt_s *dcxp, const char **atta)
{
	struct prop_s *pp;
	uuid_t dcid;
	const char *uuidp;
	char uuidstr[UUID_STR_SIZE];

	uuidp = atta[0];

	pp = dcxp->curprop;
	assert(pp->vtype == VT_include);
	
	if (uuidp) {
		str2uuid(uuidp, dcid);
		if (uuidsEq(dcid, pp->v.dev.uuid)) return;  /* good return */
	}
	acnlogmark(lgERR, "UUID mismatch:\n"
						 "  expected %s\n"
						 "       got %s",
						 uuid2str(pp->v.dev.uuid, uuidstr),
						 uuidp);
	exit(EXIT_FAILURE);
}

/**********************************************************************/
void
behavior_start(struct dcxt_s *dcxp, const char **atta)
{
	struct prop_s *pp;
	
	if ((pp = dcxp->curprop) == NULL) {
		acnlogmark(lgERR, "Behavior with no property");
		exit(EXIT_FAILURE);
	}
}

/**********************************************************************/
const char UUIDname_atts[] =
	/* 0 */   "name@"
	/* 1 */   "UUID@"
	/* 2 */   "http://www.w3.org/XML/1998/namespace id|"
;

void
alias_start(struct dcxt_s *dcxp, const char **atta)
{
	struct uuidalias_s *alp;
	const char *aliasname;
	const char *aliasuuid;

	/* handle attributes */
	aliasname = atta[0];
	aliasuuid = atta[1];

	if (aliasname == NULL || aliasuuid == NULL) {
		acnlogmark(lgERR, "%4d UUIDname missing attribute(s)", dcxp->elcount);
		return;
	}
	if (!quickuuidOKstr(aliasuuid)) {
		acnlogmark(lgERR, "%4d UUIDname bad format: %s", dcxp->elcount, aliasuuid);
		return;
	}
	alp = acnNew(struct uuidalias_s);
	if (alp == NULL) {
		acnlogerror(lgERR);
		exit(EXIT_FAILURE);
	}
	alp->alias = savestr(dcxp, aliasname);
	str2uuidx(aliasuuid, alp->uuid);
	alp->next = dcxp->curprop->v.dev.aliases;
	dcxp->curprop->v.dev.aliases = alp;
	acnlogmark(lgDBUG, "%4d added alias %s", dcxp->elcount, aliasname);
}

/**********************************************************************/
const char property_atts[] =
	/* WARNING: order must match case labels below */
	/* 0 */   "valuetype@"
	/* 1 */   "array|"
	/* 2 */   "http://www.w3.org/XML/1998/namespace id|"
	/* 3 */   "sharedefine|"
;

const char valuetypes[] = "NULL|immediate|implied|network|";

void
prop_start(struct dcxt_s *dcxp, const char **atta)
{
	struct prop_s *pp;
	vtype_t vtype;
	uint32_t arraysize;

	const XML_Char *vtypep = atta[0];
	const XML_Char *arrayp = atta[1];
	const XML_Char *propID = atta[2];

	if (vtypep == NULL
				|| (vtype = tokmatch(valuetypes, vtypep)) == -1)
	{
		acnlogmark(lgERR, "%4d bad or missing valuetype", dcxp->elcount);
		dcxp->skip = dcxp->nestlvl;
		return;
	}

	pp = newprop(dcxp);  /* newprop links in to dcxp->curprop */
	pp->vtype = vtype;
	if (propID) {
		pp->id = savestr(dcxp, propID);
	}

	if (arrayp && gooduint(arrayp, &arraysize) == 0 && arraysize > 1) {
		pp->array = arraysize;
		if (!pp->arraytotal) pp->arraytotal = 1;
		pp->arraytotal *= arraysize;
	}
}

/**********************************************************************/
void
prop_end(struct dcxt_s *dcxp)
{
	struct prop_s *pp;

	pp = dcxp->curprop;
	if (pp->vtype == VT_network) {
		if (pp->v.net.flags & PROP_VALID) {
			acnlogmark(lgDBUG, "%4d property address %u", dcxp->elcount, pp->v.net.hd.addr);
			dcxp->nprops++;
		} else {
			acnlogmark(lgERR, "%4d property not valid", dcxp->elcount);
		}
	}
	if (pp->array && pp->children && !pp->childinc) {
		acnlogmark(lgNTCE,
					"%4d array with no child increment",
					dcxp->elcount);
	}
	/* reverse the order of any children to match documnt order */
	pp->children = reverseproplist(pp->children);
	dcxp->curprop = pp->parent;
}

/**********************************************************************/
/*
Includedev - can't call the included device until we reach the end 
tag because we need to accumulate content first, so we allocate a 
dummy property for now.
*/
const char includedev_atts[] =
	/* 0 */ "UUID@"
	/* 1 */ "array|"
	/* 2 */ "http://www.w3.org/XML/1998/namespace id|"
;

void
incdev_start(struct dcxt_s *dcxp, const char **atta)
{
	struct prop_s *pp;
	struct prop_s *prevp;
	uint32_t arraysize;

	const XML_Char *uuidp = atta[0];
	const XML_Char *arrayp = atta[1];
	const XML_Char *propID = atta[2];

	acnlogmark(lgDBUG, "%4d include %s", dcxp->elcount, uuidp);

	pp = newprop(dcxp);
	pp->vtype = VT_include;
	if (propID) {
		pp->id = savestr(dcxp, propID);
	}
	resolveuuidx(dcxp, uuidp, pp->v.dev.uuid);
	if (arrayp && gooduint(arrayp, &arraysize) == 0 && arraysize > 1) {
		pp->array = arraysize;
		if (!pp->arraytotal) pp->arraytotal = 1;
		pp->arraytotal *= arraysize;
	}

	/* link to inherited params before adding new ones */
	for (prevp = pp->parent; prevp; prevp = prevp->parent) {
		if (prevp->vtype == VT_include) {
			pp->v.dev.params = prevp->v.dev.params;
			break;
		}
	}
	/* add to devices to be processed */
	dcxp->lastdev = dcxp->lastdev->v.dev.nxtdev = pp;
}

/**********************************************************************/
void
incdev_end(struct dcxt_s *dcxp)
{
	prop_t *pp = dcxp->curprop;

	if (pp->array && !pp->childinc) {
		acnlogmark(lgERR,
					"%4d include array with no child increment",
					dcxp->elcount);
	}
	dcxp->curprop = pp->parent;
}

/**********************************************************************/
const char protocol_atts[] =
	/* note: this is a single string */
	/* WARNING: order determines values used in switches below */
	/* 0 */  "name@"
	/* 1 */  "http://www.w3.org/XML/1998/namespace id|"
;

void
protocol_start(struct dcxt_s *dcxp, const char **atta)
{
	if (!atta[0] || strcasecmp(atta[0], "ESTA.DMP") != 0) {
		dcxp->skip = dcxp->nestlvl;
		acnlogmark(lgINFO, "%4d skipping protocol \"%s\"...", dcxp->elcount, atta[0]);
	}
}

/**********************************************************************/
const char value_atts[] = 
	/* note: this is a single string */
	/* WARNING: order determines values used in switches below */
	/*  0 */ "type@"
	/*  1 */ "http://www.w3.org/XML/1998/namespace id|"
	/*  2 */ "value.paramname|"
;

/* WARNING: order follows immtype_e enumeration */
const char value_types[] = "uint|sint|float|string|object|";

const int vsizes[] = {
	sizeof(uint32_t),
	sizeof(int32_t),
	sizeof(double),
	sizeof(char *),
	sizeof(struct immobj_s),
};

void
value_start(struct dcxt_s *dcxp, const char **atta)
{
	int i;
	const char *ptype = atta[0];
	uint8_t *arrayp;

	if (dcxp->curprop->vtype < VT_imm_uint
			&& dcxp->curprop->vtype != VT_imm_unknown)
	{
		acnlogmark(lgERR, "%4d <value> in non-immediate property",
					dcxp->elcount);
skipvalue:
		dcxp->skip = dcxp->nestlvl;
		return;
	}
	if ((i = tokmatch(value_types, ptype)) < 0) {
		acnlogmark(lgERR, "%4d value: missing or bad type \"%s\"",
					dcxp->elcount, ptype);
		goto skipvalue;
	}

	if (dcxp->curprop->v.imm.count == 0) {
		dcxp->curprop->vtype = i + VT_imm_uint;
	} else {
		if (dcxp->curprop->arraytotal == 0) {
			acnlogmark(lgERR, "%4d multiple values in single property",
						dcxp->elcount);
			goto skipvalue;
		}
		if ((i + VT_imm_uint) != dcxp->curprop->vtype) {
			acnlogmark(lgWARN, "%4d value type mismatch \"%s\"",
					dcxp->elcount, ptype);
			goto skipvalue;
		}
		if (dcxp->curprop->v.imm.count == 1) {
			/* need to assign an array for values */
			arrayp = acnAlloc(vsizes[i] * dcxp->curprop->arraytotal);
			switch (i) {
			case 0:   /* uint */
				*(uint32_t *)arrayp = dcxp->curprop->v.imm.t.ui;
				dcxp->curprop->v.imm.t.Aui = (uint32_t *)arrayp;
				break;
			case 1:   /* sint */
				*(int32_t *)arrayp = dcxp->curprop->v.imm.t.si;
				dcxp->curprop->v.imm.t.Asi = (int32_t *)arrayp;
				break;
			case 2:   /* float */
				*(double *)arrayp = dcxp->curprop->v.imm.t.f;
				dcxp->curprop->v.imm.t.Af = (double *)arrayp;
				break;
			case 3:   /* string */
				*(char **)arrayp = dcxp->curprop->v.imm.t.str;
				dcxp->curprop->v.imm.t.Astr = (char **)arrayp;
				break;
			case 4:   /* object */
				*(struct immobj_s *)arrayp = dcxp->curprop->v.imm.t.obj;
				dcxp->curprop->v.imm.t.Aobj = (struct immobj_s *)arrayp;
				break;
			default:
				break;
			}
		}
	}
	startText(dcxp, atta[2]);
}

/**********************************************************************/
/*
FIXME: Array properties may have arrays of values - currently only 
one supported.
*/

void
value_end(struct dcxt_s *dcxp)
{
	struct prop_s *pp;
	const char *vtext;

	pp = dcxp->curprop;
	vtext = endText(dcxp);
	acnlogmark(lgDBUG, "%4d got value type %d \"%s\"",
					dcxp->elcount, pp->vtype - VT_imm_uint, vtext);

	switch (pp->vtype) {
	case VT_imm_object:
		pp->v.imm.t.obj.size = savestrasobj(dcxp, vtext, &pp->v.imm.t.obj.data);	
		break;
	case VT_imm_string:
		pp->v.imm.t.str = savestr(dcxp, vtext);
		break;
	default: {
		char *ep;
		
		switch (pp->vtype) {
		case VT_imm_float:
			pp->v.imm.t.f = strtod(vtext, &ep);
			break;
		case VT_imm_uint:
			pp->v.imm.t.ui = strtoul(vtext, &ep, 10);
			break;
		case VT_imm_sint:
		default:
			pp->v.imm.t.si = strtol(vtext, &ep, 10);
			break;
		}
		if (*vtext == 0 || ep == vtext || *ep != 0) {
			acnlogmark(lgERR, "%4d uint parse error \"%s\"",
							dcxp->elcount, vtext);
		}
	}	break;
	}
}

/**********************************************************************/
const char propref_DMP_atts[] = 
	/* WARNING: order determines values used in switches below */
	/*  0 */ "loc@"
	/*  1 */ "size@"
	/*  2 */ "read|"
	/*  3 */ "write|"
	/*  4 */ "event|"
	/*  5 */ "varsize|"
	/*  6 */ "inc|"
	/*  7 */ "abs|"
	/*  8 */ "http://www.w3.org/XML/1998/namespace id|"
;

void
propref_start(struct dcxt_s *dcxp, const char **atta)
{
	struct prop_s *pp;
	unsigned int flags;

	pp = dcxp->curprop;
	flags = PROP_VALID;   /* assume valid for now */
	flags |= (getboolatt(atta[2]) << bPROP_READ);
	flags |= (getboolatt(atta[3]) << bPROP_WRITE);
	flags |= (getboolatt(atta[4]) << bPROP_EVENT);
	flags |= (getboolatt(atta[5]) << bPROP_VSIZE);
	flags |= (getboolatt(atta[7]) << bPROP_ABS);

	/* "loc" is mandatory */
	if (gooduint(atta[0], &pp->v.net.hd.addr) == 0) {
		if ((flags & PROP_ABS) == 0 && pp->parent)
			pp->v.net.hd.addr += pp->parent->childaddr;
	} else {
		flags = 0;
		acnlogmark(lgERR, "%4d bad or missing net location",
						dcxp->elcount);
	}
	/* "size" is mandatory */
	if (gooduint(atta[1], &pp->v.net.size) < 0) {
		flags = 0;
		acnlogmark(lgERR, "%4d bad or missing size", dcxp->elcount);
	}
	/* "inc" */
	if (atta[6]) gooduint(atta[6], &pp->v.net.hd.inc);
	pp->v.net.flags = flags;
}

/**********************************************************************/
const char childrule_DMP_atts[] =
	/* WARNING: order determines values used in switches below */
	/*  0 */ "loc|"
	/*  1 */ "inc|"
	/*  2 */ "abs|"
	/*  3 */ "http://www.w3.org/XML/1998/namespace id|"
;

void
childrule_start(struct dcxt_s *dcxp, const char **atta)
{
	struct prop_s *pp;
	uint32_t addoff;

	pp = dcxp->curprop;
	addoff = pp->childaddr;
	if (!getboolatt(atta[2])) addoff = 0;

	if (gooduint(atta[0], &pp->childaddr) == 0) {
		pp->childaddr += addoff;
	}
	if (atta[1]) gooduint(atta[1], &pp->childinc);
}

/**********************************************************************/
const char setparam_atts[] =
	/* WARNING: order determines values used in switches below */
	/*  0 */ "name@"
	/*  1 */ "setparam.paramname|"
	/*  2 */ "http://www.w3.org/XML/1998/namespace id|"
;

void
setparam_start(struct dcxt_s *dcxp, const char **atta)
{
	const char *name = atta[0];
	struct param_s *parp;

	if (!name) {
		acnlogmark(lgERR, "%4d setparam with no name",
							dcxp->elcount);
		return;
	}
	parp = acnNew(struct param_s);
	parp->name = savestr(dcxp, name);
	parp->nxt = dcxp->curprop->v.dev.params;
	dcxp->curprop->v.dev.params = parp;
	startText(dcxp, atta[1]);
}

/**********************************************************************/
void
setparam_end(struct dcxt_s *dcxp)
{
	struct param_s *parp;

	parp = dcxp->curprop->v.dev.params;
	parp->subs = endText(dcxp);
	if (dcxp->txtlen >= 0) parp->subs = savestr(dcxp, parp->subs);
	acnlogmark(lgDBUG, "%4d add param %s=\"%s\"",
							dcxp->elcount, parp->name, parp->subs);
}

/**********************************************************************/
typedef void elemstart_fn(struct dcxt_s *dcxp, const char **atta);
typedef void elemend_fn(struct dcxt_s *dcxp);

elemstart_fn *startvec[EL_MAX] = {
	[EL_device]	= &dev_start,
	[EL_property] = &prop_start,
	[EL_value] = &value_start,
	[EL_protocol] = &protocol_start,
	[EL_includedev] = &incdev_start,
	[EL_UUIDname] = &alias_start,
	[EL_propref_DMP] = &propref_start,
	[EL_childrule_DMP] = &childrule_start,
	[EL_setparam] = &setparam_start,
};

elemend_fn *endvec[EL_MAX] = {
	/* [EL_device]	= &dev_end, */
	[EL_property] = &prop_end,
	[EL_value] = &value_end,
	[EL_includedev] = &incdev_end,
	[EL_setparam] = &setparam_end,
};

const char * const elematts[EL_MAX] = {
	/* [EL_DDL] = DDL_atts, */
	[EL_UUIDname] = UUIDname_atts,
	/*
	[EL_alternatefor] = alternatefor_atts,
	[EL_behavior] = behavior_atts,
	[EL_behaviordef] = behaviordef_atts,
	[EL_behaviorset] = behaviorset_atts,
	*/
	[EL_childrule_DMP] = childrule_DMP_atts,
	/* [EL_choice] = choice_atts, */
	[EL_device] = device_atts,
	/*
	[EL_extends] = extends_atts,
	[EL_hd] = hd_atts,
	[EL_EA_propext] = EA_propext_atts,
	*/
	[EL_includedev] = includedev_atts,
	/*
	[EL_label] = label_atts,
	[EL_language] = language_atts,
	[EL_languageset] = languageset_atts,
	[EL_maxinclusive] = maxinclusive_atts,
	[EL_mininclusive] = mininclusive_atts,
	[EL_p] = p_atts,
	[EL_parameter] = parameter_atts,
	*/
	[EL_property] = property_atts,
	/*
	[EL_propertypointer] = propertypointer_atts,
	[EL_propmap_DMX] = propmap_DMX_atts,
	*/
	[EL_propref_DMP] = propref_DMP_atts,
	[EL_protocol] = protocol_atts,
	/*
	[EL_refinement] = refinement_atts,
	[EL_refines] = refines_atts,
	[EL_section] = section_atts,
	*/
	[EL_setparam] = setparam_atts,
	/*
	[EL_string] = string_atts,
	[EL_useprotocol] = useprotocol_atts,
	*/
	[EL_value] = value_atts,
};

void
el_start(void *data, const char *el, const char **atts)
{
	struct dcxt_s *dcxp = (struct dcxt_s *)data;
	int eltok;
	const char *cxtatts;
	const char *atta[MAXATTS];
	int i;
	bool bad;
	const char *cp;
	const char *tp;

	++dcxp->elcount;
	if (dcxp->skip) {
		++dcxp->nestlvl;
		return;
	}

	if (dcxp->nestlvl >= MAX_NEST) {
		acnlogmark(lgERR, "E%4d Maximum XML nesting reached. skipping...", dcxp->elcount);
		dcxp->skip = dcxp->nestlvl++;
		return;
	}

	//acnlogmark(lgDBUG, "<%s %d>", el, dcxp->nestlvl);

	if ((eltok = gettok(el, content[dcxp->elestack[dcxp->nestlvl++]])) < 0) {
		acnlogmark(lgERR, "%4d Unexpected element \"%s\". skipping...", dcxp->elcount, el);
		dcxp->skip = dcxp->nestlvl;
	} else {
		dcxp->elestack[dcxp->nestlvl] = eltok;
		cxtatts = elematts[eltok];
		memset(atta, 0, sizeof(atta));

		if (cxtatts) {
			for (; *atts != NULL; atts += 2) {

				i = 0;
				cp = atts[0];
				bad = 0;
				for (tp = cxtatts; ; ++tp) {
					if (*tp == 0) {
						acnlogmark(lgWARN, "%4d unknown attribute %s=\"%s\"",
										dcxp->elcount, atts[0], atts[1]);
						break;
					}
					if (*tp != TOKTERM && *tp != TOKTERMr) {
						if (!bad) {
							if (*cp == *tp) {
								++cp;
							} else
								bad = 1;
						}
					} else {
						if (!bad) {  /* matched thus far */
							if (*cp == 0) {
								if (atta[i] == NULL) atta[i] = atts[1];
								break;
							}
							if (strcmp(cp, ".paramname") == 0) {
								subsparam(dcxp, atts[1], atta + i);
								break;
							}
						}
						++i;
						cp = atts[0];	/* reset to start of attr name */
						bad = 0;
					}
				}
			}
			i = 0;
			cp = tp = cxtatts;
			do {
				switch (*tp) {
				case TOKTERMr:
					if (atta[i] == NULL) {
						acnlogmark(lgERR, "%4d <%s> required attribute %.*s missing",
										dcxp->elcount, el, (int)(tp - cp), cp);
					}
					/* fall through */
				case TOKTERM:
					++i;
					cp = tp + 1;
					/* fall through */
				default:
					++tp;
					break;
				}
			} while (*tp);
		}

		if (startvec[eltok]) {
			//acnlogmark(lgDBUG, "Start %s", el);
			(*startvec[eltok])(dcxp, atta);
			//acnlogmark(lgDBUG, "Started %s", el);
		}
	}
	return;
}

/**********************************************************************/
void
el_end(void *data, const char *el)
{
	struct dcxt_s *dcxp = (struct dcxt_s *)data;
	int eltok;

	//acnlogmark(lgDBUG, "</%s %d>", el, dcxp->nestlvl);
	if (!dcxp->skip) {
		eltok = dcxp->elestack[dcxp->nestlvl];

		if (endvec[eltok]) {
			//acnlogmark(lgDBUG, "End  %s", el);
			(*endvec[eltok])(dcxp);
		}
	}
	if (dcxp->nestlvl > 0 && dcxp->nestlvl == dcxp->skip) {
		dcxp->skip = 0;
		acnlogmark(lgDBUG, "%4d ...done", dcxp->elcount);
	}
	--dcxp->nestlvl;
}

/**********************************************************************/
int openddlx(uint8_t *uuidp)
{
	char fname[sizeof(ddl_location) + UUID_STR_SIZE];
	const char *sp;
	char *dp;
	char c;
	int fd;

	sp = ddl_location;
	dp = fname;
	
	while ((c = *sp++) != '%') *dp++ = c;
	uuid2str(uuidp, dp);
	dp += UUID_STR_SIZE - 1;
	while ((*dp++ = *sp++));
	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		acnlogerror(lgERR);
		exit(EXIT_FAILURE);
	}
	return fd;
}

/**********************************************************************/
void
parsedevx(struct dcxt_s *dcxp)
{
	int fd;
	int sz;
	void *buf;
	char uuidstr[UUID_STR_SIZE];

	acnlogmark(lgDBUG, "     parse   %s", uuid2str(dcxp->curdev->v.dev.uuid, uuidstr));

	dcxp->elcount = 0;
	dcxp->curprop = dcxp->curdev;
	fd = openddlx(dcxp->curdev->v.dev.uuid);

	dcxp->parser = XML_ParserCreateNS(NULL, ' ');
	XML_SetElementHandler(dcxp->parser, &el_start, &el_end);
	XML_SetUserData(dcxp->parser, dcxp);

	do {
		
		if ((buf = XML_GetBuffer(dcxp->parser, BUFF_SIZE)) == NULL) {
			acnlogmark(lgERR, "Can't allocate buffer %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		if ((sz = read(fd, buf, BUFF_SIZE)) < 0) {
			acnlogerror(lgERR);
			exit(EXIT_FAILURE);
		}
		if (! XML_ParseBuffer(dcxp->parser, sz, sz == 0)) {
			acnlogmark(lgERR, "Parse error");
			exit(EXIT_FAILURE);
		}

	} while (sz > 0);

	XML_ParserFree(dcxp->parser);
	close(fd);	/* leave files as we found them */

	/* reverse the order of top level children to match document order */
	dcxp->curdev->children = reverseproplist(dcxp->curdev->children);

	acnlogmark(lgDBUG, "     end     %s", uuid2str(dcxp->curdev->v.dev.uuid, uuidstr));

	return;
}

/**********************************************************************/
const char *ptypes[] = {
	[VT_NULL] = "NULL",
	[VT_imm_unknown] = "immediate (unknown)",
	[VT_implied] = "implied",
	[VT_network] = "network",
	[VT_include] = "include",
	[VT_imm_uint] = "immediate (uint)",
	[VT_imm_sint] = "immediate (sint)",
	[VT_imm_float] = "immediate (float)",
	[VT_imm_string] = "immediate (string)",
	[VT_imm_object] = "immediate (object)",
};

void
showtree(struct prop_s *prop, int lvl)
{
	struct prop_s *pp;

	{
		int i;
		for (i = lvl; i--;) fputs("   ", stdout);
	}
	/*
	if (lvl) printf("%-*d", lvl * 3, lvl);
	*/
	printf("%s property", ptypes[prop->vtype]);
	if (prop->id) printf(" ID: %s", prop->id);
	switch (prop->vtype) {
	case VT_imm_object:
		{
			int i;
			printf(" =");
			for (i = 0; i < prop->v.imm.t.obj.size; ++i) {
				printf(" %02x", prop->v.imm.t.obj.data[i]);
			}
		}
		/* fall through */
	case VT_NULL:
	case VT_implied:
	case VT_include:
		fputc('\n', stdout);
		break;
	case VT_imm_unknown:
		printf(" value: Unknown\n");
		break;
	case VT_network:
		printf(" loc: %u, size %u\n", prop->v.net.hd.addr, prop->v.net.size);
		break;
	case VT_imm_uint:
		printf(" = %u\n", prop->v.imm.t.ui);
		break;
	case VT_imm_sint:
		printf(" = %d\n", prop->v.imm.t.si);
		break;
	case VT_imm_float:
		printf(" = %g\n", prop->v.imm.t.f);
		break;
	case VT_imm_string:
		printf(" = \"%s\"\n", prop->v.imm.t.str);
		break;
	default:
		printf("unknown type!\n");
		break;
	}
	
	for (pp = prop->children; pp != NULL; pp = pp->siblings)
		showtree(pp, lvl + 1);
}

/**********************************************************************/
void clearprop(struct prop_s *prop);

/**********************************************************************/
void
freeprop(struct prop_s *prop)
{
	if (prop->children) clearprop(prop);

	/*
	while (prop->behaviors != NULL) {
		behavior_t *bp = prop->behaviors;
		prop->behaviors = bp->nxt;
		acnFree(bp, behavior_t);
	}
	*/

	acnFree(prop, struct prop_s);
}

/**********************************************************************/
void
clearprop(struct prop_s *prop)
{
	struct prop_s *pp;
	
	while ((pp = prop->children) != NULL) {
		prop->children = pp->siblings;
		pp->siblings = NULL;
		freeprop(pp);
	}
}

/**********************************************************************/
void
freedev(struct dcxt_s *dcxp)
{
	clearprop(&dcxp->rootprop);
}

/**********************************************************************/

int
main(int argc, char *argv[])
{
	struct dcxt_s dcxt;

	memset(&dcxt, 0, sizeof(dcxt));
	dcxt.curdev = dcxt.lastdev = &dcxt.rootprop;
	dcxt.rootprop.vtype = VT_include;
	/* dcxt.elestack[0] = ELx_ROOT; // ELx_ROOT is zero so no need to initialize */

	switch (argc) {
	case 2:
		str2uuid(argv[1], dcxt.rootprop.v.dev.uuid);
		break;
	case 0:
	default:
		acnlogmark(lgERR, "Usage: %s <root-DCID>", argv[0]);
		return EXIT_FAILURE;
	}

	for ( ; dcxt.curdev != NULL; dcxt.curdev = dcxt.curdev->v.dev.nxtdev) {
		parsedevx(&dcxt);
	}

	acnlogmark(lgDBUG, "Found %d net properties", dcxt.nprops);
	showtree(&dcxt.rootprop, 0);
	freedev(&dcxt);

	return 0;
}
