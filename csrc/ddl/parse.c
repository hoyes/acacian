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
file: parse.c

Parser for DDL (Device Description Language)

For both devices and controllers, the DDL description is used to 
generate structures which are used by DMP.

about: Devices

For building a simple device, the property map is a static part of 
the code of the device itself, and the device has no need for DDL 
(except to serve it up as opaque files as required by EPI-11.

When building a device DDL description forms part of the build (see 
<mapgen.c>), then if extra functions or properties need to be added, 
this is done by modifying the DDL and so ensures that the 
description should match the actual properties.

The property map in a device relates incoming addresses to a handler 
routine (provided by the application code) which is called by DMP 
when the corresponding property is addressed.

DDL extension:

To allow the application layer handler function to be specified within 
the DDL itself, an expension element has been defined. This is the 
element <ea:propext name="" value=""/> which is in the namespace:
"http://www.engarts.com/namespace/2011/ddlx" This can be added within 
any <protocol> element and allows a generic specification of a field 
name and value for incorporation into the property map. The use of a 
namespace allows this extension element to be added to descriptions 
within the rules of DDLv1.1 and compliant parsers should ignore it 
(unless they are programmed to use it).

about: controllers

Controllers must parse DDL dynamically on encountering new device 
types. Once parsed, the resulting structures may be stored and 
tracked by DCID using the generic UUID code which is also used for 
tracking components by CID.

about: Structures generated during parse

The main entry point <parseroot> returns a <struct rootdev_s> which 
in turn contains three principle structures:

- a tree structure consisting of linked <struct ddlprop_s>s 
representing the entire device including all its subdevices and 
containing all declared properties including immediate, implied and 
NULL value ones and propertypointers. Once parsing is complete this 
tree is no longer used by any Acacian code but it may be useful to 
the application. In particular it can be iterated through property 
by property or interrogated recursively to examine the device 
structure, immediate values, labels etc.

- a <struct addrmap_s> which is used extensively by DMP code to 
connect the DMP adresses in incoming messages to the corresponding 
property structures, or for looking up property sizes and attributes 
when constructing outgoing messages. This is an operation which must 
be fast and efficient.

- a singly linked list of <struct dmpprop_s>s with one for each 
declared DMP accessible property. The list format is merely a 
convenient for, to record these structures in. they are cross 
referenced both from the ddlprop_s tree and the addrmap_s so should 
not be freed until after those structures. The rootdev_s also contains
a string pool and hash table for fast lookup of ID references.

*/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "expat.h"
#include "acn.h"
/**********************************************************************/
/*
Logging facility
*/
#define lgFCTY LOG_DDL
/**********************************************************************/
/* bufer size for expat parser */
#define BUFF_SIZE 2048
/* initial and incremental sizes for address map */
/*
macros: address map memory blocks

	AFMAPINC - increment to grow map when required
	AFMAPISIZE - initial size of address map
	AFTESTBLKINC - increment used to grow addrfind.p when ntests > 1
*/
#define AFMAPINC (32 * sizeof(struct addrfind_s))
#define AFMAPISIZE AFMAPINC
#define AFTESTBLKINC 8
/**********************************************************************/
/*
Grammar summary - refer to DTD for detail:
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
/**********************************************************************/
/*
about: Tokenising

Tokens are statically declared and have an enumeration and a string value.
All tokens are stored within a single token array alltoks[] indexed by
enumeration value.

Two routines are provided to test whether a name matches a context 
dependent set of allowable tokens. These are virtually identical 
except that tokmatchtok() returns the token itself whilst 
tokmatchofs() returns the offset of the name in the allowed array. 
So if the name *is* in the allowed array then tokmatchtok(name, 
allowed) == allowed->toks[tokmatchofs(name, allowed)].

sorting:

The token search routines use a binary search which means the keys 
must be sorted. In this case that means that the tokens in an 
allowtok_s MUST be in ascending lexical order of the strings they 
represent. To facilitate sorting, most token names in C are simply 
the token string with TK_ prepended. For token strings which are 
very long or contains characters which do not make valif C names, the
name is derived from the string so that it sorts in the correct 
order.

If the C names of tokens sort in the same order as the strings they 
represent, then when defining allowtok_s structures it is simple to 
sort the elements correctly using:

  LC_ALL=C sort

*/
/*
Symbolic names for tokens
*/
enum token_e {
/* DDL element names come first */
/*
Group element names first since they index several tables of other items.
*/

	TK_DDL,
	TK_alternatefor,
	TK_extends,
	TK_UUIDname,
	TK_languageset,
	TK_language,
	TK_string,
	TK_behaviorset,
	TK_behaviordef,
	TK_refines,
	TK_section,
	TK_hd,
	TK_p,
	TK_device,
	TK_parameter,
	TK_property,
	TK_behavior,
	TK_propertypointer,
	TK_includedev,
	TK_useprotocol,
	TK_protocol,
/* DMP specific elements */
	TK_propref_DMP,
	TK_childrule_DMP,
/*  DMX/E1.31 specific elements */
/* elements from other namespaces */
	TK_http_engarts_propext,
/* parameterizable elements. Observe TK_paramname() below */
	TK_choice,
	TK_label,
	TK_mininclusive,
	TK_maxinclusive,
	TK_refinement,
	TK_setparam,
	TK_value,
	TK_propmap_DMX,
	TK__elmax_,
/* parameterizable attributes follow immediately */
	TK_UUID,
	TK_abs,
	TK_array,
	TK_event,
	TK_inc,
	TK_key,
	TK_loc,
	TK_name,
	TK_read,
	TK_ref,
	TK_set,
	TK_sharedefine,
	TK_size,
	TK_type,
	TK_valuetype,
	TK_varsize,
	TK_write,
/* paramname attributes for elements and attributes MUST match order */
/* exactly so TK_paramname() works */
/* (use '0' in token name as it sorts closer to '.' than '_' does) */
/* paramname attributes for element names */
	TK_choice0paramname,
	TK_label0paramname,
	TK_mininclusive0paramname,
	TK_maxinclusive0paramname,
	TK_refinement0paramname,
	TK_setparam0paramname,
	TK_value0paramname,
	TK_propmap_DMX0paramname,
/* paramname attributes for attribute names */
	TK_UUID0paramname,
	TK_abs0paramname,
	TK_array0paramname,
	TK_event0paramname,
	TK_inc0paramname,
	TK_key0paramname,
	TK_loc0paramname,
	TK_name0paramname,
	TK_read0paramname,
	TK_ref0paramname,
	TK_set0paramname,
	TK_sharedefine0paramname,
	TK_size0paramname,
	TK_type0paramname,
	TK_valuetype0paramname,
	TK_varsize0paramname,
	TK_write0paramname,
/* Non parameterizable attribute names */
	TK_altlang,
	TK_date,
	TK_lang,
	TK_provider,
	TK_version,
/* attribute names from other namespaces */
	TK_http_w3_id,
	TK_http_w3_space,
/* Property types */
	TK_NULL,
	TK_immediate,
	TK_implied,
	TK_network,
/* value types */
	TK_uint,
	TK_float,
	TK_sint,
	TK_object,
/* "string" already has a token */
/* values for attribute "name" on peopext */
#ifdef ACNCFG_PROPEXT_TOKS
#undef _EXTOKEN_
#define _EXTOKEN_(tk, type) TK_ ## tk ,
	ACNCFG_PROPEXT_TOKS
#endif
	TK_unknown,
	TK__max_,
};
/*
macros for tokens
*/
#define TK__none_ ((tok_t)(-1))
#define ISTOKEN(tk) ((tk) < TK__max_)
#define ISELTOKEN(tk) ((tk) < TK__elmax_)

#define ISPARAMNAME(TK) ((TK) >= TK_choice0paramname && (TK) <= TK_write0paramname)
#define PARAMTOK(TK) ((TK) - (TK_choice0paramname - TK_choice))

/*
String values for each token
*/
const ddlchar_t *tokstrs[] = {
/* DDL element names come first */ \
	[TK_DDL]                     = "DDL",
	[TK_alternatefor]            = "alternatefor",
	[TK_extends]                 = "extends",
	[TK_UUIDname]                = "UUIDname",
	[TK_languageset]             = "languageset",
	[TK_language]                = "language",
	[TK_string]                  = "string",
	[TK_behaviorset]             = "behaviorset",
	[TK_behaviordef]             = "behaviordef",
	[TK_refines]                 = "refines",
	[TK_section]                 = "section",
	[TK_hd]                      = "hd",
	[TK_p]                       = "p",
	[TK_device]                  = "device",
	[TK_parameter]               = "parameter",
	[TK_property]                = "property",
	[TK_behavior]                = "behavior",
	[TK_propertypointer]         = "propertypointer",
	[TK_includedev]              = "includedev",
	[TK_useprotocol]             = "useprotocol",
	[TK_protocol]                = "protocol",
/* DMP specific elements */
	[TK_propref_DMP]             = "propref_DMP",
	[TK_childrule_DMP]           = "childrule_DMP",
/* elements from other namespaces */
	[TK_http_engarts_propext]    = "http://www.engarts.com/namespace/2011/ddlx propext",
/* parameterizable elements. Observe TK_paramname() below */
	[TK_choice]                  = "choice",
	[TK_label]                   = "label",
	[TK_mininclusive]            = "mininclusive",
	[TK_maxinclusive]            = "maxinclusive",
	[TK_refinement]              = "refinement",
	[TK_setparam]                = "setparam",
	[TK_value]                   = "value",
	[TK_propmap_DMX]             = "propmap_DMX",  /*  DMX/E1.31 specific */
/* parameterizable attributes follow immediately */
	[TK_UUID]                    = "UUID",
	[TK_abs]                     = "abs",
	[TK_array]                   = "array",
	[TK_event]                   = "event",
	[TK_inc]                     = "inc",
	[TK_key]                     = "key",
	[TK_loc]                     = "loc",
	[TK_name]                    = "name",
	[TK_read]                    = "read",
	[TK_ref]                     = "ref",
	[TK_set]                     = "set",
	[TK_sharedefine]             = "sharedefine",
	[TK_size]                    = "size",
	[TK_type]                    = "type",
	[TK_valuetype]               = "valuetype",
	[TK_varsize]                 = "varsize",
	[TK_write]                   = "write",
/* paramname attributes for elements and attributes MUST match order */
/* exactly so TK_paramname() works */
/* (use '0' in token name as it sorts closer to '.' than '_' does) */
/* paramname attributes for element names */
	[TK_choice0paramname]        = "choice.paramname",
	[TK_label0paramname]         = "label.paramname",
	[TK_mininclusive0paramname]  = "mininclusive.paramname",
	[TK_maxinclusive0paramname]  = "maxinclusive.paramname",
	[TK_refinement0paramname]    = "refinement.paramname",
	[TK_setparam0paramname]      = "setparam.paramname",
	[TK_value0paramname]         = "value.paramname",
	[TK_propmap_DMX0paramname]   = "propmap_DMX.paramname",
/* paramname attributes for attribute names */
	[TK_UUID0paramname]          = "UUID.paramname",
	[TK_abs0paramname]           = "abs.paramname",
	[TK_array0paramname]         = "array.paramname",
	[TK_event0paramname]         = "event.paramname",
	[TK_inc0paramname]           = "inc.paramname",
	[TK_key0paramname]           = "key.paramname",
	[TK_loc0paramname]           = "loc.paramname",
	[TK_name0paramname]          = "name.paramname",
	[TK_read0paramname]          = "read.paramname",
	[TK_ref0paramname]           = "ref.paramname",
	[TK_set0paramname]           = "set.paramname",
	[TK_sharedefine0paramname]   = "sharedefine.paramname",
	[TK_size0paramname]          = "size.paramname",
	[TK_type0paramname]          = "type.paramname",
	[TK_valuetype0paramname]     = "valuetype.paramname",
	[TK_varsize0paramname]       = "varsize.paramname",
	[TK_write0paramname]         = "write.paramname",
/* Non parameterizable attribute names */
	[TK_altlang]                 = "altlang",
	[TK_date]                    = "date",
	[TK_lang]                    = "lang",
	[TK_provider]                = "provider",
	[TK_version]                 = "version",
/* attribute names from other namespaces */
	[TK_http_w3_id]              = "http://www.w3.org/XML/1998/namespace id",
	[TK_http_w3_space]           = "http://www.w3.org/XML/1998/namespace space",
/* Property types */
	[TK_NULL]                    = "NULL",
	[TK_immediate]               = "immediate",
	[TK_implied]                 = "implied",
	[TK_network]                 = "network",
/* value types */
	[TK_uint]                    = "uint",
	[TK_float]                   = "float",
	[TK_sint]                    = "sint",
	[TK_object]                  = "object",
/* "string" already has a token */
/* values for attribute "name" on peopext */
#ifdef ACNCFG_PROPEXT_TOKS
#undef _EXTOKEN_
#define _EXTOKEN_(tk, type) [TK_ ## tk] = # tk ,
	ACNCFG_PROPEXT_TOKS
#endif
	[TK_unknown]                 = "unknown",
};
/**********************************************************************/
/*
permissible child element definitions
*/
const struct allowtok_s content_EMPTY = {
	.ntoks = 0
};
const struct allowtok_s content_DOCROOT = {
	.ntoks = 1,
	.toks = {
		TK_DDL,
	}
};
const struct allowtok_s content_DDL = {
	.ntoks = 3,
	.toks = {
		TK_behaviorset,
		TK_device,
		TK_languageset,
	}
};
const struct allowtok_s content_languageset = {
	.ntoks = 5,
	.toks = {
		TK_UUIDname,
		TK_alternatefor,
		TK_extends,
		TK_label,
		TK_language,
	}
};
const struct allowtok_s content_language = {
	.ntoks = 2,
	.toks = {
		TK_label,
		TK_string,
	}
};
const struct allowtok_s content_behaviorset = {
	.ntoks = 5,
	.toks = {
		TK_UUIDname,
		TK_alternatefor,
		TK_behaviordef,
		TK_extends,
		TK_label,
	}
};
const struct allowtok_s content_behaviordef = {
	.ntoks = 3,
	.toks = {
		TK_label,
		TK_refines,
		TK_section,
	}
};
const struct allowtok_s content_section = {
	.ntoks = 3,
	.toks = {
		TK_hd,
		TK_p,
		TK_section,
	}
};
const struct allowtok_s content_device = {
	.ntoks = 9,
	.toks = {
		TK_UUIDname,
		TK_alternatefor,
		TK_extends,
		TK_includedev,
		TK_label,
		TK_parameter,
		TK_property,
		TK_propertypointer,
		TK_useprotocol,
	}
};
const struct allowtok_s content_deviceprops = {
	.ntoks = 3,
	.toks = {
		TK_includedev,
		TK_property,
		TK_propertypointer,
	}
};
const struct allowtok_s content_parameter = {
	.ntoks = 5,
	.toks = {
		TK_choice,
		TK_label,
		TK_maxinclusive,
		TK_mininclusive,
		TK_refinement,
	}
};
const struct allowtok_s content_property = {
	.ntoks = 7,
	.toks = {
		TK_behavior,
		TK_includedev,
		TK_label,
		TK_property,
		TK_propertypointer,
		TK_protocol,
		TK_value,
	}
};
const struct allowtok_s content_includedev = {
	.ntoks = 3,
	.toks = {
		TK_label,
		TK_protocol,
		TK_setparam,
	}
};
const struct allowtok_s content_protocol = {
	.ntoks = 4,
	.toks = {
		TK_childrule_DMP,
		TK_http_engarts_propext,
		TK_propmap_DMX,
		TK_propref_DMP,
	}
};

/*
This table contains the element tokens to search for given a current 
tokens. This is therefore a loose content model for DDL (loose 
because it doesn't enforce ordering or other rules).
*/
const struct allowtok_s * const content[TK__elmax_] = {
	[TK_DDL]                  = &content_DDL,
	[TK_alternatefor]         = NULL,
	[TK_extends]              = NULL,
	[TK_UUIDname]             = NULL,
	[TK_languageset]          = &content_languageset,
	[TK_language]             = &content_language,
	[TK_string]               = NULL,
	[TK_behaviorset]          = &content_behaviorset,
	[TK_behaviordef]          = &content_behaviordef,
	[TK_refines]              = NULL,
	[TK_section]              = &content_section,
	[TK_hd]                   = NULL,
	[TK_p]                    = NULL,
	[TK_device]               = &content_device,
	[TK_parameter]            = &content_parameter,
	[TK_property]             = &content_property,
	[TK_behavior]             = NULL,
	[TK_propertypointer]      = NULL,
	[TK_includedev]           = &content_includedev,
	[TK_useprotocol]          = NULL,
	[TK_protocol]             = &content_protocol,
	[TK_propref_DMP]          = NULL,
	[TK_childrule_DMP]        = NULL,
	[TK_propmap_DMX]          = NULL,
	[TK_http_engarts_propext] = NULL,
	[TK_choice]               = NULL,
	[TK_label]                = NULL,
	[TK_mininclusive]         = NULL,
	[TK_maxinclusive]         = NULL,
	[TK_refinement]           = NULL,
	[TK_setparam]             = NULL,
	[TK_value]                = NULL,
};
/**********************************************************************/
/*
permissible attribute definitions
*/
const struct allowtok_s atts_DDL = {
	.ntoks = 2,
	.toks = {
		TK_http_w3_id,
		TK_version,
	}
};
#define RQA_DDL (1 << 1)

const struct allowtok_s atts_label = {
	.ntoks = 6,
	.toks = {
		TK_http_w3_id,             /* 0 */
		TK_key,                    /* 1 */
		TK_key0paramname,          /* 2 */
		TK_label0paramname,        /* 3 */
		TK_set,                    /* 4 */
		TK_set0paramname,          /* 5 */
	}
};
#define RQA_label 0

const struct allowtok_s atts_alternatefor_extends = {
	.ntoks = 3,
	.toks = {
		TK_UUID,                   /* 0 */
		TK_UUID0paramname,         /* 1 */
		TK_http_w3_id,             /* 2 */
	}
};
#define RQA_alternatefor_extends (1 << 0)

const struct allowtok_s atts_UUIDname = {
	.ntoks = 3,
	.toks = {
		TK_UUID,                   /* 0 */
		TK_http_w3_id,             /* 1 */
		TK_name,                   /* 2 */
	}
};
#define RQA_UUIDname (1 << 0 | 1 << 2)

const struct allowtok_s atts_module = {
	.ntoks = 4,
	.toks = {
		TK_UUID,                   /* 0 */
		TK_date,                   /* 1 */
		TK_http_w3_id,             /* 2 */
		TK_provider,               /* 3 */
	}
};
#define RQA_module (1 << 0 | 1 << 1 | 1 << 3)

const struct allowtok_s atts_language = {
	.ntoks = 3,
	.toks = {
		TK_altlang,                /* 0 */
		TK_http_w3_id,             /* 1 */
		TK_lang,                   /* 2 */
	}
};
#define RQA_language (1 << 2)

#if ACNCFG_STR_FOLDSPACE
const struct allowtok_s atts_string = {
	.ntoks = 3,
	.toks = {
		TK_http_w3_id,             /* 0 */
		TK_http_w3_space,          /* 1 */
		TK_key,                    /* 2 */
	}
};
#define RQA_string (1 << 2)
#else
const struct allowtok_s atts_string = {
	.ntoks = 2,
	.toks = {
		TK_http_w3_id,             /* 0 */
		TK_key,                    /* 2 */
	}
};
#define RQA_string (1 << 1)
#endif

const struct allowtok_s atts_behaviordef = {
	.ntoks = 2,
	.toks = {
		TK_http_w3_id,             /* 0 */
		TK_name,                   /* 1 */
	}
};
#define RQA_behaviordef (1 << 1)

const struct allowtok_s atts_refines = {
	.ntoks = 3,
	.toks = {
		TK_http_w3_id,             /* 0 */
		TK_name,                   /* 1 */
		TK_set,                    /* 2 */
	}
};
#define RQA_refines (1 << 1 | 1 << 2)

const struct allowtok_s atts_idonly = {
	.ntoks = 1,
	.toks = {
		TK_http_w3_id,             /* 0 */
	}
};
#define RQA_section (0)
#define RQA_hd (0)

const struct allowtok_s atts_p = {
	.ntoks = 2,
	.toks = {
		TK_http_w3_id,             /* 0 */
		TK_http_w3_space,          /* 1 */
	}
};
#define RQA_p (0)

const struct allowtok_s atts_parameter = {
	.ntoks = 2,
	.toks = {
		TK_http_w3_id,             /* 0 */
		TK_name,                   /* 1 */
	}
};
#define RQA_parameter (1 << 1)

const struct allowtok_s atts_choice = {
	.ntoks = 2,
	.toks = {
		TK_choice0paramname,       /* 0 */
		TK_http_w3_id,             /* 1 */
	}
};
#define RQA_choice (0)

const struct allowtok_s atts_mininclusive = {
	.ntoks = 2,
	.toks = {
		TK_http_w3_id,             /* 0 */
		TK_mininclusive0paramname, /* 1 */
	}
};
#define RQA_mininclusive (0)

const struct allowtok_s atts_maxinclusive = {
	.ntoks = 2,
	.toks = {
		TK_http_w3_id,             /* 0 */
		TK_maxinclusive0paramname, /* 1 */
	}
};
#define RQA_maxinclusive (0)

const struct allowtok_s atts_refinement = {
	.ntoks = 2,
	.toks = {
		TK_http_w3_id,             /* 0 */
		TK_refinement0paramname,   /* 1 */
	}
};
#define RQA_refinement (0)

const struct allowtok_s atts_property = {
	.ntoks = 7,
	.toks = {
		TK_array,                  /* 0 */
		TK_array0paramname,        /* 1 */
		TK_http_w3_id,             /* 2 */
		TK_sharedefine,            /* 3 */
		TK_sharedefine0paramname,  /* 4 */
		TK_valuetype,              /* 5 */
		TK_valuetype0paramname,    /* 6 */
	}
};
#define RQA_property (1 << 5)

const struct allowtok_s atts_behavior = {
	.ntoks = 5,
	.toks = {
		TK_http_w3_id,             /* 0 */
		TK_name,                   /* 1 */
		TK_name0paramname,         /* 2 */
		TK_set,                    /* 3 */
		TK_set0paramname,          /* 4 */
	}
};
#define RQA_behavior (1 << 1 | 1 << 3)

const struct allowtok_s atts_value = {
	.ntoks = 4,
	.toks = {
		TK_http_w3_id,             /* 0 */
		TK_type,                   /* 1 */
		TK_type0paramname,         /* 2 */
		TK_value0paramname,        /* 3 */
	}
};
#define RQA_value (1 << 1)

const struct allowtok_s atts_propertypointer = {
	.ntoks = 3,
	.toks = {
		TK_http_w3_id,             /* 0 */
		TK_ref,                    /* 1 */
		TK_ref0paramname,          /* 2 */
	}
};
#define RQA_propertypointer (1 << 1)

const struct allowtok_s atts_includedev = {
	.ntoks = 5,
	.toks = {
		TK_UUID,                   /* 0 */
		TK_UUID0paramname,         /* 1 */
		TK_array,                  /* 2 */
		TK_array0paramname,        /* 3 */
		TK_http_w3_id,             /* 4 */
	}
};
#define RQA_includedev (1 << 0)

const struct allowtok_s atts_setparam = {
	.ntoks = 3,
	.toks = {
		TK_http_w3_id,             /* 0 */
		TK_name,                   /* 1 */
		TK_setparam0paramname,     /* 2 */
	}
};
#define RQA_setparam (1 << 1)

const struct allowtok_s atts_useprotocol = {
	.ntoks = 3,
	.toks = {
		TK_http_w3_id,             /* 0 */
		TK_name,                   /* 1 */
		TK_name0paramname,         /* 2 */
	}
};
#define RQA_useprotocol (1 << 1)

const struct allowtok_s atts_protocol = {
	.ntoks = 3,
	.toks = {
		TK_http_w3_id,             /* 0 */
		TK_name,                   /* 1 */
		TK_name0paramname,         /* 2 */
	}
};
#define RQA_protocol (1 << 1)

const struct allowtok_s atts_propref_DMP = {
	.ntoks = 17,
	.toks = {
		TK_abs,                /*  0 */
		TK_abs0paramname,      /*  1 */
		TK_event,              /*  2 */
		TK_event0paramname,    /*  3 */
		TK_http_w3_id,         /*  4 */
		TK_inc,                /*  5 */
		TK_inc0paramname,      /*  6 */
		TK_loc,                /*  7 */
		TK_loc0paramname,      /*  8 */
		TK_read,               /*  9 */
		TK_read0paramname,     /* 10 */
		TK_size,               /* 11 */
		TK_size0paramname,     /* 12 */
		TK_varsize,            /* 13 */
		TK_varsize0paramname,  /* 14 */
		TK_write,              /* 15 */
		TK_write0paramname,    /* 16 */
	}
};
#define RQA_propref_DMP (1 << 7 | 1 << 11)

const struct allowtok_s atts_childrule_DMP = {
	.ntoks = 7,
	.toks = {
		TK_abs,                    /* 0 */
		TK_abs0paramname,          /* 1 */
		TK_http_w3_id,             /* 2 */
		TK_inc,                    /* 3 */
		TK_inc0paramname,          /* 4 */
		TK_loc,                    /* 5 */
		TK_loc0paramname,          /* 6 */
	}
};
#define RQA_childrule_DMP (1 << 5)

const struct allowtok_s atts_propmap_DMX = {
	.ntoks = 6,
	.toks = {
		TK_http_w3_id,             /* 0 */
		TK_inc,                    /* 1 */
		TK_inc0paramname,          /* 2 */
		TK_propmap_DMX0paramname,  /* 3 */
		TK_size,                   /* 4 */
		TK_size0paramname,         /* 5 */
	}
};
#define RQA_propmap_DMX (1 << 4)

const struct allowtok_s atts_http_engarts_propext = {
	.ntoks = 3,
	.toks = {
		TK_http_w3_id,             /* 0 */
		TK_name,                   /* 1 */
		TK_value,                  /* 2 */
  }
};
#define RQA_http_engarts_propext (1 << 1 | 1 << 2)

const struct allowtok_s * const elematts[TK__elmax_] = {
	[TK_DDL]                  = &atts_DDL,
	[TK_alternatefor]         = &atts_alternatefor_extends,
	[TK_extends]              = &atts_alternatefor_extends,
	[TK_UUIDname]             = &atts_UUIDname,
	[TK_languageset]          = &atts_module,
	[TK_language]             = &atts_language,
	[TK_string]               = &atts_string,
	[TK_behaviorset]          = &atts_module,
	[TK_behaviordef]          = &atts_behaviordef,
	[TK_refines]              = &atts_refines,
	[TK_section]              = &atts_idonly,
	[TK_hd]                   = &atts_idonly,
	[TK_p]                    = &atts_p,
	[TK_device]               = &atts_module,
	[TK_parameter]            = &atts_parameter,
	[TK_property]             = &atts_property,
	[TK_behavior]             = &atts_behavior,
	[TK_propertypointer]      = &atts_propertypointer,
	[TK_includedev]           = &atts_includedev,
	[TK_useprotocol]          = &atts_useprotocol,
	[TK_protocol]             = &atts_protocol,
	[TK_propref_DMP]          = &atts_propref_DMP,
	[TK_childrule_DMP]        = &atts_childrule_DMP,
	[TK_propmap_DMX]          = &atts_propmap_DMX,
	[TK_http_engarts_propext] = &atts_http_engarts_propext,
	[TK_choice]               = &atts_choice,
	[TK_label]                = &atts_label,
	[TK_mininclusive]         = &atts_mininclusive,
	[TK_maxinclusive]         = &atts_maxinclusive,
	[TK_refinement]           = &atts_refinement,
	[TK_setparam]             = &atts_setparam,
	[TK_value]                = &atts_value,
};

typedef uint16_t rq_att_t;

const rq_att_t rq_atts[TK__elmax_] = {
	[TK_DDL]                  = RQA_DDL,
	[TK_alternatefor]         = RQA_alternatefor_extends,
	[TK_extends]              = RQA_alternatefor_extends,
	[TK_UUIDname]             = RQA_UUIDname,
	[TK_languageset]          = RQA_module,
	[TK_language]             = RQA_language,
	[TK_string]               = RQA_string,
	[TK_behaviorset]          = RQA_module,
	[TK_behaviordef]          = RQA_behaviordef,
	[TK_refines]              = RQA_refines,
	[TK_section]              = RQA_section,
	[TK_hd]                   = RQA_hd,
	[TK_p]                    = RQA_p,
	[TK_device]               = RQA_module,
	[TK_parameter]            = RQA_parameter,
	[TK_property]             = RQA_property,
	[TK_behavior]             = RQA_behavior,
	[TK_propertypointer]      = RQA_propertypointer,
	[TK_includedev]           = RQA_includedev,
	[TK_useprotocol]          = RQA_useprotocol,
	[TK_protocol]             = RQA_protocol,
	[TK_propref_DMP]          = RQA_propref_DMP,
	[TK_childrule_DMP]        = RQA_childrule_DMP,
	[TK_propmap_DMX]          = RQA_propmap_DMX,
	[TK_http_engarts_propext] = RQA_http_engarts_propext,
	[TK_choice]               = RQA_choice,
	[TK_label]                = RQA_label,
	[TK_mininclusive]         = RQA_mininclusive,
	[TK_maxinclusive]         = RQA_maxinclusive,
	[TK_refinement]           = RQA_refinement,
	[TK_setparam]             = RQA_setparam,
	[TK_value]                = RQA_value,
};
/**********************************************************************/
/*
Strings used in messages
*/
const ddlchar_t *ptypes[] = {
	[VT_NULL] = "NULL",
	[VT_imm_unknown] = "immediate",
	[VT_implied] = "implied",
	[VT_network] = "network",
	[VT_include] = "include",
	[VT_device] = "(sub)device",
	[VT_imm_uint] = "immediate uint",
	[VT_imm_sint] = "immediate sint",
	[VT_imm_float] = "immediate float",
	[VT_imm_string] = "immediate string",
	[VT_imm_object] = "immediate object",
};

#if ACNCFG_DDL_BEHAVIORS
const ddlchar_t *etypes[] = {
	[etype_unknown]      = "unknown type",
	[etype_boolean]      = "boolean",
	[etype_sint]      = "sint",
	[etype_uint]      = "uint",
	[etype_float]      = "float",
	[etype_UTF8]      = "UTF8",
	[etype_UTF16]      = "UTF16",
	[etype_UTF32]      = "UTF32",
	[etype_string]      = "string",
	[etype_enum]      = "enum",
	[etype_opaque]      = "opaque",
	[etype_UUID]      = "UUID",
	[etype_DCID]      = "DCID",
	[etype_CID]      = "CID",
	[etype_languagesetID]      = "languagesetID",
	[etype_behaviorsetID]      = "behaviorsetID",
	[etype_ISOdate]      = "ISOdate",
	[etype_URI]      = "URI",
	[etype_bitmap]      = "bitmap",
};
#endif

const char *pflgnames[pflg_COUNT] = {
	[pflgb_read] = "read",
	[pflgb_write] = "write",
	[pflgb_event] = "event",
	[pflgb_vsize] = "vsize",
	[pflgb_abs] = "abs",
#if ACNCFG_DDL_BEHAVIORS
	[pflgb_constant] = "constant",
	[pflgb_persistent] = "persistent",
	[pflgb_volatile] = "volatile",
	[pflgb_ordered] = "ordered",
	[pflgb_measure] = "measure",
	[pflgb_cyclic] = "cyclic",
#endif  /* ACNCFG_DDL_BEHAVIORS */
	[pflgb_packed] = "packed",
	[pflgb_overlap] = "overlap",
};

/* pflg_NAMELEN is sum of strlen(pflg_NAMES) */
#define pflg_NAMELEN 61

const ddlchar_t nulstr[] = {0};
/**********************************************************************/
/*
Global variables
*/
#if ACNCFG_DDL_STRINGS
struct uuidset_s languagesets;
#endif
#if ACNCFG_DDL_BEHAVIORS
struct uuidset_s behaviorsets;
#endif
struct uuidset_s devtrees;

struct pool_s permpool;
/**********************************************************************/
/*
Rather than parsing multiple modules recursively which can overload 
lightweight systems they are queued up (in the ddl parse context 
structure) to be parsed sequentially.
*/

struct qentry_s {
	struct qentry_s *next;
	tok_t modtype;
	void *ref;
	ddlchar_t name[];
};
/**********************************************************************/
typedef struct ppoint_s {
	struct ddlprop_s *prop;
	const ddlchar_t *path;
} ppoint_s;
/**********************************************************************/
/*
Prototypes
*/
void queue_module(struct dcxt_s *dcxp, tok_t modtype, 
				const ddlchar_t *name, void *ref);
static void check_queued_modulex(struct dcxt_s *dcxp, tok_t typefound, 
				const ddlchar_t *uuidstr, uint8_t *dcid);
/**********************************************************************/
/*
Token utilities
*/
/**********************************************************************/
static int
parenttok(struct dcxt_s *dcxp)
{
	if (dcxp->nestlvl <= 0) return -1;
	return dcxp->elestack[dcxp->nestlvl - 1];
}
/**********************************************************************/
/*
func: tokmatchtok
*/
static tok_t
tokmatchtok(const ddlchar_t *str, const struct allowtok_s *allowed)
{
	int ofs, span;
	const tok_t *tp;
	int cmp;

	if (allowed != NULL && (span = allowed->ntoks) > 0) {
		tp = allowed->toks;
		while (span) {
			tp += ofs = span / 2;
			if ((cmp = strcmp(str, tokstrs[*tp])) == 0) return *tp;
			if (cmp < 0) {
				tp -= ofs;
				span = ofs;
			} else {
				++tp;
				span -= ofs + 1;
			}
		}
	}
	return TK__none_;
}
/**********************************************************************/
/*
func: tokmatchofs
*/
static int
tokmatchofs(const ddlchar_t *str, const struct allowtok_s *allowed)
{
	int ofs, span;
	const tok_t *tp;
	int cmp;

	if (allowed != NULL && (span = allowed->ntoks) > 0) {
		tp = allowed->toks;
		while (span) {
			tp += ofs = span / 2;
			if ((cmp = strcmp(str, tokstrs[*tp])) == 0) return tp - allowed->toks;
			if (cmp < 0) {
				tp -= ofs;
				span = ofs;
			} else {
				++tp;
				span -= ofs + 1;
			}
		}
	}
	return -1;
}
/**********************************************************************/
/*
Misc text utilities
*/
/**********************************************************************/
/*
check the format of an "object" as contained in a value field
return its size
*/

static int
objlen(const ddlchar_t *str)
{
	int i;

	i = 0;

	while (*str) {
		if (hexdigs[(uint8_t)*str++] >= 0) ++i;
	}
	if ((i & 1)) return -1;
	return i/2;
}
/**********************************************************************/
static int
savestrasobj(const ddlchar_t *str, uint8_t **objp)
{
	uint8_t *cp;
	int len;
	int byte;
	int nibble;

	if ((len = objlen(str)) < 0) return len;
	if ((cp = acnalloc(len)) == NULL) return -1;

	*objp = cp;
	byte = 1;  /* bit 0 is shift marker */

	while (*str) {
		nibble = hexdigs[(uint8_t)*str++];
		if (nibble >= 0) {
			byte = (byte << 4) | nibble;
			if (byte >= 0x100) {
				*cp++ = (uint8_t)byte;
				byte = 1;  /* restore shift marker */
			}
		} else if (nibble == c_BAD) {
			return -1;
		}
	}
	assert(cp - *objp == len);
	return len;
}
/**********************************************************************/
/*
func: flagnames
Utility function that prints the names of those flags which are set in a word
into a string.
*/
char *
flagnames(uint32_t flags, const char **names, char *buf, const char *format)
{
	char *dp;
	int i;

	dp = buf;
	*dp = 0;
	for (;flags != 0; flags >>= 1, ++names) if (flags & 1) {
		i = sprintf(dp, format, *names);
		dp += i;
	}
	return buf;
}
/**********************************************************************/
/*
func: resolveuuid

Resolve a UUID alias to a full UUID using currently scoped UUIDname 
definitions.
*/
const ddlchar_t *
resolveuuid(struct dcxt_s *dcxp, const ddlchar_t *name, uint8_t *dcid)
{
	struct uuidalias_s *ap;

LOG_FSTART();
	/* if it is a properly formatted string, just convert it */
	if (str2uuid(name, dcid) == 0) return name;
	/* otherwise try for an alias */
	for (ap = dcxp->aliases; ap < dcxp->aliases + dcxp->naliases; ++ap) {
		if (strcmp(ap->alias, name) == 0) {
			if (dcid) str2uuid(ap->uuidstr, dcid);
			LOG_FEND();
			return ap->uuidstr;
		}
	}
	acnlogmark(lgERR, "Can't resolve UUID \"%s\"", name);
	return NULL;
}
/**********************************************************************/
/*

*/
static int
getboolatt(const ddlchar_t *str)
{
	if (!str || strcmp(str, "false") == 0) return 0;
	if (strcmp(str, "true") == 0) return 1;
	acnlogmark(lgERR, "     bad boolean \"%s\"", str);
	return 0;
}
/**********************************************************************/
/*

*/
static int
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
		acnlogmark(lgERR,
				"     bad format, expected unsigned int, got \"%s\"", str);
	}
	return -1;
}
/**********************************************************************/
/*

*/
static int
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
/*
Behaviorset utilities
*/
/**********************************************************************/
#if ACNCFG_DDL_BEHAVIORS
/*
func: findbv

Find the struct bv_s corresponding to a behavior in a set.

We use standard acn UUID search to find the set structure, then search
its hash table for the name.
*/
const struct bv_s *
findbv(const uint8_t *uuid, const ddlchar_t *name, struct bvset_s **bvset)
{
	struct bvset_s *set;

	set = (struct bvset_s *)finduuid(&behaviorsets, uuid);
	if (bvset) *bvset = set;

	if (set == NULL) return NULL;
	return (struct bv_s *)findkey(&set->hasht, name);
}
/**********************************************************************/
/*
func: init_behaviors

Add our defined behaviorsets to the kbehaviors structure where we 
can find them quickly.
*/
static void
init_behaviors(struct dcxt_s *dcxp)
{
	struct bv_s *kbv;
	struct bvset_s *set;
	static unsigned int nbvs = 0;
	size_t create;

	if (nbvs) return;

	set = NULL;
	for (kbv = known_bvs; kbv->name; ++kbv) {
		if (kbv->action == NULL) {
			/* UUID entry: sets the behaviorset for subsequent names */
			uint8_t uuid[UUID_SIZE];

			if (str2uuid(kbv->name, uuid) < 0) {
				acnlogmark(lgWARN, "Register behaviorset: bad format %s", kbv->name);
				set = NULL;
				continue;
			}
			create = sizeof(struct bvset_s);
			set = (struct bvset_s *)findornewuuid(&behaviorsets, uuid, &create);
			if (set == NULL) {
				acnlogmark(lgWARN, "Out of memory");
				continue;
			}
			if (create == 0) {
				/*
				new set - need to parse it
				*/
				set->hasht.pool = &permpool;
				queue_module(dcxp, TK_behaviorset, kbv->name, set);
			}
		} else if (set == NULL) {
			acnlogmark(lgNTCE, "Behavior \"%s\": unspecified behaviorset", kbv->name);
		} else {
			switch (addkey(&set->hasht, &kbv->name)) {
			case 0:
				++nbvs;
				break;
			case KEY_ALREADY:
				acnlogmark(lgNTCE, "Duplicate behavior \"%s\" ignored", kbv->name);
				break;
			case KEY_NOMEM:
			default:
				acnlogmark(lgWARN, "Out of memory");
				break;
			}
		}
	}
	acnlogmark(lgDBUG, "     Registered %u behaviors", nbvs);
}
/**********************************************************************/
#if acntestlog(lgINFO)
static void
showbsets(void)
{
	char dcidstr[UUID_STR_SIZE];
	struct bvset_s *set;

	acnlogmark(lgINFO, "Behaviorsets accumulated:");
	FOR_EACH_UUID(&behaviorsets, set, struct bvset_s, uuid[0]) {	
		acnlogmark(lgINFO, "  %s", uuid2str(set->uuid, dcidstr));
	} NEXT_UUID()
}
#else
#define showbsets()
#endif
#endif  /* ACNCFG_DDL_BEHAVIORS */
/**********************************************************************/
/*
languageset utilities
*/
/**********************************************************************/
#if ACNCFG_DDL_STRINGS
/*
func: setlang
*/
void
setlang(const ddlchar_t **ltags)
{
	struct lset_s *set;
	struct language_s *lng;
	const ddlchar_t **lp;
	int l;
#if acntestlog(lgINFO)
	char dcidstr[UUID_STR_SIZE];
#endif	

	FOR_EACH_UUID(&languagesets, set, struct lset_s, uuid[0]) {
		assert(set->nlangs);
		set->userlang = 0;  /* default */
		lng = set->languages;
		for (lp = ltags; *lp; ++lp) {
			for (l = 0; l < set->nlangs; ++l) {
				if (strcmp(*lp, lng[l].tag) == 0) {
					set->userlang = l;
					goto nextset;
				}
			}
		}
#if acntestlog(lgINFO)
		acnlogmark(lgINFO, "Languageset %s does not provide requested language(s)", 
			uuid2str(set->uuid, dcidstr));
#endif	
nextset: ;
	} NEXT_UUID();
}
/**********************************************************************/

/*
func: findlabel
*/
const ddlchar_t *
lblookup(struct label_s *lbl)
{
	struct string_s *str;
	struct lset_s *set;

	if ((set = lbl->set) == NULL) return lbl->txt;
	str = (struct string_s *)findkey(&set->hasht, lbl->txt);
	if (str == NULL) return NULL;
	if (set->userlang < 0 || set->userlang >= set->nlangs)
		set->userlang = 0;
	return str->text[set->userlang];
}
#endif  /* ACNCFG_DDL_STRINGS */
/**********************************************************************/
/*
DDL property tree utilities
*/
/**********************************************************************/
struct ddlprop_s *
itsdevice(struct ddlprop_s *prop)
{
	while (prop && prop->vtype != VT_device) prop = prop->parent;
	return prop;
}
/**********************************************************************/
static int
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
/* includes terminator */
#define MAXPROPNAME 100
static char pnbuf[MAXPROPNAME];
/*
recursive part of propname()
*/

static int
_propname(struct ddlprop_s *pp, enum pname_flags_e flags, int ofs)
{
	const char *cp;
	int nlen;

	if (flags & pn_path && pp->parent) {
		struct ddlprop_s *dev;
		dev = itsdevice(pp->parent);
		if (dev->parent) {
			ofs = _propname(dev, flags, ofs);
			pnbuf[ofs++] = (flags & pn_translate) ? '_' : '/';
		}
	}
	cp = NULL;
	if (pp->id) {
		cp = pp->id + sizeof(pp->pnum);
		nlen = strlen(cp);
	} else {
		unsigned int u = pp->pnum;
		nlen = ((pp->vtype == VT_device) ? 4 : 2);
		while ((u /= 10)) ++nlen;
	}
	if (nlen > (MAXPROPNAME - 3)) {
		cp += nlen - (MAXPROPNAME - 1);
		nlen = MAXPROPNAME - 1;
		ofs = 0;
	} else if (nlen > (MAXPROPNAME - ofs - 1)) {
		ofs = MAXPROPNAME - nlen - 3;
		if (flags & pn_translate) {
			pnbuf[ofs++] = '_'; pnbuf[ofs++] = '_';
		} else {
			pnbuf[ofs++] = '.'; pnbuf[ofs++] = '.';
		}
	}
	if (cp) {
		/*
		copy the string
		*/
		if (flags & pn_translate) {
			for (; *cp; ++cp) {
				pnbuf[ofs++] = isalnum(*cp) ? *cp : '_';
			}
		} else {
			memcpy(pnbuf + ofs, cp, nlen);
			ofs += nlen;
		}
		pnbuf[ofs] = 0;
	} else {
		ofs += sprintf(pnbuf + ofs, 
				((pp->vtype == VT_device) ? "sub%u" : "p%u"), pp->pnum);
	}
	//acnlogmark(lgDBUG, "propname: %s", buf);
	return ofs;
}
/**********************************************************************/
/*
func: propname

Generate a property name derived from its xml ID if it has one, 
otherwise from its property number which is its position in document 
order.

The flags argument can include.

	pn_translate - translate characters so resulting name is a valid
	C identifier (this is used by <mapgen>).

	pn_path - include the names of ancestor devices in the name which 
	becomes the path to the property from the root device and 
	guarantees it will be unique within the whole device.

returns:
	A pointer to the property name which is in a statically allocated 
	buffer and will be overwritten in the next call.
*/
const char *
propname(struct ddlprop_s *pp, enum pname_flags_e flags)
{
	_propname(pp, flags, 0);
	return pnbuf;
}
/**********************************************************************/
/*
*/
static void
addpropID(struct dcxt_s *dcxp, const ddlchar_t *propID)
{
	struct ddlprop_s *dev;
	struct ddlprop_s *pp;
	const ddlchar_t *id;

	pp = dcxp->m.dev.curprop;
	/* don't overwrite incdev ID with device ID */
	if (pp->id != NULL) return;

	dev = itsdevice(pp);

	pool_appendn(&dcxp->rootdev->strpool, (const ddlchar_t *)&dev->pnum,
			sizeof(dev->pnum));
	id = pool_addstr(&dcxp->rootdev->strpool, propID);

	acnlogmark(lgDBUG, "addpropID %s to %s", id, propxpath(pp));
	pp->id = id;
	addkey(&dcxp->rootdev->idtab, &pp->id);
}
/**********************************************************************/
/*
func: add_devtask

Add a task (during parsing) for execution once the entire tree has been
constructed but before returning it to the caller.

This is useful for behavior actions which need to reference child 
properties (which have notyet been parsed at the time the initial 
bahevior action is executed) or other parts of the tree. It is also 
used to implement propertypointers which need the entire tree to 
navigate arbitrary reference paths.

arguments:

dcxp - the current parse context, includes the information needed to
queue the task.

task - a pointer to the task function to be called back when the 
tree is complete.

size - add_devtask() creates a task specific area of this size which 
may be used to store any necessary data needed to execute the task. 
This task data area is initialized to zero returned to the caller to 
be filled with whatever data is required. The data is then passed to 
the task function when the task is executed. This task-data area is 
automatically freed after execution of the task.

returns: a pointer to the task data area or NULL if there is not 
enough memory to create and queue the task.
*/

void *
add_devtask(struct dcxt_s *dcxp, devtask_fn *task, size_t size)
{
	struct devtask_s *tp;

	LOG_FSTART();
	if (task == NULL
		|| !(tp = (struct devtask_s *)acnalloc(
						offsetof(struct devtask_s, taskdata[0]) + size)))
	{
		acnlogmark(lgWARN, "Add task fail");
		return NULL;
	}
	if (size) memset(tp->taskdata, 0, size);
	tp->task = task;
	tp->nxt = dcxp->tasks;
	dcxp->tasks = tp;
	return (void *)tp->taskdata;
	LOG_FEND();
}
/**********************************************************************/
/*
utility function to grow the address map
*/
static uint8_t *
growmap(union addrmap_u *amap)
{
	uint8_t *mp;

	LOG_FSTART();
	amap->any.size += AFMAPINC;

	if ((mp = realloc(amap->any.map, amap->any.size)) == NULL) {
		acnlogerror(LOG_ON | LOG_CRIT);
		exit(EXIT_FAILURE);
	}
	LOG_FEND();
	return amap->any.map = mp;
}

#define growsrchmap(amap) ((struct addrfind_s *)growmap(amap))
/**********************************************************************/
/*
reverse the order of a property list
*/
static struct ddlprop_s *
reverseproplist(struct ddlprop_s *plist) {
	struct ddlprop_s *tp;
	struct ddlprop_s *rlist = NULL;

	while ((tp = plist) != NULL) {
		plist = tp->siblings;
		tp->siblings = rlist;
		rlist = tp;
	}
	return rlist;
}
/**********************************************************************/
#define MAXPATHSTEP 32
/*
func: ddlref

Trace a DDL property cross reference using xml IDs and the syntax specified in
the DDL spec.

arguments:

root - the root device structure

pp - pointer to the starting property (usually the one containing or
specifying the reference.

path - the string specifying the path.
*/
struct ddlprop_s *
ddlref(struct rootdev_s *root, struct ddlprop_s *pp,
	const ddlchar_t *path)
{
	union {
		uint16_t sdn;
		ddlchar_t str[MAXPATHSTEP];
	} scratch;
	const ddlchar_t *sp;
	ddlchar_t *dp;
	const ddlchar_t **idp;

	acnlogmark(lgDBUG, "resolve ddlref %s from %s",
			path, propxpath(pp));
	if ((sp = path) == NULL) return NULL;
	if (*sp == 0) return pp;

	if (*sp == '/') {  /* absolute path */
		pp = root->ddlroot;
		++sp;
	} else {
		pp = itsdevice(pp);
	}
	acnlogmark(lgDBUG, "start at %s", propxpath(pp));
	while (*sp) {
		/* FIXME: need to handle array instances. */
		if (pp->vtype != VT_device) {
			acnlogmark(lgWARN, 
					"ddlref %s, bad step (not a device), using parent device",
					path);
			pp = itsdevice(pp);
		}
		if (*sp == '/') {
			++sp;
			continue;
		}
		if (sp[0] == '.') {
			switch (sp[1]) {
			case 0:
			case '/':
				acnlogmark(lgDBUG, "stay at %s", propxpath(pp));
				++sp;
				continue;
			case '.':
				if (sp[2] == 0 || sp[2] == '/') {
					if (pp->parent == NULL) return NULL;
					pp = itsdevice(pp->parent);
					acnlogmark(lgDBUG, "up to %s", propxpath(pp));
					sp += 2;
					continue;
				}
				break;
			default:
				break;
			}
		}
		/* got a real step */
		scratch.sdn = pp->pnum;
		dp = scratch.str + sizeof(scratch.sdn);
		while (*sp && *sp != '/') {
			if (dp >= scratch.str + MAXPATHSTEP) return NULL;
			*dp++ = *sp++;
		}
		*dp = 0;  /* terminate */
		acnlogmark(lgDBUG, "step is %s", scratch.str);
		if ((idp = findkey(&root->idtab, scratch.str)) == NULL) {
			acnlogmark(lgDBUG, "not found");
			return NULL;
		}
		pp = container_of(idp, struct ddlprop_s, id);
		acnlogmark(lgDBUG, "move to %s", propxpath(pp));
	}
	return pp;
}
/**********************************************************************/
/*
func: 
*/
static void
resolvepp(struct rootdev_s *root, void *tdat)
{
	struct ppoint_s *ppt = (struct ppoint_s *)tdat;
	struct ddlprop_s *pp;
	struct ddlprop_s *tgt;

	pp = ppt->prop;
	tgt = ddlref(root, pp, ppt->path);
	if (tgt == NULL) {
		acnlogmark(lgERR, "property pointer %s does not resolve", ppt->path);
	} else {
		/* FIXME: need to check pointer matches array spec etc. */
	}
	pp->v.alias = tgt;
}
/**********************************************************************/
/*
XML element handlers
*/
/**********************************************************************/
/*
callback to accumulate element text
*/
static void
elem_text(void *data, const ddlchar_t *txt, int len)
{
	struct dcxt_s *dcxp = (struct dcxt_s *)data;
	int space;

	LOG_FSTART();
	space = sizeof(dcxp->txt.ch) - dcxp->txtlen - 1;
	if (len > space) {
		len = space;
		acnlogmark(lgERR, "%4d value text too long - truncating", dcxp->elcount);
	}
	memcpy(dcxp->txt.ch + dcxp->txtlen, txt, len);
	dcxp->txtlen += len;
	LOG_FEND();
}
/**********************************************************************/
/*
start accumulating element content

Before calling this, element text is ignored.
*/
static void
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
/*
stop accumulating element content

Any accumulated text is returned to the caller.
Subsequent text is ignored
*/
static const ddlchar_t *
endText(struct dcxt_s *dcxp)
{
	if (dcxp->txtlen == -1) return dcxp->txt.p;

	XML_SetCharacterDataHandler(dcxp->parser, NULL);
	dcxp->txt.ch[dcxp->txtlen] = 0;  /* terminate */
	return dcxp->txt.ch;
}
/**********************************************************************/
/*
Record a UUID alias.

It would be simple to enter the short names in a hash table for fast
lookup but typically there are well less than ten names in a module
so they are stored in a simple list a linear search is fast and simple.
*/
static void
alias_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	struct uuidalias_s *ap;
	const ddlchar_t *aliasname;
	const ddlchar_t *aliasuuid;

	LOG_FSTART();
	/* handle attributes */
	aliasname = atta[2];
	aliasuuid = atta[0];

	if (aliasname == NULL || aliasuuid == NULL) {
		acnlogmark(lgERR, "%4d UUIDname missing attribute(s)", dcxp->elcount);
		return;
	}
	if (str2uuid(aliasuuid, NULL) < 0) {
		acnlogmark(lgERR, "%4d UUIDname bad format: %s", dcxp->elcount, aliasuuid);
		return;
	}
	if (dcxp->naliases >= MAXALIASES) {
		acnlogmark(lgERR, "%4d UUIDname list full", dcxp->elcount);
		return;
	}
	ap = dcxp->aliases + dcxp->naliases++;
	ap->alias = pool_addstr(&dcxp->modulepool, aliasname);
	ap->uuidstr = pool_addstr(&dcxp->modulepool, aliasuuid);
	LOG_FEND();
}
/**********************************************************************/
static void
label_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	struct ddlprop_s *pp;
	uint8_t dcid[UUID_SIZE];
	struct lset_s *set;
	ddlchar_t uuidstr[UUID_STR_SIZE + 1];
	size_t create;

	const ddlchar_t *setname = atta[4];
	const ddlchar_t *keyp = atta[1];
	const ddlchar_t *contentparam = atta[3];

	LOG_FSTART();
	switch (parenttok(dcxp)) {  /* labels can occur on multiple elements */
	case TK_property:
	case TK_includedev:
	case TK_device:
		pp = dcxp->m.dev.curprop;
		if (pp->label.txt && parenttok(dcxp) != TK_device) {
			acnlogmark(lgERR, "%4d multiple labels on property (%s + %s)", dcxp->elcount, pp->label.txt, keyp);
		} else switch ((setname != NULL) + (keyp != NULL)) {
		case 1:  /* error - must have both set and key or neither */
			acnlogmark(lgERR, "%4d label must have both @set and @key or neither",
						dcxp->elcount);
			break;
		case 0:  /* literal label */
			break;
		case 2:
			if ((setname = resolveuuid(dcxp, setname, dcid)) == NULL) {
				SKIPON(dcxp);
				return;
			}
			create = sizeof(struct lset_s);
			set = (struct lset_s *)findornewuuid(&languagesets, dcid, &create);
			if (set == NULL) {
				acnlogmark(lgERR, "out of memory");
				break;
			}
			if (create == 0) {
				set->hasht.pool = &permpool;
				queue_module(dcxp, TK_languageset, setname, set);
			}
			pp->label.set = set;
			pp->label.txt = pool_addstr(&dcxp->rootdev->strpool, keyp);
			acnlogmark(lgDBUG, "%4d label set=%s key=\"%s\"",
						dcxp->elcount, uuid2str(set->uuid, uuidstr), 
						pp->label.txt);
			break;
		}
		break;
	case TK_languageset:
	case TK_language:
	case TK_behaviorset:
	case TK_behaviordef:
	case TK_parameter:
	default:
		break;
	}
	startText(dcxp, contentparam);
	LOG_FEND();
}
/**********************************************************************/
static void
label_end(struct dcxt_s *dcxp)
{
	const ddlchar_t *ltext;
	struct ddlprop_s *pp;

	LOG_FSTART();
	ltext = endText(dcxp);

	switch (parenttok(dcxp)) {  /* labels can occur on multiple elements */
	case TK_property:
	case TK_includedev:
	case TK_device:
		pp = dcxp->m.dev.curprop;
		if (pp->label.set == NULL) {
			if (!pp->label.txt) {
				acnlogmark(lgDBUG, "%4d literal label \"%s\"", dcxp->elcount, ltext);
				pp->label.txt = pool_addstr(&dcxp->rootdev->strpool, ltext);
			}
		} else {
			if (*ltext != 0) {
				acnlogmark(lgERR, "%4d text provided to label by reference",
							dcxp->elcount);
			}
		}
		break;
	case TK_languageset:
	case TK_language:
	case TK_behaviorset:
	case TK_behaviordef:
	case TK_parameter:
	default:
		break;
	}
	LOG_FEND();
}
/**********************************************************************/
/*
Behaviorsets
*/
/**********************************************************************/
/*
func: bset_start

Process a behaviorset.
*/
static void
bset_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	const ddlchar_t *uuidp = atta[0];
	struct bvset_s *set;
	uint8_t dcid[UUID_SIZE];

	LOG_FSTART();
	check_queued_modulex(dcxp, TK_behaviorset, uuidp, dcid);
	set = (struct bvset_s *)dcxp->queuehead->ref;
	if (set == NULL) {
		size_t create = sizeof(struct bvset_s);
		set = (struct bvset_s *)findornewuuid(&behaviorsets, dcid, &create);
		if (create == 0) {
			set->hasht.pool = &permpool;
		} else {
			/*
			This only applies where queuehead->ref is NULL. For
			queue entries from init_behaviors we will parse anyway
			*/
			acnlogmark(lgNTCE, "Not re-parsing behaviorset %s", uuidp);
			SKIPON(dcxp);
		}
	}
	dcxp->m.bset.curset = set;
	LOG_FEND();
}
/**********************************************************************/
/*
func: bset_start

Process a behaviorset.
*/
static void
bset_end(struct dcxt_s *dcxp)
{
	struct bvset_s *set;
	struct bv_s *bv;
	struct bv_s **bva;
	int i;
	ddlchar_t dstr[UUID_STR_SIZE];

	LOG_FSTART();
	set = dcxp->m.bset.curset;
	uuid2str(set->uuid, dstr);
	bva = (struct bv_s **)set->hasht.v;
	if (bva == NULL) {
		acnlogmark(lgWARN, "%4d no behaviors found in set", dcxp->elcount);
	} else {
		i = 1 << set->hasht.power;

		while (i--) if ((bv = bva[i]) != NULL) {
			if (bv->nrefines == BV_NEW) {
				assert(bv->action == NULL);
				acnlogmark(lgERR, "%4d no definition for %s in set %s", 
						dcxp->elcount, bv->name, dstr);
				bv->nrefines = BV_NULL;
			}
		}
	}
	set->parsed = 1;
	LOG_FEND();
}
/**********************************************************************/
static void
bvdef_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	struct bvset_s *set;
	struct bv_s *bv;
	size_t create;

	const ddlchar_t *namep = atta[1];

	set = dcxp->m.bset.curset;
	create = sizeof(struct bv_s);
	bv = (struct bv_s *)findornewkey(&set->hasht, namep, &create);
	if (bv == NULL) {
		acnlogmark(lgERR, "%4d out of memory. skipping...", dcxp->elcount);
		SKIPON(dcxp);
		return;
	}
	if (create == 0) {
		bv->nrefines = BV_NEW;
	} else {
		if (bv->nrefines > 0) {
			acnlogmark(lgERR, "%4d redefinition of behavior %s. skipping...", 
							dcxp->elcount, namep);
			SKIPON(dcxp);
		} else if (bv->nrefines == BV_FINAL) {
			acnlogmark(lgDBUG, "%4d skipping finalized behavior %s...", 
							dcxp->elcount, namep);
			SKIPON(dcxp);
		}
	}
	dcxp->m.bset.curbv = bv;
	dcxp->m.bset.nrefines = 0;
}
/**********************************************************************/
static void
bvdef_end(struct dcxt_s *dcxp)
{
	struct bv_s *bv;

	bv = dcxp->m.bset.curbv;

	if (dcxp->m.bset.nrefines) {
		struct bv_s **bva;
		int i = dcxp->m.bset.nrefines;

		bva = (struct bv_s **)acnalloc(i * sizeof(struct bv_s *));
		if (bva == NULL) {
			acnlogmark(lgERR, "%4d out of memory", dcxp->elcount);
			return;
		}
		memcpy(bva, dcxp->m.bset.refines, i * sizeof(struct bv_s *));
		bv->refa = (void *)bva;
		bv->nrefines = i;
	} else if (bv->action == NULL) {
		bv->nrefines = BV_NULL;
	}
}
/**********************************************************************/
static void
refines_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	struct bv_s *bv;
	struct bvset_s *set;
	const ddlchar_t *dcidstr;
	uint8_t dcid[UUID_SIZE];
	size_t create;

	const ddlchar_t *name = atta[1];
	const ddlchar_t *setp = atta[2];

	LOG_FSTART();
	if ((dcidstr = resolveuuid(dcxp, setp, dcid)) == NULL) {
		SKIPON(dcxp);
		return;
	}
	create = sizeof(struct bvset_s);
	set = (struct bvset_s *)findornewuuid(&behaviorsets, dcid, &create);
	if (set == NULL) {
		acnlogmark(lgERR, "%4d out of memory", dcxp->elcount);
		return;
	}
	if (create == 0) {
		/*
		new set - need to parse it
		*/
		set->hasht.pool = &permpool;
		queue_module(dcxp, TK_behaviorset, dcidstr, set);
	}
	if (set->parsed == true) {
		bv = (struct bv_s *)findkey(&set->hasht, name);
		if (bv == NULL) {
			acnlogmark(lgERR, "%4d No behavior %s in set %s",
						dcxp->elcount, name, setp);
		} else if (bv->nrefines == BV_NULL) {
			bv = NULL;
		}
	} else {
		create = sizeof(struct bv_s);
		bv = (struct bv_s *)findornewkey(&set->hasht, name, &create);
		if (bv == NULL) {
			acnlogmark(lgERR, "%4d out of memory", dcxp->elcount);
		} else if (create == 0) {
			bv->nrefines = BV_NEW;
		}
	}
	if (bv) {
		if (dcxp->m.bset.nrefines >= BV_MAXREFINES) {
			acnlogmark(lgERR, "%4d behaviordef has more than maximum (%u) refinements", dcxp->elcount, BV_MAXREFINES);
		} else {
			dcxp->m.bset.refines[dcxp->m.bset.nrefines++] = bv;
		}
	}
	LOG_FEND();
}
/**********************************************************************/
/*
Recursively implement refinements then the current behavior
*/
static void
execbv(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	int i;

	LOG_FSTART();
	// acnlogmark(lgDBUG, "     exec bv %s", bv->name);
	switch (bv->nrefines) {
	case BV_NEW:
		acnlogmark(lgERR, "%4d undefined behavior %s", dcxp->elcount, bv->name);
		/* fall through */
	case BV_NULL:
		LOG_FEND();
		return;
	case BV_FINAL:
	case 0:
		break;
	default:
		for (i = 0; i < bv->nrefines; ++i)
			execbv(dcxp, bv->refa[i]);
		break;
	}
	if (bv->action) {
		acnlogmark(lgDBUG, "     bv action %s", bv->name);
		(*bv->action)(dcxp, bv);
	}
	LOG_FEND();
}
/**********************************************************************/
/*
Languagesets
*/
/**********************************************************************/
/*
func: lset_start

Process a languageset.
This is currently just a stub that skips the entire set.
*/
static void
lset_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	const ddlchar_t *uuidp = atta[0];
	struct lset_s *set;
	uint8_t dcid[UUID_SIZE];

	LOG_FSTART();
	check_queued_modulex(dcxp, TK_languageset, uuidp, dcid);
	set = (struct lset_s *)dcxp->queuehead->ref;
	if (set == NULL) {
		size_t create = sizeof(struct lset_s);

		set = (struct lset_s *)findornewuuid(&languagesets, dcid, &create);
		if (set == NULL) {
			acnlogmark(lgERR, "Out of memory, skipping...");
			SKIPON(dcxp);
			return;
		}
		if (create == 0) set->hasht.pool = &permpool;
	}
	if (set->hasht.v) {
		acnlogmark(lgNTCE, "Not re-parsing languageset %s", uuidp);
		SKIPON(dcxp);
	}
	dcxp->m.lset.curset = set;
	LOG_FEND();
}
/**********************************************************************/
/*
func: lset_end

We've accumulated the full language array in dcxp. Now we know
how many languages and keys are in the set we can consolidate.
Allocate all our strings and languages in one big block.
We can then free the rather wasteful string structures and
the languages are part of dxcp which gets wiped for the next module.
*/
static void
lset_end(struct dcxt_s *dcxp)
{
	int nkeys;
	int nlangs;
	int i;
	struct lset_s *set;
	struct string_s *strings;
	struct language_s *langs;
	int stroffs, strsize;
	char dcid[UUID_STR_SIZE];

	LOG_FSTART();
	set = dcxp->m.lset.curset;
	nkeys = dcxp->m.lset.nkeys;
	set->nlangs = nlangs = dcxp->m.lset.nlangs;
	acnlogmark(lgINFO, "End languageset %s\n  %d languages, %d strings.", 
			uuid2str(set->uuid, dcid), nlangs, nkeys);
	stroffs = sizeof(struct language_s) * nlangs;
	strsize = sizeof(ddlchar_t *) * (nlangs + 1);
	langs = (struct language_s *)acnalloc(stroffs + strsize * nkeys);
	if (langs == NULL) {
		/* FIXME */
		exit(EXIT_FAILURE);
	}
	//acnlogmark(lgDBUG, "%d allocated (%d for languages), copying", stroffs + strsize * nkeys, stroffs);
	memcpy(langs, dcxp->m.lset.languages, stroffs);
	strings = (struct string_s *)(((char *)langs) + stroffs);
	//acnlogmark(lgDBUG, "revising hashtab size %d", (1 << set->hasht.power));
	for (i = 0; i < (1 << set->hasht.power); ++i) {
		struct string_s *he;
		int lang;
		he = (struct string_s *)set->hasht.v[i];
		if (he) {
			strings->key = he->key;  /* copy the key */
			for (lang = 0; lang < nlangs; ++lang) {
				int altl = lang;
				const ddlchar_t *cp;

				while ((cp = he->text[altl]) == NULL) {
					altl = langs[altl].altlang;
					if (altl == NO_LANG) {
						acnlogmark(lgERR, "No definition for %s in %s or its alternatives", 
								he->key, langs[lang].tag);
						break;
					}
				}
				strings->text[lang] = cp;
			}
			set->hasht.v[i] = (const ddlchar_t **)strings;
			free(he);
			strings = (struct string_s *)(&strings->text[nlangs]);
		}
	}
	set->languages = langs;
	LOG_FEND();
}
/**********************************************************************/
static void
language_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	int i;
	int this, alt;
	int needed;

	const ddlchar_t *altag = atta[0];
	const ddlchar_t *ltag = atta[2];


	LOG_FSTART();
	this = alt = -1;

	/* have we already created it from an altlang? */
	acnlogmark(lgDBUG, "Define language %s, fallback %s", ltag, 
				altag ? altag : "(none)");
	needed = altag ? 2 : 1;
	for (i = 0; i < dcxp->m.lset.nlangs; ++i) {
		if (strcmp(ltag, dcxp->m.lset.languages[i].tag) == 0) {
			this = i;
			if (--needed == 0) break;
		}
		if (altag && strcmp(altag, dcxp->m.lset.languages[i].tag) == 0) {
			alt = i;
			if (--needed == 0) break;
		}
	}
	if (dcxp->m.lset.nlangs + needed >= LSET_MAXLANGS) {
		acnlogmark(lgWARN, "Maximum languages exceeded, skipping %s", ltag);
		SKIPON(dcxp);
		return;
	}
	if (this < 0) {
		acnlogmark(lgDBUG, "New language %s", ltag);
		this = dcxp->m.lset.nlangs++;
		dcxp->m.lset.languages[this].tag = pool_addstr(&permpool, ltag);
	}
	if (altag && alt < 0) {
		acnlogmark(lgDBUG, "Infer language %s", altag);
		alt = dcxp->m.lset.nlangs++;
		dcxp->m.lset.languages[alt].tag = pool_addstr(&permpool, altag);
	}
	acnlogmark(lgDBUG, "%s at index %d", ltag, this);
	dcxp->m.lset.curlang = (int16_t)this;
	if (altag) {
		acnlogmark(lgDBUG, "%s at index %d", altag, alt);
		dcxp->m.lset.languages[this].altlang = (int16_t)alt;
	}
	LOG_FEND();
}
/**********************************************************************/
static void
string_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	struct string_s *str;
	struct lset_s *set;
	int lang;
	size_t create;
#if ACNCFG_STR_FOLDSPACE
	const ddlchar_t *folds = atta[1];
	const ddlchar_t *key = atta[2];
#else
	const ddlchar_t *key = atta[1];
#endif

	LOG_FSTART();
	set = dcxp->m.lset.curset;
	lang = dcxp->m.lset.curlang;

#if ACNCFG_STR_FOLDSPACE
	dcxp->m.lset.foldsp = !(folds && strcmp(folds, "preserve") == 0);
#endif

	//acnlogmark(lgDBUG, "String %s/%s", dcxp->m.lset.languages[lang].tag, key);

	create = sizeof(struct string_s);
	str = (struct string_s *)findornewkey(&set->hasht, key, &create);
	if (str == NULL) {
		acnlogmark(lgERR, "Out of memory, skipping...");
		SKIPON(dcxp);
		return;
	}
	if (create == 0) ++dcxp->m.lset.nkeys;
	if (str->text[lang]) {
		acnlogmark(lgWARN, "Duplicate strings: key=%s lang=%s text=%s",
					key, dcxp->m.lset.languages[lang].tag, str->text[lang]);
		SKIPON(dcxp);
	} else {
		dcxp->m.lset.curstr = str;
		++dcxp->m.lset.languages[lang].nkeys;
		startText(dcxp, NULL);
	}
	LOG_FEND();
}
/**********************************************************************/
static void
string_end(struct dcxt_s *dcxp)
{
	struct string_s *str;
	int lang;
	const ddlchar_t *ltext;

	LOG_FSTART();
	str = dcxp->m.lset.curstr;
	lang = dcxp->m.lset.curlang;
	ltext = endText(dcxp);
	if (*ltext == 0) ltext = nulstr;
#if ACNCFG_STR_FOLDSPACE
	else if (dcxp->m.lset.foldsp) {
		acnlogmark(lgDBUG, "String \"%s\"", ltext);
		ltext = pool_addfoldsp(&permpool, ltext);
		acnlogmark(lgDBUG, "Folded \"%s\"", ltext);
	}
#endif
	else ltext = pool_addstr(&permpool, ltext);

	str->text[lang] = ltext;
	LOG_FEND();
}
/**********************************************************************/
/*
Devices
*/
/**********************************************************************/
static void
dev_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	struct ddlprop_s *pp;
	const ddlchar_t *uuidp = atta[0];
	const ddlchar_t *idp = atta[2];
	uint8_t dcid[UUID_SIZE];

	LOG_FSTART();
	check_queued_modulex(dcxp, TK_device, uuidp, dcid);

	pp = (struct ddlprop_s *)dcxp->queuehead->ref;
	if (pp == NULL) {  /* nowhere to put it - must be root device */
		struct rootdev_s *dev;

		assert(dcxp->rootdev == NULL);
		dcxp->rootdev = dev = acnNew(struct rootdev_s);
		dev->minaddr = 0xffffffff;

		/* create a device ddlprop_s */
		dev->ddlroot = pp = acnNew(struct ddlprop_s);
		pp->vtype = VT_device;
		dcxp->arraytotal = pp->array = 1;
		/* and an address map */
		dev->amap = acnNew(union addrmap_u);
		dev->amap->any.type = am_srch;
		dev->amap->any.map = mallocx(AFMAPISIZE);
		dev->amap->any.size = AFMAPISIZE;
		uuidcpy(dev->dcid, dcid);
		uuidcpy(dev->amap->any.dcid, dcid);
		/* subdevno is used for hashing IDs. No byte can be zero */
		pp->pnum =  0x101;
		dcxp->m.dev.subdevno = 0x102;		
	} else {
		assert(pp->vtype == VT_include);
		pp->vtype = VT_device;
	}
	uuidcpy(pp->v.dev.dcid, dcid);
	dcxp->m.dev.curprop = pp;
	if (idp) addpropID(dcxp, idp);

	LOG_FEND();
}
/**********************************************************************/
/*
WARNING: vtype_e must match lexical order of tokens in valuetypes[]
*/
const struct allowtok_s proptype_allow = {
	.ntoks = 4,
	.toks = {
	   TK_NULL,
	   TK_immediate,
	   TK_implied,
	   TK_network,
	}
};

static int prop_wrapup(struct dcxt_s *dcxp);

/**********************************************************************/
/*
func: pprop_start

Start tag for property or pseudo property (includedev, propertypointer)
*/
static void
pprop_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	struct ddlprop_s *pp;
	struct ddlprop_s *parent;
	tok_t ptype;
	const ddlchar_t *INITIALIZED(propID);
	uint32_t arraysize;

	//const ddlchar_t *vtypep = atta[5];

	LOG_FSTART();
	/*
	if this is the first child we need to wrap up the current 
	property before processing the children.
	*/
	if (((dcxp->m.dev.curprop->children) == NULL && prop_wrapup(dcxp) < 0))
	{
		SKIPON(dcxp);
		return;
	}

	ptype = dcxp->elestack[dcxp->nestlvl];
	switch (ptype) {
	case TK_property:
		/* array is attribute 0 */
		if (gooduint(atta[0], &arraysize) != 0) arraysize = 1;
		propID = atta[2];
		break;
	case TK_includedev:
		/* array is attribute 2 */
		if (gooduint(atta[2], &arraysize) != 0) arraysize = 1;
		propID = atta[4];
		break;
	case TK_propertypointer:
		arraysize = 1;
		propID = atta[0];
		break;
	}
	/* allocate a new property */
	pp = acnNew(struct ddlprop_s);
	parent = dcxp->m.dev.curprop;
	pp->parent = parent;	/* link to our parent */
	pp->childaddr = parent->childaddr; /* default - content may override */
	pp->array = arraysize;
	dcxp->arraytotal *= arraysize;
	pp->childinc = parent->childinc;	/* inherit - may get overwritten */
	if (parent->array > 1) pp->arrayprop = parent;
	else pp->arrayprop = parent->arrayprop;

	dcxp->m.dev.curprop = pp;

	switch (ptype) {
	case TK_property:
		/* valuetype is attribute 5 */
		if (atta[5] == NULL
			|| (pp->vtype = tokmatchofs(atta[5], &proptype_allow)) == -1)
		{
			acnlogmark(lgERR, "%4d bad or missing valuetype", dcxp->elcount);
			acnfree(pp);
			SKIPON(dcxp);
			return;
		}
		pp->pnum = dcxp->m.dev.propnum++;
		break;
	case TK_includedev: {
		const ddlchar_t *dcidstr;

		if ((dcidstr = resolveuuid(dcxp, atta[0], NULL)) == NULL) {
			acnfree(pp);
			SKIPON(dcxp);
			return;
		}
		pp->vtype = VT_include;
		if ((dcxp->m.dev.subdevno & 0xff) == 0) ++dcxp->m.dev.subdevno;
		pp->pnum = dcxp->m.dev.subdevno++;
		/* link to inherited params before adding new ones */
		pp->v.dev.params = itsdevice(pp->parent)->v.dev.params;
		queue_module(dcxp, TK_device, dcidstr, pp);
		} break;
	case TK_propertypointer: {
		ppoint_s *tp;
		const ddlchar_t *path;

		path = pool_addstr(&dcxp->parsepool, atta[1]);
		acnlogmark(lgDBUG, "%4d propertypointer to %s", dcxp->elcount, path);
		tp = (ppoint_s *)add_devtask(dcxp, &resolvepp, sizeof(ppoint_s));
		tp->prop = pp;
		tp->path = path;
		pp->vtype = VT_alias;
		pp->pnum = dcxp->m.dev.propnum++;
		} break;
	default:
		break;
	}

	if (propID) addpropID(dcxp, propID);
	pp->siblings = parent->children;	/* link us to parents children */
	parent->children = pp;
	LOG_FEND();
}
/**********************************************************************/
static void
pprop_end(struct dcxt_s *dcxp)
{
	struct ddlprop_s *pp;

	LOG_FSTART();
	pp = dcxp->m.dev.curprop;

	switch (pp->vtype) {
	case VT_include:
		if (pp->array > 1 && !pp->childinc) {
			acnlogmark(lgERR,
						"%4d include array with no child increment",
						dcxp->elcount);
		}
		break;
	case VT_alias:  /* not used */
		break;
	case VT_device:
	default:  /* must be a <property> element */
		/* if no children we still need to wrap up the property */
		if ((pp->children) == NULL)
			prop_wrapup(dcxp);
		else /* reverse the order of any children to match documnt order */
			pp->children = reverseproplist(pp->children);
		break;
	}
	dcxp->arraytotal /= pp->array;
	dcxp->m.dev.curprop = pp->parent;
	LOG_FEND();
}
/**********************************************************************/
/*
func: prop_wrapup

Call after initial elements within property (label, behavior, value, 
protocol) but before children or included properties.

*/
static int
prop_wrapup(struct dcxt_s *dcxp)
{
	struct ddlprop_s *pp;
	int i;

	LOG_FSTART();
	pp = dcxp->m.dev.curprop;

	i = 0;
	switch (pp->vtype) {
	case VT_network:
		if (1
#if ACNCFG_DDLACCESS_DMP
			&& pp->v.net.dmp == NULL
#endif
#if ACNCFG_DDLACCESS_EPI26
			&& pp->v.net.dmx == NULL
#endif
		) {
			acnlogmark(lgERR, "%4d net property with no protocol details", dcxp->elcount);
			i = -1;
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
			i = -1;
		} else if (!(pp->v.imm.count == 1 || pp->v.imm.count == dcxp->arraytotal)) {
			acnlogmark(lgERR, "%4d wrong immediate value count. Expected %u, got %u", dcxp->elcount, dcxp->arraytotal, pp->v.imm.count);
		}
		break;
	case VT_include:
			acnlogmark(lgERR, "%4d wrapup includedev", dcxp->elcount);
			i = -1;
		break;
	}
	if (dcxp->m.dev.nbvs > 0) {
		int j;

		for (j = 0; j < dcxp->m.dev.nbvs; ++j) {
			acnlogmark(lgDBUG, "%4d exec behavior", dcxp->elcount);
			execbv(dcxp, dcxp->m.dev.bvs[j]);
		}
		dcxp->m.dev.nbvs = 0;
	} else if (pp->vtype != VT_device && pp->vtype != VT_alias) {
		acnlogmark(lgWARN, "%4d property with no behaviors", dcxp->elcount);
	}
	LOG_FEND();
	return i;
}
/**********************************************************************/
static void
behavior_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	const struct bv_s *bv;
	struct bvset_s *set;
	const ddlchar_t *dcidstr;
	uint8_t dcid[UUID_SIZE];

	const ddlchar_t *setp = atta[3];
	const ddlchar_t *namep = atta[1];

	//acnlogmark(lgDBUG, "%4d behavior %s", dcxp->elcount, namep);

	LOG_FSTART();
	if ((dcidstr = resolveuuid(dcxp, setp, dcid)) == NULL) {
		SKIPON(dcxp);
		return;
	}
	acnlogmark(lgDBUG, "%4d behavior set %s, name %s", dcxp->elcount, setp, namep);
	bv = findbv(dcid, namep, &set);
	acnlogmark(lgDBUG, "%4d set %s, name %s", dcxp->elcount,
					(set ? "found" : "not found"), 
					(bv ? "found" : "not found"));
	if (bv == NULL) {
		if (set == NULL) {
			parseroot(dcidstr);
		}
		bv = findbv(dcid, namep, NULL);
	}
	if (bv == NULL) {
		acnlogmark(lgERR, "%4d behavior %s not in set %s", dcxp->elcount, namep, setp);
	} else if (dcxp->m.dev.nbvs >= PROP_MAXBVS) {
		acnlogmark(lgERR, "%4d too many behaviors on property (max %d)", dcxp->elcount, PROP_MAXBVS);
	} else {
		dcxp->m.dev.bvs[dcxp->m.dev.nbvs++] = bv;
	}
#if 0
	if (dcxp->m.dev.nbvs >= PROP_MAXBVS) {
		acnlogmark(lgERR, "%4d property has more than maximum (%u) behaviors", dcxp->elcount, PROP_MAXBVS);
	} else {
		dcxp->m.dev.bvs[dcxp->m.dev.nbvs++] = bv;
	}
#endif
	LOG_FEND();
}
/**********************************************************************/
static void
protocol_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	const ddlchar_t *protoname = atta[1];

	LOG_FSTART();
	if (strcasecmp(protoname, "ESTA.DMP") != 0) {
		SKIPON(dcxp);
		acnlogmark(lgINFO, "%4d skipping protocol \"%s\"...", dcxp->elcount, protoname);
	}
	LOG_FEND();
}
/**********************************************************************/
#if ACNCFG_DDL_IMMEDIATEPROPS
/* WARNING: order of immediat pseudotypes in vtype_e must match */
const struct allowtok_s valtype_allow = {
	.ntoks = 5,
	.toks = {
		TK_float,
		TK_object,
		TK_sint,
		TK_string,
	   TK_uint,
	}
};

const int vsizes[] = {
	sizeof(double),
	sizeof(struct immobj_s),
	sizeof(int32_t),
	sizeof(ddlchar_t *),
	sizeof(uint32_t),
};

static void
value_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	int i;
	const ddlchar_t *ptype = atta[1];
	const ddlchar_t *contentparam = atta[3];
	uint8_t *arrayp;
	struct ddlprop_s *pp;

	LOG_FSTART();
	pp = dcxp->m.dev.curprop;

	if (pp->vtype < VT_imm_FIRST && pp->vtype != VT_imm_unknown) {
		acnlogmark(lgERR, "%4d <value> in non-immediate property",
					dcxp->elcount);
skipvalue:
		SKIPON(dcxp);
		return;
	}
	if ((i = tokmatchofs(ptype, &valtype_allow)) < 0) {
		acnlogmark(lgERR, "%4d value: bad type \"%s\"",
					dcxp->elcount, ptype);
		goto skipvalue;
	}

	if (pp->v.imm.count == 0) {
		pp->vtype = VT_imm_FIRST + i;
	} else {
		if (pp->v.imm.count >= dcxp->arraytotal) {
			acnlogmark(lgERR, "%4d multiple values in single property",
						dcxp->elcount);
			goto skipvalue;
		}
		if ((VT_imm_FIRST + i) != pp->vtype) {
			acnlogmark(lgWARN, "%4d value type mismatch \"%s\"",
					dcxp->elcount, ptype);
			goto skipvalue;
		}
		if (pp->v.imm.count == 1) {
			/* need to assign an array for values */
			arrayp = mallocx(vsizes[i] * dcxp->arraytotal);
			switch (i) {
			case VT_imm_uint - VT_imm_FIRST:   /* uint */
				*(uint32_t *)arrayp = pp->v.imm.t.ui;
				pp->v.imm.t.Aui = (uint32_t *)arrayp;
				break;
			case VT_imm_sint - VT_imm_FIRST:   /* sint */
				*(int32_t *)arrayp = pp->v.imm.t.si;
				pp->v.imm.t.Asi = (int32_t *)arrayp;
				break;
			case VT_imm_float - VT_imm_FIRST:   /* float */
				*(double *)arrayp = pp->v.imm.t.f;
				pp->v.imm.t.Af = (double *)arrayp;
				break;
			case VT_imm_string - VT_imm_FIRST:   /* string */
				*(const ddlchar_t **)arrayp = pp->v.imm.t.str;
				pp->v.imm.t.Astr = (const ddlchar_t **)arrayp;
				break;
			case VT_imm_object - VT_imm_FIRST:   /* object */
				*(struct immobj_s *)arrayp = pp->v.imm.t.obj;
				pp->v.imm.t.Aobj = (struct immobj_s *)arrayp;
				break;
			default:
				break;
			}
		}
	}
	startText(dcxp, contentparam);
	LOG_FEND();
}
/**********************************************************************/
static void
value_end(struct dcxt_s *dcxp)
{
	struct ddlprop_s *pp;
	const ddlchar_t *vtext;
	uint32_t count;

	LOG_FSTART();
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
		const ddlchar_t *s;

		s = pool_addstr(&dcxp->rootdev->strpool, vtext);
		if (count == 0) pp->v.imm.t.str = s;
		else pp->v.imm.t.Astr[count] = s;
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
	LOG_FEND();
}
#endif /* ACNCFG_DDL_IMMEDIATEPROPS */
/**********************************************************************/
/*
func: findaddr

search the map for our insertion point

Returns the region in the map where addr belongs (our address is less
than the current low address at this point, or this point overlaps 
our low address). If ours is a sparse array it may also overlap zero 
or more higher regions.
*/
static int
findaddr(union addrmap_u *amap, uint32_t addr)
{
	int lo, hi, i;
	struct addrfind_s *af;

	LOG_FSTART();
	/* search the map for our insertion point */
	lo = 0;
	hi = amap->srch.count;
	af = amap->srch.map;
	while (hi > lo) {
		i = (hi + lo) / 2;
		if (addr < af[i].adlo) hi = i;
		else if (addr <= af[i].adhi) {
			LOG_FEND();
			return i;
		}
		else lo = i + 1;
	}
	LOG_FEND();
	return lo;
}
/**********************************************************************/
#if ACNCFG_DDLACCESS_DMP
/*
func: mapprop

Add a dmp property to the property map.

During parsing property map is always a search type.

Make a single pass up the array tree (if any) filling in the 
dimensions in the netprop.dim[] array. At the same time calculate: 
total number of addresses and address span.

Determine whether the array is packed or has overlapping indexes.

The dim[] array is in sorted order from largest increment at dim[0] 
to smallest at dim[n]. This code anticipates that larger increments 
will be higher up the tree so we will find the smaller ones first, 
but whilst this is the most likely it is not guaranteed so we need 
to sort as we go.
*/

static void
mapprop(struct dcxt_s *dcxp, struct ddlprop_s *prop)
{
	struct dmpprop_s *np;
	struct ddlprop_s *pp;
	uint32_t INITIALIZED(base);
	uint32_t ulim;
	union addrmap_u *amap;
	struct addrfind_s *af;
	int i;
	int arraytotal;
	bool ispacked;
	int32_t inc;


	LOG_FSTART();
	inc = prop->inc;
	np = prop->v.net.dmp;

	np->nxt = dcxp->rootdev->dmpprops;
	dcxp->rootdev->dmpprops = np;
	amap = dcxp->rootdev->amap;
	pp = prop;
	if (np->ndims == 0) {
		ulim = 1;
		arraytotal = 1;
	} else {
		struct dmpdim_s *dp;

		//acnlogmark(lgDBUG, "%4d %u dims", dcxp->elcount, np->ndims);
		dp = np->dim + np->ndims - 1;
		i = 0;
		if (inc) {
			dp->lvl = i;  /* record the original tree level */
			dp->inc = inc;
			dp->cnt = pp->array;
			--dp; ++i;
		}
		while (i < np->ndims) {
			struct dmpdim_s *sdp, *ddp;

			pp = pp->arrayprop;
			inc = pp->childinc;

			sdp = dp;
			ddp = sdp++;
			/* find our place */
			while (sdp < np->dim + np->ndims && inc < sdp->inc) {
				*ddp++ = *sdp++;  /* move larger indexes down (struct copy) */
			}
			ddp->lvl = i;
			ddp->inc = inc;
			ddp->cnt = pp->array;
			--dp; ++i;
		}
		/*
		now check for self overlap, calculate totals and
		index the tree levels
		*/
		i = np->ndims - 1;
		dp = np->dim + i;
		arraytotal = dp->cnt;
		ulim = dp->inc * (dp->cnt - 1) + 1;
		np->dim[dp->lvl].tref = i;
		while (--i >= 0) {
			--dp;
			if (dp->inc < ulim) {
				np->flags |= pflg(overlap);
				amap->srch.flags |= pflg(overlap);
			}
			ulim += dp->inc * (dp->cnt - 1);
			arraytotal *= dp->cnt;
			np->dim[dp->lvl].tref = i;
		}
	}
	assert(arraytotal = dcxp->arraytotal);

	if (ulim < arraytotal) {	/* sanity check */
		/* there are fewer addresses than array elements */
		acnlogmark(lgERR, 
			"%4d invalid array: %d addresses in range %u-%u",
			dcxp->elcount, arraytotal, base, ulim);
		return;
	}
	if ((ispacked = (ulim == arraytotal))) np->flags |= pflg(packed);

	np->span = ulim;
	base = np->addr;
	ulim += base - 1;
	/* now have lower and upper inclusive limits */
	if (dcxp->rootdev->maxaddr < ulim) dcxp->rootdev->maxaddr = ulim;
	if (dcxp->rootdev->minaddr > base) dcxp->rootdev->minaddr = base;
	dcxp->rootdev->nflatprops += arraytotal;
	
	if (np->ndims > amap->srch.maxdims) amap->srch.maxdims = np->ndims;

	i = findaddr(amap, base);
	/*
	Our region is from base to ulim. We've found the region which base
	is either below or within.
	*/
	af = amap->srch.map + i;
	/* check for overlaps with other properties */
	if (i == amap->srch.count || ulim < af->adlo) {  /* no overlap */
		if ((amap->srch.count + 1) > maplength(amap, srch)) {
			af = growsrchmap(amap) + i;
		}
		if (i < amap->srch.count)
			memmove(af + 1, af, sizeof(*af) * (amap->srch.count - i));
		++amap->srch.count;
		af->adlo = base;
		af->adhi = ulim;
		af->ntests = !ispacked;
		af->p.prop = np;
	} else /* overlap */ if (ispacked) {
		/*
		We must split this region and insert ours between.
		*/
		if ((amap->srch.count + 2) > maplength(amap, srch)) {
			af = growsrchmap(amap) + i;
		}
		memmove(af + 2, af, sizeof(*af) * (amap->srch.count - i));
		amap->srch.count += 2;
		af->adhi = base - 1;
		++af;
		af->adlo = base;
		af->adhi = ulim;
		af->ntests = 0;
		af->p.prop = np;
		++af;
		af->adlo = ulim + 1;
	} else {
		int nnew, j;
		uint32_t lasthi;
		struct addrfind_s *oaf;
		/*
		our sparse region overlaps the current one (which must also 
		be sparse) and possibly higher ones too. Where we overlap 
		sparse regions we can just merge the two. Where we overlap 
		consecutive packed regions with space in between, or where 
		we extend beyond a last packed region we must add a new 
		intermediate region.
		*/
		nnew = 0;
		j = i; oaf = af;
		lasthi = 0;
		while (++j < amap->srch.count && (++oaf)->adlo < ulim) {
			if (oaf->ntests == 0) {
				nnew += (lasthi && lasthi < oaf->adlo);
				lasthi = oaf->adhi + 1;
			} else {
				lasthi = 0;
			}
		}
		nnew += (lasthi != 0);
		if (nnew) {
			while ((amap->srch.count + nnew) > maplength(amap, srch))
				af = growsrchmap(amap) + i;
			/* move overlapping and higher regions out of the way */
			memmove(af + nnew, af, sizeof(*af) * (amap->srch.count - i));
			amap->srch.count += nnew;
		}
		j = i + nnew;
		oaf = af + nnew;

		while (base < ulim) {
			if (oaf->ntests) {
				struct dmpprop_s **pa;

				af->adlo = (base < oaf->adlo) ? base : oaf->adlo;
				af->adhi = (ulim > oaf->adhi) ? ulim : oaf->adhi;
				/* link in our extra property */
				af->ntests = oaf->ntests + 1;
				pa = af->p.pa;
				if (af->ntests == 2) {
					pa = mallocx(AFTESTBLKINC * sizeof(*pa));
					pa[0] = af->p.prop;
					af->p.pa = pa;
				} else if (((af->ntests) % AFTESTBLKINC) == 1) {
					pa = realloc(pa, (af->ntests + AFTESTBLKINC - 1) * sizeof(*pa));
					assert(pa);
					af->p.pa = pa;
				}
				pa[af->ntests - 1] = np;
				/* now look at next up */
				++oaf; ++j;
				if (j < amap->srch.count && oaf->adlo < af->adhi) {
					af->adhi = oaf->adlo - 1;
				}
				base = af->adhi + 1;
				++af;
			} else {
				if (af != oaf) memcpy(af, oaf, sizeof(*oaf));
				base = af->adhi + 1;
				++af;
				if (++j == amap->srch.count || ((++oaf)->ntests == 0 && oaf->adlo > base)) {
					af->adlo = base;
					af->adhi = (j == amap->srch.count) ? ulim : oaf->adlo - 1;
					af->ntests = 1;
					af->p.prop = np;
					base = af->adhi + 1;
				}
			}
		}
	}
	LOG_FEND();
}
#endif  /* ACNCFG_DDLACCESS_DMP */
/**********************************************************************/

#if ACNCFG_DDLACCESS_DMP
/*
func: propref_start
*/
static void
propref_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	struct ddlprop_s *pp;
	struct dmpprop_s *np;
	unsigned int flags;
	struct ddlprop_s *xpp;
	int dims;

	const ddlchar_t *locp = atta[7];
	const ddlchar_t *sizep = atta[11];
	const ddlchar_t *incp = atta[5];

	LOG_FSTART();
	pp = dcxp->m.dev.curprop;
	if (pp->v.net.dmp) {
		acnlogmark(lgERR, "%4d multiple <propref_DMP>s for the same property",
						dcxp->elcount);
		return;
	}
	/* need to count dimensions before we can allocate */
	dims = (pp->array > 1);  /* add 1 if this prop is an array */
	for (xpp = pp->arrayprop; xpp != NULL; xpp = xpp->arrayprop) ++dims;
	np = mallocxz(dmppropsize(dims));
	np->ndims = dims;
	pp->v.net.dmp = np;
	np->prop = pp;

	/* get all the flags */
	flags = 0;
	if (getboolatt(atta[9])) flags |= pflg(read);
	if (getboolatt(atta[15])) flags |= pflg(write);
	if (getboolatt(atta[2])) flags |= pflg(event);
	if (getboolatt(atta[13])) flags |= pflg(vsize);
	if (getboolatt(atta[0])) flags |= pflg(abs);
	np->flags = flags;

	/* "loc" is mandatory */
	if (gooduint(locp, &np->addr) == 0) {
		if (!(flags & pflg(abs)))
			np->addr += pp->parent->childaddr;
	} else {
		acnlogmark(lgERR, "%4d bad location \"%s\"",
						dcxp->elcount, locp);
	}

	/* "size" is mandatory */
	if (gooduint(sizep, &np->size) < 0) {
		acnlogmark(lgERR, "%4d bad size \"%s\"", dcxp->elcount, sizep);
	}

	/* inc is necessary if we are an array, ignored otherwise */
	if (pp->array > 1
		&& (incp == NULL 
			|| goodint(incp, &pp->inc) < 0)
	) {
		acnlogmark(lgWARN, "%4d array with no inc - assume 1",
					dcxp->elcount);
		pp->inc = 1;
	}
	mapprop(dcxp, pp);

	assert(dcxp->arraytotal > 0);

	dcxp->rootdev->nnetprops += 1;
	acnlogmark(lgDBUG, "%4d property addr %u, this dim %u, total %u", 
		dcxp->elcount, 
		np->addr, 
		pp->array, 
		dcxp->arraytotal);
	LOG_FEND();
}
#endif  /* ACNCFG_DDLACCESS_DMP */
/**********************************************************************/
#if ACNCFG_DDLACCESS_DMP
static void
childrule_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	struct ddlprop_s *pp;
	uint32_t addoff;

	const ddlchar_t *absp = atta[0];
	const ddlchar_t *incp = atta[3];
	const ddlchar_t *locp = atta[5];

	LOG_FSTART();
	pp = dcxp->m.dev.curprop;
	addoff = pp->childaddr;
	if (getboolatt(absp)) addoff = 0;

	if (gooduint(locp, &pp->childaddr) == 0) {
		pp->childaddr += addoff;
	}
	if (incp) gooduint(incp, &pp->childinc);
	LOG_FEND();
}

#endif  /* ACNCFG_DDLACCESS_DMP */
/**********************************************************************/
#ifdef ACNCFG_PROPEXT_TOKS
#undef _EXTOKEN_
#define _EXTOKEN_(tk, type) TK_ ## tk ,
const struct allowtok_s extendallow = {
	.ntoks = ACNCFG_NUMEXTENDFIELDS,
	.toks = {
	   ACNCFG_PROPEXT_TOKS
	}
};

static void
EA_propext_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	struct dmpprop_s *np;
	int ofs;

	const ddlchar_t *namep = atta[1];
	const ddlchar_t *valp = atta[2];

	LOG_FSTART();
	np = dcxp->m.dev.curprop->v.net.dmp;
	if (np == NULL) {
		acnlogmark(lgERR, "%4d ea:propext element must follow propref_DMP", dcxp->elcount);
	} else if ((ofs = tokmatchofs(namep, &extendallow)) < 0) {
		acnlogmark(lgNTCE, "%4d ea:propext discarding name=\"%s\"", dcxp->elcount, namep);
	} else {
		acnlogmark(lgDBUG, "%4d ea:propext type%d=%s", dcxp->elcount, ofs, valp);
		np->extends[ofs] = pool_addstr(&dcxp->rootdev->strpool, valp);
	}
	LOG_FEND();
}
#endif
/**********************************************************************/
static void
setparam_start(struct dcxt_s *dcxp, const ddlchar_t **atta)
{
	struct param_s *parp;

	const ddlchar_t *name = atta[1];
	const ddlchar_t *contentparam = atta[2];

	LOG_FSTART();
	parp = acnNew(struct param_s);
	parp->name = pool_addstr(&dcxp->parsepool, name);
	parp->nxt = dcxp->m.dev.curprop->v.dev.params;
	dcxp->m.dev.curprop->v.dev.params = parp;
	startText(dcxp, contentparam);
	LOG_FEND();
}
/**********************************************************************/
static void
setparam_end(struct dcxt_s *dcxp)
{
	struct param_s *parp;

	LOG_FSTART();
	parp = dcxp->m.dev.curprop->v.dev.params;
	/* need to use parsepool rather than modpool unless we get
	smarter about deleting params that go out of scope */
	parp->subs = pool_addstr(&dcxp->parsepool, endText(dcxp));
	acnlogmark(lgDBUG, "%4d add param %s=\"%s\"",
							dcxp->elcount, parp->name, parp->subs);
	LOG_FEND();
}
/**********************************************************************/
/*
Main element handler callbacks and vector table
*/
/**********************************************************************/
typedef void elemstart_fn(struct dcxt_s *dcxp, const ddlchar_t **atta);
typedef void elemend_fn(struct dcxt_s *dcxp);

elemstart_fn *startvec[TK__elmax_] = {
	[TK_device]	= &dev_start,
	[TK_behaviorset] = &bset_start,
	[TK_behaviordef] = &bvdef_start,
	[TK_refines] = &refines_start,
	[TK_languageset] = &lset_start,
	[TK_language] = &language_start,
	[TK_string] = &string_start,
	[TK_property] = &pprop_start,
	[TK_propertypointer] = &pprop_start,
	[TK_includedev] = &pprop_start,
#if ACNCFG_DDL_IMMEDIATEPROPS
	[TK_value] = &value_start,
#endif /* ACNCFG_DDL_IMMEDIATEPROPS */
	[TK_protocol] = &protocol_start,
	[TK_UUIDname] = &alias_start,
	[TK_propref_DMP] = &propref_start,
	[TK_childrule_DMP] = &childrule_start,
	[TK_setparam] = &setparam_start,
#if ACNCFG_DDL_BEHAVIORS
	[TK_behavior] = &behavior_start,
#endif /* ACNCFG_DDL_BEHAVIORS */
	[TK_label] = &label_start,
#ifdef ACNCFG_PROPEXT_TOKS
	[TK_http_engarts_propext] = &EA_propext_start,
#endif
};

elemend_fn *endvec[TK__elmax_] = {
	[TK_behaviorset] = &bset_end,
	[TK_behaviordef] = &bvdef_end,
	[TK_languageset] = &lset_end,
	[TK_string] = &string_end,
	[TK_device]	= &pprop_end,
	[TK_property] = &pprop_end,
	[TK_includedev] = &pprop_end,
	[TK_propertypointer] = &pprop_end,
#if ACNCFG_DDL_IMMEDIATEPROPS
	[TK_value] = &value_end,
#endif /* ACNCFG_DDL_IMMEDIATEPROPS */
	[TK_setparam] = &setparam_end,
	[TK_label] = &label_end,
};


static void
el_start(void *data, const ddlchar_t *el, const ddlchar_t **atts)
{
	struct dcxt_s *dcxp = (struct dcxt_s *)data;
	tok_t eltok;
	int atti;
	const struct allowtok_s *allow;
	const ddlchar_t *atta[MAXATTS];
	rq_att_t required;

	LOG_FSTART();
	++dcxp->elcount;
	++dcxp->nestlvl;
	if (SKIPPING(dcxp)) return;

	if (dcxp->nestlvl >= ACNCFG_DDL_MAXNEST) {
		acnlogmark(lgERR, "E%4d Maximum XML nesting reached. skipping...", dcxp->elcount);
		SKIPON(dcxp);
		return;
	}

	//acnlogmark(lgDBUG, "<%s %d>", el, dcxp->nestlvl);
	if (dcxp->nestlvl == 0) {
		allow = &content_DOCROOT;
	} else {
		allow = content[dcxp->elestack[dcxp->nestlvl - 1]];
	}
	if (allow == NULL || !ISELTOKEN(eltok = tokmatchtok(el, allow))) {
		acnlogmark(lgWARN, "%4d Unexpected element \"%s\". skipping...", dcxp->elcount, el);
		SKIPON(dcxp);
	} else {
		dcxp->elestack[dcxp->nestlvl] = eltok;
		allow = elematts[eltok];
		assert(allow);
		memset(atta, 0, sizeof(atta));
		for (; *atts != NULL; atts += 2) {
			atti = tokmatchofs(atts[0], allow);
			if (atti < 0) {
				acnlogmark(lgWARN, "%4d unknown attribute %s=\"%s\"",
								dcxp->elcount, atts[0], atts[1]);
			} else {
				/* if already set must have been overridden by a parameter */
				if (atta[atti] == NULL) atta[atti] = atts[1];
				if (ISPARAMNAME(allow->toks[atti]))
					subsparam(dcxp, atts[1], &atta[atti - 1]);
			}
		}
		required = rq_atts[eltok];
		for (atti = 0; required; required >>= 1, ++atti) {
			if ((required & 1) && !atta[atti]) {
				acnlogmark(lgERR, "%4d <%s> required attribute %s missing. skipping...",
								dcxp->elcount, el,  tokstrs[allow->toks[atti]]);
				SKIPON(dcxp);
			}
		}
		if (startvec[eltok] && !SKIPPING(dcxp)) (*startvec[eltok])(dcxp, atta);
	}
	dcxp->elprev = TK__none_;
	LOG_FEND();
	return;
}
/**********************************************************************/
static void
el_end(void *data, const ddlchar_t *el)
{
	struct dcxt_s *dcxp = (struct dcxt_s *)data;
	tok_t eltok;

	LOG_FSTART();
	//acnlogmark(lgDBUG, "</%s %d>", el, dcxp->nestlvl);
	if (!SKIPPING(dcxp)) {
		eltok = dcxp->elestack[dcxp->nestlvl];

		if (endvec[eltok]) {
			//acnlogmark(lgDBUG, "End  %s", el);
			(*endvec[eltok])(dcxp);
		}
		dcxp->elprev = eltok;
	} else if (dcxp->nestlvl == dcxp->skip) {
		dcxp->skip = NOSKIP;
		acnlogmark(lgDBUG, "%4d ...done", dcxp->elcount);
	}
	--dcxp->nestlvl;
	LOG_FEND();
}
/**********************************************************************/
/*
Module queuing
*/
/**********************************************************************/
/*
func: queue_module

Add a DDL module to the queue for future processing.

The structure of DDL would naturally encourage recursive processing but
that could easily be exessively resource hungry in lightweight components
so when a reference requiring processing af a subsiduary module is encountered
the module is added to the queue together with a reference to the point at
which it applies. Modules can then be processed in sequence instead of
recursively.
*/
void
queue_module(struct dcxt_s *dcxp, tok_t modtype, const ddlchar_t *name, void *ref)
{
	struct qentry_s *qentry;
	unsigned int namelen;

	LOG_FSTART();

	namelen = strlen(name);
	qentry = mallocx(sizeof(struct qentry_s) + namelen + 1);
	qentry->next = NULL;
	qentry->modtype = modtype;
	qentry->ref = ref;
	strcpy(qentry->name, name);

	if (dcxp->queuehead == NULL) dcxp->queuehead = qentry;
	else dcxp->queuetail->next = qentry;
	dcxp->queuetail = qentry;
	acnlogmark(lgDBUG, "module %s queued", name);
	LOG_FEND();
}
/**********************************************************************/
static void
check_queued_modulex(struct dcxt_s *dcxp, tok_t typefound, const ddlchar_t *uuidstr, uint8_t *dcid)
{
	struct qentry_s *qentry;
	int fail = 0;
	uint8_t mdcid[UUID_SIZE];
	uint8_t qdcid[UUID_SIZE];

	LOG_FSTART();
	qentry = dcxp->queuehead;

	if (qentry->modtype != TK_unknown && qentry->modtype != typefound) {
		acnlogmark(lgERR, "     DDL module: expected %s, found %s",
				tokstrs[qentry->modtype], tokstrs[typefound]);
		fail = 1;
	}
	if (dcid == NULL) dcid = mdcid;
	if (str2uuid(uuidstr, dcid) != 0) {
		acnlogmark(lgERR, "     DDL module: bad UUID %s", uuidstr);
		fail = 1;
	} else if (str2uuid(qentry->name, qdcid) == 0
		&& !uuidsEq(dcid, qdcid))
	{
		acnlogmark(lgERR, "     DDL module UUID: expected %s, found %s",
				qentry->name,
				uuidstr);
		fail = 1;
	}
	if (fail) exit(EXIT_FAILURE);
	acnlogmark(lgINFO, "Starting %s %s", tokstrs[typefound], uuidstr);
	LOG_FEND();
}
/**********************************************************************/
static void
parsemodules(struct dcxt_s *dcxp)
{
	int fd;
	int sz;
	void *buf;
	struct qentry_s *qentry;
#if acntestlog(lgDBUG)
#endif

	LOG_FSTART();
	while ((qentry = dcxp->queuehead)) {
		acnlogmark(lgINFO, "Parse %s %s", tokstrs[qentry->modtype], qentry->name);

		memset(&dcxp->m, 0, sizeof(dcxp->m));
		dcxp->elcount = 0;
		fd = openddlx(qentry->name);

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

		/*
		aliases are now out of scope
		*/
		dcxp->naliases = 0;
		/*
		dump any module scoped strings
		*/
		pool_reset(&dcxp->modulepool);

		acnlogmark(lgDBUG, "     end     %s", qentry->name);

		dcxp->queuehead = qentry->next;
		free(qentry);
	}
	dcxp->queuetail = NULL;
	XML_ParserFree(dcxp->parser);
	/*
	Dump parser scoped strings
	*/
	pool_reset(&dcxp->parsepool);

	LOG_FEND();
}
/**********************************************************************/
/*
func: parseroot

Parse the root device of a component.
This is the main entry point for the device parser. It is passed a name which
must resolve to a DDL document (see <resolve.c>). It initalizes the root, 
enters the na,med module as first in the queue and starts the parser. As 
subdevices or other modules are found, during construction of the property
tree, they are added to the queue and processed in turn.
This function returns once the entire queue has been processed.
As parsing progresses, both the property tree (DDL properties) and the address
map (DMP properties) are constructed.

returns:pointer to the resulting rootdev structure (<struct rootdev_s>) or
NULL if unrecoverable errors are encountered.

The rootdev_s should be freed with <freerootdev> when it is no longer needed.
*/
struct rootdev_s *
parseroot(const char *name)
{
	struct dcxt_s dcxt;
	struct devtask_s *tp;
	int i = 0;

	LOG_FSTART();
	memset(&dcxt, 0, sizeof(dcxt));
	dcxt.skip = NOSKIP;
	dcxt.elprev = TK__none_;
	dcxt.nestlvl = -1;

	init_behaviors(&dcxt);

	queue_module(&dcxt, TK_unknown, name, NULL);

	parsemodules(&dcxt);

	showbsets();

	/* are there queued tasks? */
	while ((tp = dcxt.tasks) != NULL) {
		acnlogmark(lgDBUG, "task %d", ++i);
		(*tp->task)(dcxt.rootdev, (void *)tp->taskdata);
		dcxt.tasks = tp->nxt;
		acnfree(tp);
	}

	if (dcxt.rootdev) {
		/* finished root property */
		acnlogmark(lgDBUG, "Found %d net properties", dcxt.rootdev->nnetprops);
		acnlogmark(lgDBUG, " %d flat net properties", dcxt.rootdev->nflatprops);
		acnlogmark(lgDBUG, " min addr %u", dcxt.rootdev->minaddr);
		acnlogmark(lgDBUG, " max addr %u", dcxt.rootdev->maxaddr);

		adduuid(&devtrees, dcxt.rootdev->dcid);
	}
	LOG_FEND();
	return dcxt.rootdev;
}
/**********************************************************************/
/*
Destructors
*/
/**********************************************************************/
static void
freeprop(struct ddlprop_s *prop)
{
	
	LOG_FSTART();
	/*
	if (prop->id) freestr((void *)prop->id);
	*/
	
	while (1) {
		struct ddlprop_s *pp;

		pp = prop->children;
		if (pp == NULL) break;
		prop->children = pp->siblings;
		pp->siblings = NULL;
		freeprop(pp);
	}
	acnlogmark(lgDBUG, "     Freeing %s", propxname(prop));
	
	/*
	FIXME: Check carefully for memory leaks.
	*/
	switch (prop->vtype) {
	case VT_NULL:
	case VT_imm_unknown:
	case VT_implied:
		break;
	case VT_network: {
/*
		struct dmpprop_s *np;

		np = prop->v.net.dmp;
		free(np);
*/
		break;
	}
	case VT_include:
		acnlogmark(lgWARN, "     Freeing incomplete includedev");
	case VT_device:
		if (prop->v.dev.params) {
			struct param_s *pp, *ep;

			ep = NULL;
			if (prop->parent)
				ep = itsdevice(prop->parent)->v.dev.params;
			
			while ((pp = prop->v.dev.params) != ep) {
				prop->v.dev.params = pp->nxt;
				free(pp);
			}
		}
		break;
	case VT_imm_uint:
	case VT_imm_sint:
	case VT_imm_float:
		if (prop->v.imm.count > 1) free(prop->v.imm.t.ptr);
		break;
	case VT_imm_string:
		if (prop->v.imm.count > 1) free(prop->v.imm.t.Astr);
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
	LOG_FEND();
}
/**********************************************************************/
/*
func: freerootdev

Free all the resources used by a rootdev_s.
*/
void
freerootdev(struct rootdev_s *dev)
{
	struct dmpprop_s *dprop;

	LOG_FSTART();
	pool_reset(&dev->strpool);
	if (dev->ddlroot) freeprop(dev->ddlroot);
	if (dev->amap) freeamap(dev->amap);
	for (dprop = dev->dmpprops; dprop;) {
	   struct dmpprop_s *np;

	   np = dprop->nxt;
	   free(dprop);
	   dprop = np;
	}
	free(dev);
	LOG_FEND();
}
