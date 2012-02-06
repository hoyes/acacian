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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include "acncommon.h"
#include "acnlog.h"
#include "acnmem.h"
#include "uuid.h"
#include "ddl/parse.h"
#include "propmap.h"
#include "ddl/behaviors.h"
#include "ddl/resolve.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL
/**********************************************************************/
#define BUFF_SIZE 2048
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

const ddlchar_t *elenames[EL_MAX] = {
	[ELx_terminator] = NULL,
	[ELx_ROOT] = NULL,
	[EL_DDL] = "DDL",
	[EL_UUIDname] = "UUIDname",
	[EL_alternatefor] = "alternatefor",
	[EL_behavior] = "behavior",
	[EL_behaviordef] = "behaviordef",
	[EL_behaviorset] = "behaviorset",
	[EL_childrule_DMP] = "childrule_DMP",
	[EL_choice] = "choice",
	[EL_device] = "device",
	[EL_extends] = "extends",
	[EL_hd] = "hd",
	[EL_EA_propext] = "http://www.engarts.com/namespace/2011/ddlx propext",
	[EL_includedev] = "includedev",
	[EL_label] = "label",
	[EL_language] = "language",
	[EL_languageset] = "languageset",
	[EL_maxinclusive] = "maxinclusive",
	[EL_mininclusive] = "mininclusive",
	[EL_p] = "p",
	[EL_parameter] = "parameter",
	[EL_property] = "property",
	[EL_propertypointer] = "propertypointer",
	[EL_propmap_DMX] = "propmap_DMX",
	[EL_propref_DMP] = "propref_DMP",
	[EL_protocol] = "protocol",
	[EL_refinement] = "refinement",
	[EL_refines] = "refines",
	[EL_section] = "section",
	[EL_setparam] = "setparam",
	[EL_string] = "string",
	[EL_useprotocol] = "useprotocol",
	[EL_value] = "value"
};

/*
Although we use a brain dead linear string search, we base the list 
of strings to search on context (the content model) so it is never 
long. Note this is not a strict syntax check as it does not 
generally check the order of elements, the presence of required 
content or the presence of multiple elements. Handling routines for 
specific elements may however, do some of this.

Content are entered in their permitted order, but this is not 
currently checked.
*/
const elemtok_t content_ROOT[] = {
	EL_DDL,
	ELx_terminator
};
const elemtok_t content_DDL[] = {
	EL_device,
	EL_behaviorset,
	EL_languageset,
	ELx_terminator
};
const elemtok_t content_languageset[] = {
	EL_UUIDname,
	EL_label,
	EL_alternatefor,
	EL_extends,
	EL_language,
	ELx_terminator
};
const elemtok_t content_language[] = {
	EL_label,
	EL_string,
	ELx_terminator
};
const elemtok_t content_behaviorset[] = {
	EL_UUIDname,
	EL_label,
	EL_alternatefor,
	EL_extends,
	EL_behaviordef,
	ELx_terminator
};
const elemtok_t content_behaviordef[] = {
	EL_label,
	EL_refines,
	EL_section,
	ELx_terminator
};
const elemtok_t content_section[] = {
	EL_hd,
	EL_p,
	EL_section,
	ELx_terminator
};
const elemtok_t content_device[] = {
	EL_UUIDname,
	EL_parameter,
	EL_label,
	EL_alternatefor,
	EL_extends,
	EL_useprotocol,
	EL_property,
	EL_propertypointer,
	EL_includedev,
	ELx_terminator
};
const elemtok_t content_deviceprops[] = {
	EL_property,
	EL_propertypointer,
	EL_includedev,
	ELx_terminator
};
const elemtok_t content_parameter[] = {
	EL_label,
	EL_choice,
	EL_refinement,
	EL_mininclusive,
	EL_maxinclusive,
	ELx_terminator
};
const elemtok_t content_property[] = {
	EL_label,
	EL_behavior,
	EL_protocol,
	EL_value,
	EL_property,
	EL_includedev,
	EL_propertypointer,
	ELx_terminator
};
const elemtok_t content_includedev[] = {
	EL_label,
	EL_protocol,
	EL_setparam,
	ELx_terminator
};
const elemtok_t content_protocol[] = {
	EL_propref_DMP,
	EL_childrule_DMP,
	EL_propmap_DMX,
	EL_EA_propext,
	ELx_terminator
};
const elemtok_t content_none[] = {
	ELx_terminator
};

/*
This table contains the element tokens to search for given a current 
tokens. This is therefore a (loose) content model for DDL.
*/
const elemtok_t *content[] = {
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

/* must match enum ddlmod_e */
const ddlchar_t *modnames[] = {
	[mod_dev] = "device",
	[mod_lset] = "languageset",
	[mod_bset] = "behaviorset",
};

const ddlchar_t *ptypes[] = {
	[VT_NULL] = "NULL",
	[VT_imm_unknown] = "immediate (unknown)",
	[VT_implied] = "implied",
	[VT_network] = "network",
	[VT_include] = "include",
	[VT_device] = "(sub)device",
	[VT_imm_uint] = "immediate (uint)",
	[VT_imm_sint] = "immediate (sint)",
	[VT_imm_float] = "immediate (float)",
	[VT_imm_string] = "immediate (string)",
	[VT_imm_object] = "immediate (object)",
};

const ddlchar_t *etypes[] = {
    [etype_none]      = "unknown",
    [etype_boolean]   = "boolean",
    [etype_sint]      = "sint",
    [etype_uint]      = "uint",
    [etype_float]     = "float",
    [etype_UTF8]      = "UTF8",
    [etype_UTF16]     = "UTF16",
    [etype_UTF32]     = "UTF32",
    [etype_string]    = "string",
    [etype_enum]      = "enum",
    [etype_opaque]    = "opaque",
    [etype_bitmap]    = "bitmap",
};

/**********************************************************************/
/*
Prototypes
*/

/* none */

/**********************************************************************/
/*
A table is an array of permitted tokens terminated by 
ELx_terminator. The token is used as an index into the array of 
corresponding names elenames.

Return the token if the string is matched  or -1 on failure.
*/

static int
gettok(const ddlchar_t *str, elemtok_t curelem)
{
	elemtok_t tok;
	const elemtok_t *allowed;

	if ((allowed = content[curelem]) != NULL) {
		while ((tok = *allowed++) != ELx_terminator) {
			const ddlchar_t *toktext = elenames[tok];

			if (toktext && strcmp(str, toktext) == 0) return tok;
		}
	}
	return -1;
}

/**********************************************************************/
/*
tokmatch(const ddlchar_t *tokens, const ddlchar_t *str)

tokens is a string consisting of tokens to be matched with a TOKTERM 
character after each token (including the last).

str is a string to be matched against the tokens.

returns the index (0, 1, ..n) of the token matched, or -1 if not found.

TOKTERM marks the end of an optional attribute
TOKTERMr marks the end of a required attribute

e.g. tokmatch("foo|barking|bar|", "bar") returns 2
*/

#define TOKTERM '|'
#define TOKTERMr '@'

int
tokmatch(const ddlchar_t *tokens, const ddlchar_t *str)
{
	int i;
	bool bad;
	const ddlchar_t *cp;
	const ddlchar_t *tp;

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
unsigned int
savestr(const ddlchar_t *str, const ddlchar_t **copy)
{
	ddlchar_t *cp;
	unsigned int len;
	
	len = strlen(str);
	*copy = cp = mallocx(len + 1);   /* allow for terminator */
	while ((*cp++ = *str++)) /* nothing */;
	return len;
}
/**********************************************************************/
/*
check the format of an "object" as contained in a value field
return its size
*/

int
objlen(const ddlchar_t *str)
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
savestrasobj(const ddlchar_t *str, uint8_t **objp)
{
	uint8_t *cp;
	int len;
	int byte;
	int nibble;

	if ((len = objlen(str)) < 0) return len;
	*objp = cp = mallocx(len);

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
	assert(cp - *objp == len);
	return len;
}

/**********************************************************************/
static struct prop_s *
itsdevice(struct prop_s *prop)
{
	while (prop->vtype != VT_device) {
		if ((prop = prop->parent) == NULL) break;
	}
	return prop;
}

/**********************************************************************/
static void
resolveuuidx(struct dcxt_s *dcxp, const ddlchar_t *name, uuid_t uuid)
{
	struct uuidalias_s *alp;

	if (!quickuuidOKstr(name)) {
		for (alp = itsdevice(dcxp->m.dev.curprop)->v.dev.aliases; alp != NULL; alp = alp->next) {
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
str2uuidx(const ddlchar_t *uuidstr, uuid_t uuid)
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
const char allflags[] = {
	"valid "
	"read "
	"write "
	"event "
	"vsize "
	"abs "
	"persistent "
	"constant "
	"volatile "
};

const char *
flagnames(enum propflags_e flags)
{
	static char names[sizeof(allflags) - 1];
	const char *sp;
	char *dp;

	if (flags == 0) return "";

	assert(flags < (pflg_volatile << 1));

	sp = allflags;
	dp = names;
	for (;flags != 0; flags >>= 1) {
		if ((flags & 1))
			while ((*dp++ = *sp++) != ' ') {/* nothing */} 
		else
			while (*sp++ != ' ') {/* nothing */}
	}
	*(dp - 1) = 0;
	return names;
}

/**********************************************************************/
int
subsparam(struct dcxt_s *dcxp, const ddlchar_t *name, const ddlchar_t **subs)
{
	struct param_s *parp;

	for (parp = itsdevice(dcxp->m.dev.curprop)->v.dev.params; parp; parp = parp->nxt) {
		if (strcmp(parp->name, name) == 0) {
			*subs = parp->subs;
			return 1;
		}
	}
	return 0;
}

/**********************************************************************/
static int
getboolatt(const ddlchar_t *str)
{
	if (!str || strcmp(str, "false") == 0) return 0;
	if (strcmp(str, "true") == 0) return 1;
	acnlogmark(lgERR, "     bad boolean \"%s\"", str);
	return 0;
}

/**********************************************************************/
int
gooduint(const ddlchar_t *str, uint32_t *rslt)
{
	ddlchar_t *eptr;
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
goodint(const ddlchar_t *str, int32_t *rslt)
{
	ddlchar_t *eptr;
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
elem_text(void *data, const ddlchar_t *txt, int len)
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
startText(struct dcxt_s *dcxp, const ddlchar_t *paramname)
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
const ddlchar_t *
endText(struct dcxt_s *dcxp)
{
	if (dcxp->txtlen == -1) return dcxp->txt.p;

	XML_SetCharacterDataHandler(dcxp->parser, NULL);
	dcxp->txt.ch[dcxp->txtlen] = 0;  /* terminate */
	return dcxp->txt.ch;
}

/**********************************************************************/
struct prop_s *
newprop(struct dcxt_s *dcxp, vtype_t vtype, const ddlchar_t *arrayp, const ddlchar_t *propID)
{
	struct prop_s *pp;
	struct prop_s *parent;
	uint32_t arraysize;

	if (arrayp == NULL || gooduint(arrayp, &arraysize) != 0) {
		arraysize = 1;
	}

	/* allocate a new property */
	pp = acnNew(struct prop_s);
	parent = dcxp->m.dev.curprop;
	pp->parent = parent;	/* link to our parent */
	pp->vtype = vtype;

	pp->siblings = parent->children;	/* link us to parents children */
	parent->children = pp;
	pp->childaddr = parent->childaddr; /* default - content may override */
	pp->array = arraysize;
	pp->arraytotal = parent->arraytotal * arraysize;
	pp->childinc = parent->childinc;	/* inherit - may get overwritten */
	/* if (parent->array > 1) pp->arrayprop = parent;
	else pp->arrayprop = parent->arrayprop; */

	if (propID) {
		addpropID(pp, propID);
	}

	dcxp->m.dev.curprop = pp;
	return pp;
}

/**********************************************************************/
struct rootprop_s *
newrootprop(struct dcxt_s *dcxp)
{
	struct rootprop_s *root;

	root = acnNew(rootprop_t);
	root->prop.arraytotal = root->prop.array = 1;
	root->prop.vtype = VT_include;
	root->minaddr = 0xffffffff;

	dcxp->m.dev.root = root;
	dcxp->m.dev.curprop = &root->prop;
	return root;
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
void
check_queued_modulex(struct qentry_s *qentry, enum ddlmod_e typefound, const ddlchar_t *uuidstr)
{
	uuid_t dcid;
	ddlchar_t uuidsbuf[UUID_STR_SIZE];
	int fail = 0;

	if (qentry->modtype != typefound) {
		acnlogmark(lgERR, "DDL module: expected %s, found %s",
				modnames[qentry->modtype], modnames[typefound]);
		fail = 1;
	}
	if (str2uuid(uuidstr, dcid) != 0 || !uuidsEq(qentry->uuid, dcid)) {
		acnlogmark(lgERR, "DDL module UUID: expected %s, found %s",
				uuid2str(qentry->uuid, uuidsbuf),
				uuidstr);
		fail = 1;
	}
	if (fail) exit(EXIT_FAILURE);
}
/**********************************************************************/
const ddlchar_t behaviorset_atts[] =
	/* 0 */   "UUID@"
	/* 1 */   "provider@"
	/* 2 */   "date@"
	/* 3 */   "http://www.w3.org/XML/1998/namespace id|"
;

void
bset_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	check_queued_modulex(dcxp->queuehead, mod_bset, atta[0]);
	dcxp->skip = dcxp->nestlvl;
}

/**********************************************************************/
const ddlchar_t languageset_atts[] =
	/* 0 */   "UUID@"
	/* 1 */   "provider@"
	/* 2 */   "date@"
	/* 3 */   "http://www.w3.org/XML/1998/namespace id|"
;

void
lset_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	check_queued_modulex(dcxp->queuehead, mod_lset, atta[0]);
	dcxp->skip = dcxp->nestlvl;
}

/**********************************************************************/
const ddlchar_t device_atts[] =
	/* 0 */   "UUID@"
	/* 1 */   "provider@"
	/* 2 */   "date@"
	/* 3 */   "http://www.w3.org/XML/1998/namespace id|"
;

void
dev_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	struct prop_s *pp;

	check_queued_modulex(dcxp->queuehead, mod_dev, atta[0]);

	pp = dcxp->m.dev.curprop;
	assert(pp->vtype == VT_include);
	pp->vtype = VT_device;
}

/**********************************************************************/
void prop_end(struct dcxt_s *dcxp);

static void
dev_end(struct dcxt_s *dcxp)
{
	prop_end(dcxp);
}

/**********************************************************************/

const ddlchar_t behavior_atts[] =
	/* 0 */   "set@"
	/* 1 */   "name@"
	/* 2 */   "http://www.w3.org/XML/1998/namespace id|"
;

void
behavior_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	struct prop_s *pp;
	uuid_t setuuid;
	const bv_t *bv;

	const ddlchar_t *setp = atta[0];
	const ddlchar_t *namep = atta[1];

	//acnlogmark(lgDBUG, "%4d behavior %s", dcxp->elcount, namep);

	pp = dcxp->m.dev.curprop;
	resolveuuidx(dcxp, setp, setuuid);
	bv = findbv(setuuid, namep, NULL);
	if (bv) {	/* known key */
		if (dcxp->m.dev.nbvs >= PROP_MAXBVS) {
			acnlogmark(lgERR, "%4d property has more than maximum (%u) behaviors", dcxp->elcount, PROP_MAXBVS);
		} else {
			dcxp->m.dev.bvs[dcxp->m.dev.nbvs++] = bv;
		}
	} else if (unknownbvaction) {
		(*unknownbvaction)(dcxp, bv);
	} else {
		acnlogmark(lgNTCE, "%4d unknown behavior: set=%s, name=%s", dcxp->elcount, setp, namep);
	}
}

/**********************************************************************/
const ddlchar_t UUIDname_atts[] =
	/* 0 */   "name@"
	/* 1 */   "UUID@"
	/* 2 */   "http://www.w3.org/XML/1998/namespace id|"
;

void
alias_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	struct uuidalias_s *alp;
	const ddlchar_t *aliasname;
	const ddlchar_t *aliasuuid;

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
	(void)savestr(aliasname, &alp->alias);
	str2uuidx(aliasuuid, alp->uuid);
	alp->next = dcxp->m.dev.curprop->v.dev.aliases;
	dcxp->m.dev.curprop->v.dev.aliases = alp;
	acnlogmark(lgDBUG, "%4d added alias %s", dcxp->elcount, aliasname);
}

/**********************************************************************/
const ddlchar_t property_atts[] =
	/* WARNING: order must match attribute indexes below */
	/* 0 */   "valuetype@"
	/* 1 */   "array|"
	/* 2 */   "http://www.w3.org/XML/1998/namespace id|"
	/* 3 */   "sharedefine|"
;

/*
WARNING: order of tokens in valuetypes[] must match enumeration vtype_e
*/
const ddlchar_t valuetypes[] = "NULL|immediate|implied|network|";

void prop_wrapup(struct dcxt_s *dcxp);

void
prop_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	vtype_t vtype;

	const ddlchar_t *vtypep = atta[0];
	const ddlchar_t *arrayp = atta[1];
	const ddlchar_t *propID = atta[2];

	/*
	if this is the first child we need to wrap up the current 
	property before processing the children.
	*/
	if ((dcxp->m.dev.curprop->children) == NULL)
		prop_wrapup(dcxp);

	if (vtypep == NULL
				|| (vtype = tokmatch(valuetypes, vtypep)) == -1)
	{
		acnlogmark(lgERR, "%4d bad or missing valuetype", dcxp->elcount);
		dcxp->skip = dcxp->nestlvl;
		return;
	}

	(void)newprop(dcxp, vtype, arrayp, propID);  /* newprop links in to dcxp->m.dev.curprop */
}

/**********************************************************************/
void
prop_end(struct dcxt_s *dcxp)
{
	struct prop_s *pp;
	struct proptask_s *tp;

	pp = dcxp->m.dev.curprop;

	/* if no children we still need to wrap up the property */
	if ((pp->children) == NULL)
		prop_wrapup(dcxp);

	/* reverse the order of any children to match documnt order */
	pp->children = reverseproplist(pp->children);
	while ((tp = pp->tasks) != NULL) {
		(*tp->task)(dcxp, pp, tp->ref);
		pp->tasks = tp->next;
		free(tp);
	}

	dcxp->m.dev.curprop = pp->parent;
}

/**********************************************************************/
void
prop_wrapup(struct dcxt_s *dcxp)
{
	struct prop_s *pp;
	int i;

	pp = dcxp->m.dev.curprop;
	switch (pp->vtype) {
	case VT_network:
		if ((pp->v.net.flags & pflg_valid) == 0) {
			acnlogmark(lgERR, "%4d property not valid", dcxp->elcount);
		}
		break;
	case VT_NULL:
	case VT_implied:
	case VT_device:
		break;
	case VT_imm_unknown:
	case VT_imm_uint:
	case VT_imm_sint:
	case VT_imm_float:
	case VT_imm_string:
	case VT_imm_object:
		if (pp->v.imm.count == 0) {
			acnlogmark(lgERR, "%4d no value for immediate property", dcxp->elcount);
		} else if (!(pp->v.imm.count == 1 || pp->v.imm.count == pp->arraytotal)) {
			acnlogmark(lgERR, "%4d wrong value count for immediate property", dcxp->elcount);
		}
		break;
	case VT_include:
			acnlogmark(lgERR, "%4d wrapup includedev", dcxp->elcount);
			exit(EXIT_FAILURE);
	}
	for (i = 0; i < dcxp->m.dev.nbvs; ++i) {
		const bv_t *bv;
		
		bv = dcxp->m.dev.bvs[i];
		if (bv->action) (*bv->action)(dcxp, bv);
	}
	dcxp->m.dev.nbvs = 0;
}

/**********************************************************************/

void
add_proptask(struct prop_s *prop, proptask_fn *task, void *ref)
{
	struct proptask_s *tp;

	tp = acnNew(struct proptask_s);
	tp->task = task;
	tp->ref = ref;
	tp->next = prop->tasks;
	prop->tasks = tp;
}

/**********************************************************************/
/*
Includedev - can't call the included device until we reach the end 
tag because we need to accumulate content first, so we allocate a 
dummy property for now.
*/
const ddlchar_t includedev_atts[] =
	/* 0 */ "UUID@"
	/* 1 */ "array|"
	/* 2 */ "http://www.w3.org/XML/1998/namespace id|"
;

void
incdev_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	struct prop_s *pp;
	struct prop_s *prevp;
	struct qentry_s *qentry;
#if acntestlog(lgDBUG)
	ddlchar_t uuidstr[UUID_STR_SIZE + 1];
#endif

	const ddlchar_t *uuidp = atta[0];
	const ddlchar_t *arrayp = atta[1];
	const ddlchar_t *propID = atta[2];

	/*
	if this is the first child we need to wrap up the current 
	property before processing the children.
	*/
	if ((dcxp->m.dev.curprop->children) == NULL)
		prop_wrapup(dcxp);

	acnlogmark(lgDBUG, "%4d include %s", dcxp->elcount, uuidp);

	qentry = acnNew(struct qentry_s);
	qentry->modtype = mod_dev;

	qentry->ref = pp = newprop(dcxp, VT_include, arrayp, propID);
	resolveuuidx(dcxp, uuidp, qentry->uuid);
	acnlogmark(lgDBUG, "%4d resolved to uuid %s", dcxp->elcount, uuid2str(qentry->uuid, uuidstr));


	queue_dcid(dcxp, qentry);

	/* link to inherited params before adding new ones */
	for (prevp = pp->parent; prevp; prevp = prevp->parent) {
		if (prevp->vtype == VT_include) {
			pp->v.dev.params = prevp->v.dev.params;
			break;
		}
	}
}

/**********************************************************************/
void
incdev_end(struct dcxt_s *dcxp)
{
	prop_t *pp = dcxp->m.dev.curprop;

	if (pp->arraytotal > pp->parent->arraytotal && !pp->childinc) {
		acnlogmark(lgERR,
					"%4d include array with no child increment",
					dcxp->elcount);
	}
	dcxp->m.dev.curprop = pp->parent;
}

/**********************************************************************/
/*
Propertypointer
*/
const ddlchar_t proppointer_atts[] =
	/* 0 */ "ref@"
	/* 1 */ "http://www.w3.org/XML/1998/namespace id|"
;

void
proppointer_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	/*
	if this is the first child we need to wrap up the current 
	property before processing the children.
	*/
	/*
	if ((dcxp->m.dev.curprop->children) == NULL)
		prop_wrapup(dcxp);
	*/
	/*
	FIXME - implement propertypointer
	*/
}

/**********************************************************************/
const ddlchar_t protocol_atts[] =
	/* note: this is a single string */
	/* WARNING: order determines values used in switches below */
	/* 0 */  "name@"
	/* 1 */  "http://www.w3.org/XML/1998/namespace id|"
;

void
protocol_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	if (!atta[0] || strcasecmp(atta[0], "ESTA.DMP") != 0) {
		dcxp->skip = dcxp->nestlvl;
		acnlogmark(lgINFO, "%4d skipping protocol \"%s\"...", dcxp->elcount, atta[0]);
	}
}

/**********************************************************************/
#if CONFIG_DDL_IMMEDIATEPROPS
const ddlchar_t value_atts[] = 
	/* note: this is a single string */
	/* WARNING: order determines values used in switches below */
	/*  0 */ "type@"
	/*  1 */ "http://www.w3.org/XML/1998/namespace id|"
	/*  2 */ "value.paramname|"
;

/* WARNING: order follows immtype_e enumeration */
const ddlchar_t value_types[] = "uint|sint|float|string|object|";

const int vsizes[] = {
	sizeof(uint32_t),
	sizeof(int32_t),
	sizeof(double),
	sizeof(ddlchar_t *),
	sizeof(struct immobj_s),
};

void
value_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	int i;
	const ddlchar_t *ptype = atta[0];
	uint8_t *arrayp;
	struct prop_s *pp;

	pp = dcxp->m.dev.curprop;

	if (pp->vtype < VT_imm_uint && pp->vtype != VT_imm_unknown) {
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

	if (pp->v.imm.count == 0) {
		pp->vtype = i + VT_imm_uint;
	} else {
		if (pp->v.imm.count >= pp->arraytotal) {
			acnlogmark(lgERR, "%4d multiple values in single property",
						dcxp->elcount);
			goto skipvalue;
		}
		if ((i + VT_imm_uint) != pp->vtype) {
			acnlogmark(lgWARN, "%4d value type mismatch \"%s\"",
					dcxp->elcount, ptype);
			goto skipvalue;
		}
		if (pp->v.imm.count == 1) {
			/* need to assign an array for values */
			arrayp = mallocx(vsizes[i] * pp->arraytotal);
			switch (i) {
			case 0:   /* uint */
				*(uint32_t *)arrayp = pp->v.imm.t.ui;
				pp->v.imm.t.Aui = (uint32_t *)arrayp;
				break;
			case 1:   /* sint */
				*(int32_t *)arrayp = pp->v.imm.t.si;
				pp->v.imm.t.Asi = (int32_t *)arrayp;
				break;
			case 2:   /* float */
				*(double *)arrayp = pp->v.imm.t.f;
				pp->v.imm.t.Af = (double *)arrayp;
				break;
			case 3:   /* string */
				*(const ddlchar_t **)arrayp = pp->v.imm.t.str;
				pp->v.imm.t.Astr = (const ddlchar_t **)arrayp;
				break;
			case 4:   /* object */
				*(struct immobj_s *)arrayp = pp->v.imm.t.obj;
				pp->v.imm.t.Aobj = (struct immobj_s *)arrayp;
				break;
			default:
				break;
			}
		}
	}
	startText(dcxp, atta[2]);
}

/**********************************************************************/
void
value_end(struct dcxt_s *dcxp)
{
	struct prop_s *pp;
	const ddlchar_t *vtext;
	uint32_t count;

	pp = dcxp->m.dev.curprop;
	vtext = endText(dcxp);
	acnlogmark(lgDBUG, "%4d got %s value \"%s\"",
					dcxp->elcount, ptypes[pp->vtype], vtext);
	count = pp->v.imm.count;

	switch (pp->vtype) {
	case VT_imm_object: {
		struct immobj_s *objp;

		if (count == 0) objp = &pp->v.imm.t.obj;
		else objp = pp->v.imm.t.Aobj + count;

		objp->size = savestrasobj(vtext, &objp->data);	
	}	break;
	case VT_imm_string: {
		const ddlchar_t **strp;

		if (count == 0) strp = &pp->v.imm.t.str;
		else strp = pp->v.imm.t.Astr + count;

		(void)savestr(vtext, strp);
	}	break;
	default: {
		ddlchar_t *ep;
		
		switch (pp->vtype) {
		case VT_imm_float: {
			double val = strtod(vtext, &ep);

			if (count == 0) pp->v.imm.t.f = val;
			else pp->v.imm.t.Af[count] = val;
		}	break;
		case VT_imm_uint: {
			uint32_t val = strtoul(vtext, &ep, 10);

			if (count == 0) pp->v.imm.t.ui = val;
			else pp->v.imm.t.Aui[count] = val;
		}	break;
		case VT_imm_sint:
		default: {
			int32_t val = strtol(vtext, &ep, 10);
			
			if (count == 0) pp->v.imm.t.si = val;
			else pp->v.imm.t.Asi[count] = val;
		}	break;
		}
		if (*vtext == 0 || ep == vtext || *ep != 0) {
			acnlogmark(lgERR, "%4d immediate value parse error \"%s\"",
							dcxp->elcount, vtext);
		}
	}	break;
	}
	++pp->v.imm.count;
}
#endif /* CONFIG_DDL_IMMEDIATEPROPS */
/**********************************************************************/
const ddlchar_t propref_DMP_atts[] = 
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
propref_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	struct prop_s *pp;
	unsigned int flags;
	struct proptab_s *ptp, **pptp;
	struct rootprop_s *root;
	int32_t maxdiminc;
	uint32_t maxaddr;
	struct prop_s *xpp, *app;

	pp = dcxp->m.dev.curprop;
	flags = pp->v.net.flags;

	if (flags & pflg_valid) {
		acnlogmark(lgERR, "%4d multiple <propref_DMP>s for the same property",
						dcxp->elcount);
		return;
	}

	flags |= pflg_valid;
	if (getboolatt(atta[2])) flags |= pflg_read;
	if (getboolatt(atta[3])) flags |= pflg_write;
	if (getboolatt(atta[4])) flags |= pflg_event;
	if (getboolatt(atta[5])) flags |= pflg_vsize;
	if (getboolatt(atta[7])) flags |= pflg_abs;
	/* "loc" is mandatory */
	if (gooduint(atta[0], &pp->v.net.addr) == 0) {
		if ((flags & pflg_abs) == 0)
			pp->v.net.addr += pp->parent->childaddr;
	} else {
		flags &= ~pflg_valid;
		acnlogmark(lgERR, "%4d bad or missing net location",
						dcxp->elcount);
	}
	/* "size" is mandatory */
	if (gooduint(atta[1], &pp->v.net.size) < 0) {
		flags &= ~pflg_valid;
		acnlogmark(lgERR, "%4d bad or missing size", dcxp->elcount);
	}

	root = dcxp->m.dev.root;
	assert(pp->arraytotal > 0);

	/*
	In case we are in a multidimensional array we need to find 
	the ancestor (or self) property declaring the largest 
	dimension (by item count) as the others dimensions get 
	unrolled in the propfind tables.
	*/
	maxdiminc = 1;
	maxaddr = pp->v.net.addr;

	for (app = xpp = pp; xpp->arraytotal > 1; xpp = xpp->parent) {
		uint32_t arrayinc;

		if (xpp->array == 1) continue;

		if (xpp == pp) {
			/* current property is an array expect an "inc" */
			if (atta[6] == NULL || goodint(atta[6], &pp->v.net.inc) < 0) {
				acnlogmark(lgWARN, "%4d array with no inc - assume 1",
							dcxp->elcount);
				pp->v.net.inc = 1;
			}
			arrayinc = pp->v.net.inc;
		} else {
			arrayinc = xpp->childinc;
		}

		if (xpp->array > app->array) {
			app = xpp;
			maxdiminc = arrayinc;
		} else {
			maxaddr += (xpp->array - 1) * arrayinc;
		}
	}
	if (pp->arraytotal > 1) pp->v.net.maxarrayprop = app;

	/* just count the props for now - they get filled in later */
	root->nflatprops += pp->arraytotal;
	root->nnetprops += pp->arraytotal / app->array;
	if (pp->v.net.addr < root->minaddr) root->minaddr = pp->v.net.addr;
	if (maxaddr > root->maxaddr) root->maxaddr = maxaddr;
	maxaddr += (app->array - 1) * maxdiminc;
	if (maxaddr > root->maxflataddr) root->maxflataddr = maxaddr;

	acnlogmark(lgDBUG, "%4d property addr %u, this dim %u, max dim %d (inc %d), total %u", 
		dcxp->elcount, 
		pp->v.net.addr, 
		pp->array, 
		app->array,
		maxdiminc,
		pp->arraytotal);

	pp->v.net.flags = flags;

	/* find the list of property increments */
	pptp = &(root->ptabs);
	while (1) {
		ptp = *pptp;
		if (ptp == NULL || ptp->inc > maxdiminc) {
			/* need a new table for this increment */
			ptp = acnNew(struct proptab_s);
			ptp->inc = maxdiminc;
			ptp->nxt = *pptp;
			*pptp = ptp;
			break;
		}
		if (ptp->inc == maxdiminc) break;
		pptp = &(ptp->nxt);
	}
	ptp->nprops += pp->arraytotal / app->array;
}

/**********************************************************************/
const ddlchar_t childrule_DMP_atts[] =
	/* WARNING: order determines values used in switches below */
	/*  0 */ "loc|"
	/*  1 */ "inc|"
	/*  2 */ "abs|"
	/*  3 */ "http://www.w3.org/XML/1998/namespace id|"
;

void
childrule_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	struct prop_s *pp;
	uint32_t addoff;

	pp = dcxp->m.dev.curprop;
	addoff = pp->childaddr;
	if (getboolatt(atta[2])) addoff = 0;

	if (gooduint(atta[0], &pp->childaddr) == 0) {
		pp->childaddr += addoff;
	}
	if (atta[1]) gooduint(atta[1], &pp->childinc);
}

/**********************************************************************/
const ddlchar_t setparam_atts[] =
	/* WARNING: order determines values used in switches below */
	/*  0 */ "name@"
	/*  1 */ "setparam.paramname|"
	/*  2 */ "http://www.w3.org/XML/1998/namespace id|"
;

void
setparam_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	const ddlchar_t *name = atta[0];
	struct param_s *parp;

	if (!name) {
		acnlogmark(lgERR, "%4d setparam with no name",
							dcxp->elcount);
		return;
	}
	parp = acnNew(struct param_s);
	(void)savestr(name, &parp->name);
	parp->nxt = dcxp->m.dev.curprop->v.dev.params;
	dcxp->m.dev.curprop->v.dev.params = parp;
	startText(dcxp, atta[1]);
}

/**********************************************************************/
void
setparam_end(struct dcxt_s *dcxp)
{
	struct param_s *parp;

	parp = dcxp->m.dev.curprop->v.dev.params;
	parp->subs = endText(dcxp);
	if (dcxp->txtlen >= 0) (void)savestr(parp->subs, &parp->subs);
	acnlogmark(lgDBUG, "%4d add param %s=\"%s\"",
							dcxp->elcount, parp->name, parp->subs);
}

/**********************************************************************/
typedef void elemstart_fn(struct dcxt_s *dcxp, const ddlchar_t **atta);
typedef void elemend_fn(struct dcxt_s *dcxp);

elemstart_fn *startvec[EL_MAX] = {
	[EL_device]	= &dev_start,
	[EL_behaviorset] = &bset_start,
	[EL_languageset] = &lset_start,
	[EL_property] = &prop_start,
	[EL_propertypointer] = &proppointer_start,
#if CONFIG_DDL_IMMEDIATEPROPS
	[EL_value] = &value_start,
#endif /* CONFIG_DDL_IMMEDIATEPROPS */
	[EL_protocol] = &protocol_start,
	[EL_includedev] = &incdev_start,
	[EL_UUIDname] = &alias_start,
	[EL_propref_DMP] = &propref_start,
	[EL_childrule_DMP] = &childrule_start,
	[EL_setparam] = &setparam_start,
#if CONFIG_DDL_BEHAVIORS
	[EL_behavior] = &behavior_start,
#endif /* CONFIG_DDL_BEHAVIORS */
};

elemend_fn *endvec[EL_MAX] = {
	[EL_device]	= &dev_end,
	[EL_property] = &prop_end,
#if CONFIG_DDL_IMMEDIATEPROPS
	[EL_value] = &value_end,
#endif /* CONFIG_DDL_IMMEDIATEPROPS */
	[EL_includedev] = &incdev_end,
	[EL_setparam] = &setparam_end,
};

const ddlchar_t * const elematts[EL_MAX] = {
	/* [EL_DDL] = DDL_atts, */
	[EL_UUIDname] = UUIDname_atts,
	/*
	[EL_alternatefor] = alternatefor_atts,
	*/
	[EL_behavior] = behavior_atts,
	/*
	[EL_behaviordef] = behaviordef_atts,
	*/
	[EL_behaviorset] = behaviorset_atts,
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
	[EL_propertypointer] = proppointer_atts,
	/*
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
el_start(void *data, const ddlchar_t *el, const ddlchar_t **atts)
{
	struct dcxt_s *dcxp = (struct dcxt_s *)data;
	int eltok;
	const ddlchar_t *cxtatts;
	const ddlchar_t *atta[MAXATTS];
	int i;
	bool bad;
	const ddlchar_t *cp;
	const ddlchar_t *tp;

	++dcxp->elcount;
	if (dcxp->skip) {
		++dcxp->nestlvl;
		return;
	}

	if (dcxp->nestlvl >= CONFIG_DDL_MAXNEST) {
		acnlogmark(lgERR, "E%4d Maximum XML nesting reached. skipping...", dcxp->elcount);
		dcxp->skip = dcxp->nestlvl++;
		return;
	}

	//acnlogmark(lgDBUG, "<%s %d>", el, dcxp->nestlvl);

	if ((eltok = gettok(el, dcxp->elestack[dcxp->nestlvl++])) < 0) {
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
el_end(void *data, const ddlchar_t *el)
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
void
parsemodules(struct dcxt_s *dcxp)
{
	int fd;
	int sz;
	void *buf;
#if acntestlog(lgDBUG)
	ddlchar_t uuidstr[UUID_STR_SIZE];
#endif

	while ((dcxp->queuehead)) {
		acnlogmark(lgDBUG, "     parse   %s", uuid2str(dcxp->queuehead->uuid, uuidstr));

		switch (dcxp->queuehead->modtype) {
		case mod_dev:
			dcxp->m.dev.curprop = (prop_t *)dcxp->queuehead->ref;
			break;
		case mod_lset:
		case mod_bset:
			break;
		}

		dcxp->elcount = 0;
		fd = openddlx(dcxp->queuehead->uuid, NULL);

		if (dcxp->parser == NULL) {
			dcxp->parser = XML_ParserCreateNS(NULL, ' ');
		} else {
			if (XML_ParserReset(dcxp->parser, NULL) == XML_FALSE) {
				acnlogmark(lgERR, "Can't reset parser");
				exit(EXIT_FAILURE);
			}
		}
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
	
		close(fd);	/* leave files as we found them */

		acnlogmark(lgDBUG, "     end     %s", uuid2str(dcxp->queuehead->uuid, uuidstr));

		{
			struct qentry_s *qentry = dcxp->queuehead;

			dcxp->queuehead = qentry->next;
			free(qentry);
		}
	}
	dcxp->queuetail = NULL;
	XML_ParserFree(dcxp->parser);
}
/**********************************************************************/
void initdcxt(struct dcxt_s *dcxp)
{
	memset(dcxp, 0, sizeof(struct dcxt_s));
	dcxp->elestack[0] = ELx_ROOT;
}

/**********************************************************************/
rootprop_t *
parsedevice(const char *uuidstr)
{
	struct dcxt_s dcxt;
	struct qentry_s *qentry;
	rootprop_t *dev;

	initdcxt(&dcxt);

	qentry = acnNew(struct qentry_s);
	qentry->modtype = mod_dev;
	dev = newrootprop(&dcxt);

	qentry->ref = &dev->prop;
	str2uuidx(uuidstr, qentry->uuid);

	queue_dcid(&dcxt, qentry);
	parsemodules(&dcxt);
	acnlogmark(lgDBUG, "Found %d net properties", dev->nnetprops);
	acnlogmark(lgDBUG, " %d flat net properties", dev->nflatprops);
	acnlogmark(lgDBUG, " min addr %u", dev->minaddr);
	acnlogmark(lgDBUG, " max addr %u", dev->maxaddr);
	acnlogmark(lgDBUG, " max array addr %u", dev->maxflataddr);
	return dev;
}

/**********************************************************************/
void
queue_dcid(struct dcxt_s *dcxp, struct qentry_s *qentry)
{
	assert(qentry->next == NULL);

	if (dcxp->queuehead == NULL) dcxp->queuehead = qentry;
	else dcxp->queuetail->next = qentry;
	dcxp->queuetail = qentry;
}

/**********************************************************************/
void
freeprop(struct prop_s *prop)
{
	
	while (1) {
		struct proptask_s *tp;

		tp = prop->tasks;
		if (tp == NULL) break;
		prop->tasks = tp->next;
		free(tp);
	}
	
	if (prop->id) free((void *)prop->id);
	
	while (1) {
		struct prop_s *pp;

		pp = prop->children;
		if (pp == NULL) break;
		prop->children = pp->siblings;
		pp->siblings = NULL;
		freeprop(pp);
	}
	
	
	/*
	FIXME: This leaks memory like a sieve!
	Should Free up all the aliasnames, parameters, behaviors etc.
	*/


	switch (prop->vtype) {
	case VT_NULL:
	case VT_imm_unknown:
	case VT_implied:
	case VT_network:
		break;
	case VT_include:
	case VT_device:
		while (1) {
			struct param_s *pp;
	
			pp = prop->v.dev.params;
			if (pp == NULL) break;
			prop->v.dev.params = pp->nxt;
			free((void *)pp->name);
			// free(pp->subs);	/* FIXME: may be double substitution */
			free(pp);
		}
		while (1) {
			struct uuidalias_s *ap;

			ap = prop->v.dev.aliases;
			if (ap == NULL) break;
			prop->v.dev.aliases = ap->next;
			free((void *)ap->alias);
			free(ap);
		}
		break;
	case VT_imm_uint:
	case VT_imm_sint:
	case VT_imm_float:
		if (prop->v.imm.count > 1) {
			free(prop->v.imm.t.ptr);
		}
		break;
	case VT_imm_string:
		if (prop->v.imm.count > 1) {
			uint32_t i;

			for (i = 0; i < prop->v.imm.count; ++i)
				free((void *)prop->v.imm.t.Astr[i]);
			free(prop->v.imm.t.Astr);
		} else {
			free((void *)prop->v.imm.t.str);
		}
		break;
	case VT_imm_object:
		if (prop->v.imm.count > 1) {
			uint32_t i;

			for (i = 0; i < prop->v.imm.count; ++i)
				free(prop->v.imm.t.Aobj[i].data);
			free(prop->v.imm.t.Aobj);
		} else {
			free(prop->v.imm.t.obj.data);
		}
		break;
	}
	free(prop);
}

/**********************************************************************/
void
freerootprop(struct rootprop_s *root)
{
	/*
	WARNING: This only works because &(root->prop) == root so the 
	final free(prop) correctly frees the root. If struct rootprop_s 
	changes such that this isn't true then this function must be 
	revised.
	*/
	freeprop((struct prop_s *)root);
}
