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
file: behaviors.c

Functions to handle specific DDL behaviors.

These functions are called during parsing when the behavior 
declaration is encountered. They are passed a pointer to a dcxt_s 
which provides all the necessary context in the property tree and a 
pointer together with the behavior set and name. A single function 
typically handles multiple closely related behaviors and/or 
equivalent behaviors from several behaviorsets.

topic: adding tasks for subsequent actions

Most behaviors can be processed immediately they are encountered in 
the parse. At this point the property to which the behavior applies 
and all its ancestors are defined. However, the content of the 
current property and details of its value are not yet known. If more 
information is requiredd in order to complete the processing a task 
can be registered using <add_proptask> which will ba called when the 
end tag of the property is eached, by which time the content will 
also have been parsed.
*/
/**********************************************************************/
/*
Logging level for this source file.
If not set it will default to the global CF_LOG_DEFAULT

options are

lgOFF lgEMRG lgALRT lgCRIT lgERR lgWARN lgNTCE lgINFO lgDBUG
*/
//#define LOGLEVEL lgDBUG

/**********************************************************************/
#include <expat.h>
#include <assert.h>
#include "acn.h"

/**********************************************************************/
/*
group: Property flag behaviors

The dmp property structure includes flags for access permissions read,
write and event. This is extended by defining flags for a number of 
access qualifiers such as `persistent` and similar behaviors.
*/
/**********************************************************************/
/*
func: setbvflg()

Set property flags.

- Called by multiple behavior actions.
- Only makes sense on net properties.
*/
void
setbvflg(struct dcxt_s *dcxp, enum netflags_e flags)
{
	struct ddlprop_s *pp;
	struct dmpprop_s *np;

	pp = dcxp->m.dev.curprop;
	
	LOG_FSTART();
	if (pp->vtype != VT_network) {
		if (flags != pflg(constant)) {
			acnlogmark(lgERR,
				"%24s: access class (0x%04x) on non-network property",
				propxpath(pp), flags);
		}
		return;
	}
	np = pp->v.net.dmp;
	flags |= np->flags;

	/* perform some sanity checks */
	if ((flags & pflg(constant)) && (flags & (pflg(volatile) | pflg(persistent)))) {
		acnlogmark(lgERR,
			"%24s: constant property cannot also be volatile or persistent",
			propxpath(pp));
		return;
	}
	np->flags = flags;

#if acntestlog(lgDBUG)
	char buf[pflg_NAMELEN + pflg_COUNT];
	acnlogmark(lgDBUG,
		"%24s:%s", propxpath(pp), flagnames(flags, pflgnames, buf, " %s"));
#endif
	LOG_FEND();
}

/**********************************************************************/
/*
func: persistent_bva()

behavior - persistent
behaviorsets - acnbase, acnbase-r2
*/
void
persistent_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setbvflg(dcxp, pflg(persistent));
}

/**********************************************************************/
/*
func: constant_bva()

behavior name - constant
behaviorsets - acnbase, acnbase-r2
*/

void
constant_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setbvflg(dcxp, pflg(constant));
}

/**********************************************************************/
/*
func: volatile_bva()

behavior name - volatile
behaviorsets - acnbase, acnbase-r2
*/

void
volatile_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setbvflg(dcxp, pflg(volatile));
}
/**********************************************************************/
/*
func: ordered_bva()

behavior name - ordered
behaviorsets - acnbase, acnbase-r2
*/

void
ordered_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setbvflg(dcxp, pflg(ordered));
}

/**********************************************************************/
/*
func: measure_bva()

behavior name - scalar
behaviorsets - acnbase, acnbase-r2
*/

void
measure_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setbvflg(dcxp, pflg(ordered) | pflg(measure));
}

/**********************************************************************/
/*
func: cyclic_bva()

behavior name - cyclic
behaviorsets - acnbase, acnbase-r2
*/

void
cyclic_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setbvflg(dcxp, pflg(ordered) | pflg(cyclic));
}
/**********************************************************************/
/*
group: Property encoding behaviors

Behaviors that define basic property encodings {integer, string, etc.}
*/
/**********************************************************************/
/*
flags for setptype
*/
#define SZ_1   1
#define SZ_2   2
#define SZ_4   4
#define SZ_8   8
#define SZ_16 0x10
#define SZ_6  0x20
/* any fixed size */
#define SZ_AF 0x4000
#define SZ_V  0x8000
/**********************************************************************/
/*
func: setptype

Set encoding type
- only make sense for network properties
*/
void
setptype(struct dcxt_s *dcxp, enum proptype_e type, unsigned int sizes)
{
	struct ddlprop_s *pp;
	struct dmpprop_s *np;

	pp = dcxp->m.dev.curprop;
	if (pp->vtype != VT_network) {
		acnlogmark(lgDBUG,
			"%24s: ignoring type/encoding behavior on non-network property",
			propxpath(pp));
		return;
	}
	np = pp->v.net.dmp;
	if (np->flags & pflg(vsize)) {
		if (!(sizes &= SZ_V)) {
			acnlogmark(lgERR, "%24s: %s cannot be variable size",
				propxpath(pp), etypes[type]);
			return;
		}
	} else {
		if (sizes == SZ_V) {
			acnlogmark(lgERR, "%24s: %s must be variable size",
				propxpath(pp), etypes[type]);
			return;
		}
		switch (np->size) {
		case 1: sizes &= (SZ_1 | SZ_AF); break;
		case 2: sizes &= (SZ_2 | SZ_AF); break;
		case 4: sizes &= (SZ_4 | SZ_AF); break;
		case 6: sizes &= (SZ_6 | SZ_AF); break;
		case 8: sizes &= (SZ_8 | SZ_AF); break;
		case 16: sizes &= (SZ_16 | SZ_AF); break;
		default: sizes &= SZ_AF; break;
		}
		if (sizes == 0) {
			acnlogmark(lgERR, "%24s: %s is unsupported size (%u)",
				propxpath(pp), etypes[type], np->size);
			return;
		}
	}
	if (np->etype && np->etype != type) {
		acnlogmark(lgINFO,
			"%24s: overriding %s with %s", 
			propxpath(pp), etypes[np->etype], etypes[type]);
	}
	np->etype = type;
}

/**********************************************************************/
/*
func: type_boolean_bva()

behavior name - type.boolean
behaviorsets - acnbase, acnbase-r2
*/

void
type_boolean_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_boolean, SZ_AF);
}


/**********************************************************************/
/*
func: type_sint_bva()

behavior name - type.signed.integer, type.sint
behaviorsets - acnbase, acnbase-r2
*/

void
type_sint_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_sint, SZ_1 | SZ_2 | SZ_4 | SZ_8);
}


/**********************************************************************/
/*
func: type_uint_bva()

behavior name - type.unsigned.integer, type.uint
behaviorsets - acnbase, acnbase-r2
*/

void
type_uint_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_uint, SZ_1 | SZ_2 | SZ_4 | SZ_8);
}


/**********************************************************************/
/*
func: type_float_bva()

behavior name - type.float
behaviorsets - acnbase, acnbase-r2
*/

void
type_float_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_float, SZ_4 | SZ_8);
}


/**********************************************************************/
/*
func: type_char_UTF_8_bva()

behavior name - type.char.UTF-8
behaviorsets - acnbase, acnbase-r2
*/

void
type_char_UTF_8_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_UTF8, SZ_AF);
}


/**********************************************************************/
/*
func: type_char_UTF_16_bva()

behavior name - type.char.UTF-16
behaviorsets - acnbase, acnbase-r2
*/

void
type_char_UTF_16_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_UTF16, SZ_AF);
}


/**********************************************************************/
/*
func: type_char_UTF_32_bva()

behavior name - type.char.UTF-32
behaviorsets - acnbase, acnbase-r2
*/

void
type_char_UTF_32_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_UTF32, SZ_AF);
}


/**********************************************************************/
/*
func: type_string_bva()

behavior name - type.string
behaviorsets - acnbase, acnbase-r2
*/

void
type_string_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_string, SZ_V);
}


/**********************************************************************/
/*
func: type_enum_bva()

behavior name - type.enumeration, type.enum
behaviorsets - acnbase, acnbase-r2
*/

void
type_enum_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_enum, SZ_1 | SZ_2 | SZ_4 | SZ_8);
}


/**********************************************************************/
/*
func: type_fixBinob_bva()

behavior name - type.fixBinob
behaviorsets - acnbase, acnbase-r2
*/

void
type_fixBinob_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_opaque, SZ_AF);
}


/**********************************************************************/
/*
func: type_varBinob_bva()

behavior name - type.varBinob
behaviorsets - acnbase, acnbase-r2
*/

void
type_varBinob_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_opaque, SZ_V);
}

/**********************************************************************/
/*
func: binObject_bva()

behavior name - binObject
behaviorsets - acnbase, acnbase-r2
*/

void
binObject_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_opaque, SZ_V | SZ_AF);
}


/**********************************************************************/
void
setuuidtype(struct dcxt_s *dcxp, const struct bv_s *bv, enum proptype_e type)
{
	struct ddlprop_s *pp;

	pp = dcxp->m.dev.curprop;
	switch (pp->vtype) {
	case VT_network:
		setptype(dcxp, type, SZ_16);
		break;
	case VT_imm_string: {
		const ddlchar_t *alias;
		struct immobj_s *Aobj;
		const ddlchar_t **Astr;
		int i;
		uint8_t *dcid;

		/* we are converting strings to objects */
		if (pp->v.imm.count > 1) {
			Aobj = acnalloc(pp->v.imm.count * sizeof(struct immobj_s));
			if (Aobj == NULL) {
				acnlogmark(lgERR, "Out of memory");
				break;
			}
			Astr = pp->v.imm.t.Astr;
		} else {
			Aobj = &pp->v.imm.t.obj;
			Astr = &pp->v.imm.t.str;
		}
		for (i = 0; i < pp->v.imm.count; ++i) {
			alias = Astr[i];
			if ((dcid = acnalloc(UUID_SIZE)) == NULL) {
				acnlogmark(lgERR, "Out of memory");
			} else if (resolveuuid(dcxp, alias, dcid) == NULL) {
				acnlogmark(lgERR, "Can't resolve UUID %s", alias);
				acnfree(dcid);
				dcid = NULL;
			}
			Aobj[i].data = dcid;
			Aobj[i].size = dcid ? UUID_SIZE : 0;
			pool_delstr(&dcxp->rootdev->strpool, alias);
		}
		if (pp->v.imm.count > 1) {
			pp->v.imm.t.Aobj = Aobj;
			acnfree(Astr  /* , pp->v.imm.count * sizeof(ddlchar_t *) */);
		}
		pp->vtype = VT_imm_object;
		} break;
	case VT_NULL:
	case VT_imm_unknown:
	case VT_implied:
	case VT_include:
	case VT_device:
	case VT_alias:
	case VT_imm_float:
	case VT_imm_object:
	case VT_imm_sint:
	case VT_imm_uint:
		acnlogmark(lgERR, "Behavior %s is only resolvable as an immediate string or network property",
			bv->name);
		break;
	}
}
/**********************************************************************/
/*
func: UUID_bva()

behavior name - UUID
behaviorsets - acnbase, acnbase-r2
*/

void
UUID_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setuuidtype(dcxp, bv, etype_UUID);

}
/**********************************************************************/
/*
func: DCID_bva()

behavior name - DCID
behaviorsets - acnbase, acnbase-r2
*/

void
DCID_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setuuidtype(dcxp, bv, etype_DCID);
}
/**********************************************************************/
/*
func: CID_bva()

behavior name - CID
behaviorsets - acnbase, acnbase-r2
*/

void
CID_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setuuidtype(dcxp, bv, etype_CID);
}
/**********************************************************************/
/*
func: languagesetID_bva()

behavior name - languagesetID
behaviorsets - acnbase, acnbase-r2
*/

void
languagesetID_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setuuidtype(dcxp, bv, etype_languagesetID);
}
/**********************************************************************/
/*
func: behaviorsetID_bva()

behavior name - behaviorsetID
behaviorsets - acnbase, acnbase-r2
*/

void
behaviorsetID_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setuuidtype(dcxp, bv, etype_behaviorsetID);
}
/**********************************************************************/
/*
func: type_bitmap_bva()

behavior name - type.bitmap
behaviorsets - acnbase-r2
*/

void
type_bitmap_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_bitmap, SZ_AF);
}

/**********************************************************************/
/*
func: ISOdate_bva()

behavior name - ISOdate
behaviorsets - acnbase, acnbase-r2
*/

void
ISOdate_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_ISOdate, SZ_V);
}
/**********************************************************************/
/*
func: URI_bva()

behavior name - URI
behaviorsets - acnbase, acnbase-r2
*/

void
URI_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	setptype(dcxp, etype_URI, SZ_V);
}
/**********************************************************************/

void
deviceref_bva(struct dcxt_s *dcxp, const struct bv_s *bv)
{
	/* add_proptask(prop, &do_deviceref, bv); */
}
/**********************************************************************/

#define DCID_acnbase     "71576eac-e94a-11dc-b664-0017316c497d"  
#define DCID_acnbase_r2  "3e2ca216-b753-11df-90fd-0017316c497d"
#define DCID_acnbaseExt1 "5def7c40-35c1-11df-b42f-0017316c497d"
#define DCID_artnet      "102dbb3e-3120-11df-962e-0017316c497d"
#define DCID_sl          "4ef14fd4-2e8d-11de-876f-0017316c497d"

#define BVA(name, action) 

struct bv_s known_bvs[] = {
	{"71576eac-e94a-11dc-b664-0017316c497d", NULL},  /* acnbase */
//	{"NULL",                          NULL_bva                          },
//	{"EMPTY",                         EMPTY_bva                         },
//	{"typingPrimitive",               typingPrimitive_bva               },
//	{"group",                         group_bva                         },
	{"ordered",                       ordered_bva                       , BV_FINAL},  /* set pflag */
	{"measure",                       measure_bva                       , BV_FINAL},  /* set pflag */
	{"scalar",                        measure_bva                       , BV_FINAL},  /* set pflag */
	{"cyclic",                        cyclic_bva                        , BV_FINAL},  /* set pflag */
//	{"reference",                     reference_bva                     },
//	{"bitmap",                        bitmap_bva                        },
	{"binObject",                     binObject_bva                     , BV_FINAL},  /* set etype */
//	{"enumeration",                   enumeration_bva                   },
//	{"boolean",                       boolean_bva                       },
//	{"character",                     character_bva                     },
//	{"textString",                    textString_bva                    },
//	{"encoding",                      encoding_bva                      },
//	{"type.integer",                  type_integer_bva                  },
//	{"type.unsigned.integer",         type_unsigned_integer_bva         },
//	{"type.signed.integer",           type_signed_integer_bva           },
	{"type.uint",                     type_uint_bva                     },
	{"type.sint",                     type_sint_bva                     },
//	{"type.floating_point",           type_floating_point_bva           },
	{"type.float",                    type_float_bva                    },
	{"type.enumeration",              type_enum_bva                     , BV_FINAL},  /* set etype */
	{"type.enum",                     type_enum_bva                     , BV_FINAL},  /* set etype */
	{"type.boolean",                  type_boolean_bva                  , BV_FINAL},  /* set etype */
	{"type.bitmap",                   type_bitmap_bva                   },
	{"type.fixBinob",                 type_fixBinob_bva                 },
	{"type.varBinob",                 type_varBinob_bva                 },
//	{"type.character",                type_character_bva                },
	{"type.char.UTF-8",               type_char_UTF_8_bva               , BV_FINAL},  /* set etype */
	{"type.char.UTF-16",              type_char_UTF_16_bva              , BV_FINAL},  /* set etype */
	{"type.char.UTF-32",              type_char_UTF_32_bva              , BV_FINAL},  /* set etype */
	{"type.string",                   type_string_bva                   , BV_FINAL},  /* set etype */
//	{"type.NCName",                   type_NCName_bva                   },
//	{"stringRef",                     stringRef_bva                     },
//	{"accessClass",                   accessClass_bva                   },
	{"persistent",                    persistent_bva                    , BV_FINAL},  /* set pflag */
	{"volatile",                      volatile_bva                      , BV_FINAL},  /* set pflag */
	{"constant",                      constant_bva                      , BV_FINAL},  /* set pflag */
//	{"accessOrder",                   accessOrder_bva                   },
//	{"atomicLoad",                    atomicLoad_bva                    },
//	{"atomicMaster",                  atomicMaster_bva                  },
//	{"atomicTrigger",                 atomicTrigger_bva                 },
//	{"atomicGroupMember",             atomicGroupMember_bva             },
//	{"atomicParent",                  atomicParent_bva                  },
//	{"atomicWithAncestor",            atomicWithAncestor_bva            },
//	{"atomicMasterRef",               atomicMasterRef_bva               },
//	{"syncGroupMember",               syncGroupMember_bva               },
//	{"algorithm",                     algorithm_bva                     },
//	{"behaviorRef",                   behaviorRef_bva                   },
//	{"paramSzArray",                  paramSzArray_bva                  },
//	{"arraySize",                     arraySize_bva                     },
//	{"propertySetSelector",           propertySetSelector_bva           },
//	{"propertySet",                   propertySet_bva                   },
//	{"label",                         label_bva                         },
//	{"labelString",                   labelString_bva                   },
//	{"labelRef",                      labelRef_bva                      },
//	{"multidimensionalGroup",         multidimensionalGroup_bva         },
//	{"deviceInfoGroup",               deviceInfoGroup_bva               },
//	{"deviceSupervisory",             deviceSupervisory_bva             },
//	{"sharedProps",                   sharedProps_bva                   },
	{"UUID",                          UUID_bva                          , BV_FINAL},  /* set etype */
	{"CID",                           CID_bva                           , BV_FINAL},  /* set etype */
	{"languagesetID",                 languagesetID_bva                 , BV_FINAL},  /* set etype */
	{"behaviorsetID",                 behaviorsetID_bva                 , BV_FINAL},  /* set etype */
	{"DCID",                          DCID_bva                          , BV_FINAL},  /* set etype */
//	{"time",                          time_bva                          },
//	{"timePoint",                     timePoint_bva                     },
//	{"countdownTime",                 countdownTime_bva                 },
//	{"timePeriod",                    timePeriod_bva                    },
//	{"date",                          date_bva                          },
	{"ISOdate",                       ISOdate_bva                       , BV_FINAL},  /* set etype */
//	{"componentReference",            componentReference_bva            },
//	{"deviceRef",                     deviceRef_bva                     },
//	{"CIDreference",                  CIDreference_bva                  },
//	{"propertyRef",                   propertyRef_bva                   },
//	{"DDLpropertyRef",                DDLpropertyRef_bva                },
//	{"namedPropertyRef",              namedPropertyRef_bva              },
//	{"localDDLpropertyRef",           localDDLpropertyRef_bva           },
//	{"globalDDLpropertyRef",          globalDDLpropertyRef_bva          },
//	{"DMPpropertyRef",                DMPpropertyRef_bva                },
//	{"DMPpropertyAddress",            DMPpropertyAddress_bva            },
//	{"localPropertyAddress",          localPropertyAddress_bva          },
//	{"systemPropertyAddress",         systemPropertyAddress_bva         },
//	{"xenoPropertyReference",         xenoPropertyReference_bva         },
//	{"transportConnection",           transportConnection_bva           },
//	{"connection.ESTA.DMP",           connection_ESTA_DMP_bva           },
//	{"connection.ESTA.SDT",           connection_ESTA_SDT_bva           },
//	{"connection.ESTA.SDT.ESTA.DMP",  connection_ESTA_SDT_ESTA_DMP_bva  },
//	{"URI",                           URI_bva                           },
//	{"URL",                           URL_bva                           },
//	{"URN",                           URN_bva                           },
//	{"devInfoItem",                   devInfoItem_bva                   },
//	{"manufacturer",                  manufacturer_bva                  },
//	{"maunfacturerURL",               manufacturerURL_bva               },
//	{"ESTA_OrgID",                    ESTA_OrgID_bva                    },
//	{"IEEE_OUI",                      IEEE_OUI_bva                      },
//	{"devModelName",                  devModelName_bva                  },
//	{"devSerialNo",                   devSerialNo_bva                   },
//	{"date.manufacture",              date_manufacture_bva              },
//	{"date.firmwareRev",              date_firmwareRev_bva              },
//	{"softwareVersion",               softwareVersion_bva               },
//	{"hardwareVersion",               hardwareVersion_bva               },
//	{"FCTN",                          FCTN_bva                          },
//	{"UACN",                          UACN_bva                          },
//	{"scale",                         scale_bva                         },
//	{"unitScale",                     unitScale_bva                     },
//	{"fullScale",                     fullScale_bva                     },
//	{"measureOffset",                 measureOffset_bva                 },
//	{"dimension",                     dimension_bva                     },
//	{"dimensional-scale",             dimensional_scale_bva             },
//	{"prefix-yocto",                  prefix_yocto_bva                  },
//	{"prefix-zepto",                  prefix_zepto_bva                  },
//	{"prefix-atto",                   prefix_atto_bva                   },
//	{"prefix-femto",                  prefix_femto_bva                  },
//	{"prefix-pico",                   prefix_pico_bva                   },
//	{"prefix-nano",                   prefix_nano_bva                   },
//	{"prefix-micro",                  prefix_micro_bva                  },
//	{"prefix-milli",                  prefix_milli_bva                  },
//	{"prefix-kilo",                   prefix_kilo_bva                   },
//	{"prefix-mega",                   prefix_mega_bva                   },
//	{"prefix-giga",                   prefix_giga_bva                   },
//	{"prefix-tera",                   prefix_tera_bva                   },
//	{"prefix-peta",                   prefix_peta_bva                   },
//	{"prefix-exa",                    prefix_exa_bva                    },
//	{"prefix-zetta",                  prefix_zetta_bva                  },
//	{"prefix-yotta",                  prefix_yotta_bva                  },
//	{"dim-mass",                      dim_mass_bva                      },
//	{"mass-g",                        mass_g_bva                        },
//	{"dim-length",                    dim_length_bva                    },
//	{"length-m",                      length_m_bva                      },
//	{"dim-time",                      dim_time_bva                      },
//	{"time-s",                        time_s_bva                        },
//	{"dim-charge",                    dim_charge_bva                    },
//	{"charge-C",                      charge_C_bva                      },
//	{"dim-temp",                      dim_temp_bva                      },
//	{"temp-K",                        temp_K_bva                        },
//	{"temp-celsius",                  temp_celsius_bva                  },
//	{"dim-angle",                     dim_angle_bva                     },
//	{"angle-rad",                     angle_rad_bva                     },
//	{"angle-deg",                     angle_deg_bva                     },
//	{"dim-solid-angle",               dim_solid_angle_bva               },
//	{"solid-angle-sr",                solid_angle_sr_bva                },
//	{"dim-freq",                      dim_freq_bva                      },
//	{"freq-Hz",                       freq_Hz_bva                       },
//	{"dim-area",                      dim_area_bva                      },
//	{"area-sq-m",                     area_sq_m_bva                     },
//	{"dim-volume",                    dim_volume_bva                    },
//	{"volume-cu-m",                   volume_cu_m_bva                   },
//	{"volume-L",                      volume_L_bva                      },
//	{"dim-force",                     dim_force_bva                     },
//	{"force-N",                       force_N_bva                       },
//	{"dim-energy",                    dim_energy_bva                    },
//	{"energy-J",                      energy_J_bva                      },
//	{"dim-power",                     dim_power_bva                     },
//	{"power-W",                       power_W_bva                       },
//	{"dim-pressure",                  dim_pressure_bva                  },
//	{"pressure-Pa",                   pressure_Pa_bva                   },
//	{"dim-current",                   dim_current_bva                   },
//	{"current-A",                     current_A_bva                     },
//	{"dim-voltage",                   dim_voltage_bva                   },
//	{"voltage-V",                     voltage_V_bva                     },
//	{"dim-resistance",                dim_resistance_bva                },
//	{"resistance-ohm",                resistance_ohm_bva                },
//	{"dim-torque",                    dim_torque_bva                    },
//	{"torque-Nm",                     torque_Nm_bva                     },
//	{"perceptual-dimension",          perceptual_dimension_bva          },
//	{"dim-luminous-intensity",        dim_luminous_intensity_bva        },
//	{"luminous-intensity-cd",         luminous_intensity_cd_bva         },
//	{"dim-luminous-flux",             dim_luminous_flux_bva             },
//	{"luminous-flux-lm",              luminous_flux_lm_bva              },
//	{"dim-illuminance",               dim_illuminance_bva               },
//	{"illuminance-lx",                illuminance_lx_bva                },
//	{"ratio",                         ratio_bva                         },
//	{"logratio",                      logratio_bva                      },
//	{"logunit",                       logunit_bva                       },
//	{"power-dBmW",                    power_dBmW_bva                    },
//	{"nonLinearity",                  nonLinearity_bva                  },
//	{"scalable-nonLinearity",         scalable_nonLinearity_bva         },
//	{"normalized-nonlinearity",       normalized_nonlinearity_bva       },
//	{"nonlin-log",                    nonlin_log_bva                    },
//	{"nonlin-log10",                  nonlin_log10_bva                  },
//	{"nonlin-ln",                     nonlin_ln_bva                     },
//	{"nonlin-squareLaw",              nonlin_squareLaw_bva              },
//	{"normalized-square-law",         normalized_square_law_bva         },
//	{"nonlin-S-curve",                nonlin_S_curve_bva                },
//	{"nonlin-S-curve-precise",        nonlin_S_curve_precise_bva        },
//	{"nonlin-monotonic",              nonlin_monotonic_bva              },
//	{"normalized-monotonic",          normalized_monotonic_bva          },
//	{"priority",                      priority_bva                      },
//	{"driven",                        driven_bva                        },
//	{"driver",                        driver_bva                        },
//	{"target",                        target_bva                        },
//	{"unattainableAction",            unattainableAction_bva            },
//	{"currentTarget",                 currentTarget_bva                 },
//	{"trippable",                     trippable_bva                     },
//	{"limit",                         limit_bva                         },
//	{"limitMinExc",                   limitMinExc_bva                   },
//	{"limitMinInc",                   limitMinInc_bva                   },
//	{"limitMaxExc",                   limitMaxExc_bva                   },
//	{"limitMaxInc",                   limitMaxInc_bva                   },
//	{"limitByAccess",                 limitByAccess_bva                 },
//	{"limitNetWrite",                 limitNetWrite_bva                 },
//	{"relativeTarget",                relativeTarget_bva                },
//	{"moveTarget",                    moveTarget_bva                    },
//	{"moveRelative",                  moveRelative_bva                  },
//	{"actionTimer",                   actionTimer_bva                   },
//	{"targetTimer",                   targetTimer_bva                   },
//	{"delayTime",                     delayTime_bva                     },
//	{"atTime",                        atTime_bva                        },
//	{"rate",                          rate_bva                          },
//	{"rate1st",                       rate1st_bva                       },
//	{"rate1stLimit",                  rate1stLimit_bva                  },
//	{"rate2nd",                       rate2nd_bva                       },
//	{"rate2ndLimit",                  rate2ndLimit_bva                  },
//	{"suspend",                       suspend_bva                       },
//	{"progressIndicator",             progressIndicator_bva             },
//	{"progressCounter",               progressCounter_bva               },
//	{"progressTimer",                 progressTimer_bva                 },
//	{"maxDriven",                     maxDriven_bva                     },
//	{"minDriven",                     minDriven_bva                     },
//	{"drivenOr",                      drivenOr_bva                      },
//	{"drivenAnd",                     drivenAnd_bva                     },
//	{"maxDrivenPrioritized",          maxDrivenPrioritized_bva          },
//	{"spatialCoordinate",             spatialCoordinate_bva             },
//	{"ordinate",                      ordinate_bva                      },
//	{"datum",                         datum_bva                         },
//	{"localDatum",                    localDatum_bva                    },
//	{"datumProperty",                 datumProperty_bva                 },
//	{"coordinateReference",           coordinateReference_bva           },
//	{"deviceDatum",                   deviceDatum_bva                   },
//	{"deviceDatumDescription",        deviceDatumDescription_bva        },
//	{"length",                        length_bva                        },
//	{"angle",                         angle_bva                         },
//	{"orthogonalLength",              orthogonalLength_bva              },
//	{"ordX",                          ordX_bva                          },
//	{"ordY",                          ordY_bva                          },
//	{"ordZ",                          ordZ_bva                          },
//	{"polarOrdinate",                 polarOrdinate_bva                 },
//	{"radialLength",                  radialLength_bva                  },
//	{"angleX",                        angleX_bva                        },
//	{"angleY",                        angleY_bva                        },
//	{"angleZ",                        angleZ_bva                        },
//	{"point2D",                       point2D_bva                       },
//	{"point3D",                       point3D_bva                       },
//	{"direction",                     direction_bva                     },
//	{"orientation",                   orientation_bva                   },
//	{"direction3D",                   direction3D_bva                   },
//	{"orientation3D",                 orientation3D_bva                 },
//	{"position3D",                    position3D_bva                    },
//	{"publishParam",                  publishParam_bva                  },
//	{"publishMinTime",                publishMinTime_bva                },
//	{"publishMaxTime",                publishMaxTime_bva                },
//	{"publishThreshold",              publishThreshold_bva              },
//	{"publishEnable",                 publishEnable_bva                 },
//	{"pollInterval",                  pollInterval_bva                  },
//	{"minPollInterval",               minPollInterval_bva               },
//	{"maxPollInterval",               maxPollInterval_bva               },
//	{"errorReport",                   errorReport_bva                   },
//	{"connectionDependent",           connectionDependent_bva           },
//	{"connectedSwitch",               connectedSwitch_bva               },
//	{"connectionReporter",            connectionReporter_bva            },
//	{"binding",                       binding_bva                       },
//	{"boundProperty",                 boundProperty_bva                 },
//	{"bindingAnchor",                 bindingAnchor_bva                 },
//	{"windowProperty",                windowProperty_bva                },
//	{"bindingMechanism",              bindingMechanism_bva              },
//	{"binder",                        binder_bva                        },
//	{"binderRef",                     binderRef_bva                     },
//	{"pushBindingMechanism",          pushBindingMechanism_bva          },
//	{"pullBindingMechanism",          pullBindingMechanism_bva          },
//	{"internalSlaveRef",              internalSlaveRef_bva              },
//	{"internalMasterRef",             internalMasterRef_bva             },
//	{"internalBidiRef",               internalBidiRef_bva               },
//	{"DMPbinding",                    DMPbinding_bva                    },
//	{"DMPsetPropBinding",             DMPsetPropBinding_bva             },
//	{"DMPgetPropBinding",             DMPgetPropBinding_bva             },
//	{"DMPeventBinding",               DMPeventBinding_bva               },
//	{"bindingState",                  bindingState_bva                  },
//	{"xenoPropRef",                   xenoPropRef_bva                   },
//	{"xenoBinder",                    xenoBinder_bva                    },
//	{"accessWindow",                  accessWindow_bva                  },
//	{"accessMatch",                   accessMatch_bva                   },
//	{"accessEnable",                  accessEnable_bva                  },
//	{"accessInhibit",                 accessInhibit_bva                 },
//	{"dynamicAccessEnable",           dynamicAccessEnable_bva           },
//	{"connectionMatch",               connectionMatch_bva               },
//	{"contextMatchWindow",            contextMatchWindow_bva            },
//	{"autoAssignContextWindow",       autoAssignContextWindow_bva       },
//	{"preferredValue.abstract",       preferredValue_abstract_bva       },
//	{"preferredValue",                preferredValue_bva                },
//	{"repeatPrefVal",                 repeatPrefVal_bva                 },
//	{"repeatPrefValOffset",           repeatPrefValOffset_bva           },
//	{"selected",                      selected_bva                      },
//	{"selector",                      selector_bva                      },
//	{"choice",                        choice_bva                        },
//	{"enumSelector",                  enumSelector_bva                  },
//	{"fractionalSelector",            fractionalSelector_bva            },
//	{"positionalSelector",            positionalSelector_bva            },
//	{"case",                          case_bva                          },
//	{"cyclicPath",                    cyclicPath_bva                    },
//	{"cyclicDir.increasing",          cyclicDir_increasing_bva          },
//	{"cyclicDir.decreasing",          cyclicDir_decreasing_bva          },
//	{"cyclicDir.shortest",            cyclicDir_shortest_bva            },
//	{"cyclicPath.scalar",             cyclicPath_scalar_bva             },
//	{"connectedState",                connectedState_bva                },
//	{"autoConnectedState",            autoConnectedState_bva            },
//	{"explicitConnectedState",        explicitConnectedState_bva        },
//	{"writeConnectedState",           writeConnectedState_bva           },
//	{"readConnectedState",            readConnectedState_bva            },
//	{"autoTrackedConnection",         autoTrackedConnection_bva         },
//	{"trackTargetRef",                trackTargetRef_bva                },
//	{"loadOnAction",                  loadOnAction_bva                  },
//	{"actionSpecifier",               actionSpecifier_bva               },
//	{"actionProperty",                actionProperty_bva                },
//	{"propertyActionSpecifier",       propertyActionSpecifier_bva       },
//	{"propertyLoadAction",            propertyLoadAction_bva            },
//	{"propertyChangeAction",          propertyChangeAction_bva          },
//	{"actionState",                   actionState_bva                   },
//	{"actionStateBefore",             actionStateBefore_bva             },
//	{"actionStateAfter",              actionStateAfter_bva              },
//	{"initializer",                   initializer_bva                   },
//	{"initializationState",           initializationState_bva           },
//	{"initialization.enum",           initialization_enum_bva           },
//	{"initializationBool",            initializationBool_bva            },
//	{"refInArray",                    refInArray_bva                    },
//	{"rangeOver",                     rangeOver_bva                     },
//	{"contextDependent",              contextDependent_bva              },
//	{"controllerContextDependent",    controllerContextDependent_bva    },
//	{"connectionContextDependent",    connectionContextDependent_bva    },
//	{"netInterface",                  netInterface_bva                  },
//	{"netInterfaceItem",              netInterfaceItem_bva              },
//	{"netInterfaceRef",               netInterfaceRef_bva               },
//	{"accessNetInterface",            accessNetInterface_bva            },
//	{"netInterfaceDirection",         netInterfaceDirection_bva         },
//	{"netAddress",                    netAddress_bva                    },
//	{"myNetAddress",                  myNetAddress_bva                  },
//	{"routerAddress",                 routerAddress_bva                 },
//	{"serviceAddress",                serviceAddress_bva                },
//	{"netInterfaceState",             netInterfaceState_bva             },
//	{"netInterfaceIEEE802.3",         netInterfaceIEEE802_3_bva         },
//	{"netInterfaceIEEE802.11",        netInterfaceIEEE802_11_bva        },
//	{"netAddressIEEE-EUI",            netAddressIEEE_EUI_bva            },
//	{"netIfaceIPv4",                  netIfaceIPv4_bva                  },
//	{"netAddressIPv4",                netAddressIPv4_bva                },
//	{"netAddressIPv6",                netAddressIPv6_bva                },
//	{"netMask",                       netMask_bva                       },
//	{"netMaskIPv4",                   netMaskIPv4_bva                   },
//	{"netNetworkAddress",             netNetworkAddress_bva             },
//	{"netHostAddress",                netHostAddress_bva                },
//	{"myAddressDHCP",                 myAddressDHCP_bva                 },
//	{"myAddressLinkLocal",            myAddressLinkLocal_bva            },
//	{"myAddressStatic",               myAddressStatic_bva               },
//	{"defaultRouteAddress",           defaultRouteAddress_bva           },
//	{"DHCPserviceAddress",            DHCPserviceAddress_bva            },
//	{"DHCPLeaseTime",                 DHCPLeaseTime_bva                 },
//	{"DHCPLeaseRemaining",            DHCPLeaseRemaining_bva            },
//	{"DHCPclientState",               DHCPclientState_bva               },
//	{"netInterfaceDMX512",            netInterfaceDMX512_bva            },
//	{"universeIdDMX512",              universeIdDMX512_bva              },
//	{"netInterfaceDMX512pair",        netInterfaceDMX512pair_bva        },
//	{"netDMX512-XLRpri",              netDMX512_XLRpri_bva              },
//	{"netDMX512-XLRsec",              netDMX512_XLRsec_bva              },
//	{"netIfaceE1.31",                 netIfaceE1_31_bva                 },
//	{"universeIdE1.31",               universeIdE1_31_bva               },
//	{"slotAddressDMX512",             slotAddressDMX512_bva             },
//	{"baseAddressDMX512",             baseAddressDMX512_bva             },
//	{"STARTCode",                     STARTCode_bva                     },
//	{"DMXpropRef",                    DMXpropRef_bva                    },
//	{"DMXpropRef-SC0",                DMXpropRef_SC0_bva                },
//	{"bindingDMXnull",                bindingDMXnull_bva                },
//	{"bindingDMXalt-refresh",         bindingDMXalt_refresh_bva         },
//	{"streamGroup",                   streamGroup_bva                   },
//	{"streamPoint",                   streamPoint_bva                   },
//	{"streamInput",                   streamInput_bva                   },
//	{"streamOuput",                   streamOuput_bva                   },
//	{"streamCoverter",                streamCoverter_bva                },
//	{"streamRatio",                   streamRatio_bva                   },
//	{"streamGovernor",                streamGovernor_bva                },
//	{"beamSource",                    beamSource_bva                    },
//	{"beamDiverter",                  beamDiverter_bva                  },
//	{"streamFilter",                  streamFilter_bva                  },
//	{"lightSource",                   lightSource_bva                   },
//	{"colorSpec",                     colorSpec_bva                     },
//	{"colorFilter",                   colorFilter_bva                   },
//	{"beamShape",                     beamShape_bva                     },
//	{"beamTemplate",                  beamTemplate_bva                  },
//	{"opticalLens",                   opticalLens_bva                   },
//	{"simplified-specialized",        simplified_specialized_bva        },

	{"3e2ca216-b753-11df-90fd-0017316c497d", NULL},  /* acnbase_r2 */
//	{"NULL",                          NULL_bva                          },
//	{"EMPTY",                         EMPTY_bva                         },
//	{"typingPrimitive",               typingPrimitive_bva               },
//	{"group",                         group_bva                         },
	{"ordered",                       ordered_bva                       , BV_FINAL},  /* set pflag */
	{"measure",                       measure_bva                       , BV_FINAL},  /* set pflag */
	{"scalar",                        measure_bva                       , BV_FINAL},  /* set pflag */
	{"cyclic",                        cyclic_bva                        , BV_FINAL},  /* set pflag */
//	{"reference",                     reference_bva                     },
//	{"bitmap",                        bitmap_bva                        },
	{"binObject",                     binObject_bva                     , BV_FINAL},  /* set pflag */
//	{"enumeration",                   enumeration_bva                   },
//	{"boolean",                       boolean_bva                       },
//	{"character",                     character_bva                     },
//	{"textString",                    textString_bva                    },
//	{"encoding",                      encoding_bva                      },
//	{"type.integer",                  type_integer_bva                  },
//	{"type.unsigned.integer",         type_unsigned_integer_bva         },
//	{"type.signed.integer",           type_signed_integer_bva           },
	{"type.uint",                     type_uint_bva                     },
	{"type.sint",                     type_sint_bva                     },
//	{"type.floating_point",           type_floating_point_bva           },
	{"type.float",                    type_float_bva                    },
	{"type.enumeration",              type_enum_bva                     , BV_FINAL},  /* set etype */
	{"type.enum",                     type_enum_bva                     , BV_FINAL},  /* set etype */
	{"type.boolean",                  type_boolean_bva                  , BV_FINAL},  /* set etype */
	{"type.bitmap",                   type_bitmap_bva                   },
	{"type.fixBinob",                 type_fixBinob_bva                 },
	{"type.varBinob",                 type_varBinob_bva                 },
//	{"type.character",                type_character_bva                },
	{"type.char.UTF-8",               type_char_UTF_8_bva               , BV_FINAL},  /* set etype */
	{"type.char.UTF-16",              type_char_UTF_16_bva              , BV_FINAL},  /* set etype */
	{"type.char.UTF-32",              type_char_UTF_32_bva              , BV_FINAL},  /* set etype */
	{"type.string",                   type_string_bva                   , BV_FINAL},  /* set etype */
//	{"type.NCName",                   type_NCName_bva                   },
//	{"stringRef",                     stringRef_bva                     },
//	{"accessClass",                   accessClass_bva                   },
	{"persistent",                    persistent_bva                    , BV_FINAL},  /* set pflag */
	{"volatile",                      volatile_bva                      , BV_FINAL},  /* set pflag */
	{"constant",                      constant_bva                      , BV_FINAL},  /* set pflag */
//	{"accessOrder",                   accessOrder_bva                   },
//	{"atomicLoad",                    atomicLoad_bva                    },
//	{"atomicMaster",                  atomicMaster_bva                  },
//	{"atomicTrigger",                 atomicTrigger_bva                 },
//	{"atomicGroupMember",             atomicGroupMember_bva             },
//	{"atomicParent",                  atomicParent_bva                  },
//	{"atomicWithAncestor",            atomicWithAncestor_bva            },
//	{"atomicMasterRef",               atomicMasterRef_bva               },
//	{"syncGroupMember",               syncGroupMember_bva               },
//	{"algorithm",                     algorithm_bva                     },
//	{"behaviorRef",                   behaviorRef_bva                   },
//	{"paramSzArray",                  paramSzArray_bva                  },
//	{"arraySize",                     arraySize_bva                     },
//	{"propertySetSelector",           propertySetSelector_bva           },
//	{"propertySet",                   propertySet_bva                   },
//	{"label",                         label_bva                         },
//	{"labelString",                   labelString_bva                   },
//	{"labelRef",                      labelRef_bva                      },
//	{"enumLabel",                     enumLabel_bva                     },
//	{"multidimensionalGroup",         multidimensionalGroup_bva         },
//	{"deviceInfoGroup",               deviceInfoGroup_bva               },
//	{"deviceSupervisory",             deviceSupervisory_bva             },
//	{"sharedProps",                   sharedProps_bva                   },
	{"UUID",                          UUID_bva                          , BV_FINAL},  /* set etype */
	{"CID",                           CID_bva                           , BV_FINAL},  /* set etype */
	{"languagesetID",                 languagesetID_bva                 , BV_FINAL},  /* set etype */
	{"behaviorsetID",                 behaviorsetID_bva                 , BV_FINAL},  /* set etype */
	{"DCID",                          DCID_bva                          , BV_FINAL},  /* set etype */
//	{"time",                          time_bva                          },
//	{"timePoint",                     timePoint_bva                     },
//	{"countdownTime",                 countdownTime_bva                 },
//	{"timePeriod",                    timePeriod_bva                    },
//	{"date",                          date_bva                          },
	{"ISOdate",                       ISOdate_bva                       , BV_FINAL},  /* set etype */
//	{"componentReference",            componentReference_bva            },
//	{"deviceRef",                     deviceRef_bva                     },
//	{"CIDreference",                  CIDreference_bva                  },
//	{"propertyRef",                   propertyRef_bva                   },
//	{"DDLpropertyRef",                DDLpropertyRef_bva                },
//	{"namedPropertyRef",              namedPropertyRef_bva              },
//	{"localDDLpropertyRef",           localDDLpropertyRef_bva           },
//	{"globalDDLpropertyRef",          globalDDLpropertyRef_bva          },
//	{"DMPpropertyRef",                DMPpropertyRef_bva                },
//	{"DMPpropertyAddress",            DMPpropertyAddress_bva            },
//	{"localPropertyAddress",          localPropertyAddress_bva          },
//	{"systemPropertyAddress",         systemPropertyAddress_bva         },
//	{"transportConnection",           transportConnection_bva           },
//	{"connection.ESTA.DMP",           connection_ESTA_DMP_bva           },
//	{"connection.ESTA.SDT",           connection_ESTA_SDT_bva           },
//	{"connection.ESTA.SDT.ESTA.DMP",  connection_ESTA_SDT_ESTA_DMP_bva  },
//	{"URI",                           URI_bva                           },
//	{"URL",                           URL_bva                           },
//	{"URN",                           URN_bva                           },
//	{"devInfoItem",                   devInfoItem_bva                   },
//	{"manufacturer",                  manufacturer_bva                  },
//	{"manufacturerURL",               manufacturerURL_bva               },
//	{"ESTA_OrgID",                    ESTA_OrgID_bva                    },
//	{"IEEE_OUI",                      IEEE_OUI_bva                      },
//	{"devModelName",                  devModelName_bva                  },
//	{"devSerialNo",                   devSerialNo_bva                   },
//	{"date.manufacture",              date_manufacture_bva              },
//	{"date.firmwareRev",              date_firmwareRev_bva              },
//	{"softwareVersion",               softwareVersion_bva               },
//	{"hardwareVersion",               hardwareVersion_bva               },
//	{"FCTNstring",                    FCTNstring_bva                    },
//	{"FCTN",                          FCTN_bva                          },
//	{"UACNstring",                    UACNstring_bva                    },
//	{"UACN",                          UACN_bva                          },
//	{"scale",                         scale_bva                         },
//	{"unitScale",                     unitScale_bva                     },
//	{"fullScale",                     fullScale_bva                     },
//	{"measureOffset",                 measureOffset_bva                 },
//	{"dimension",                     dimension_bva                     },
//	{"dimensional-scale",             dimensional_scale_bva             },
//	{"prefix-yocto",                  prefix_yocto_bva                  },
//	{"prefix-zepto",                  prefix_zepto_bva                  },
//	{"prefix-atto",                   prefix_atto_bva                   },
//	{"prefix-femto",                  prefix_femto_bva                  },
//	{"prefix-pico",                   prefix_pico_bva                   },
//	{"prefix-nano",                   prefix_nano_bva                   },
//	{"prefix-micro",                  prefix_micro_bva                  },
//	{"prefix-milli",                  prefix_milli_bva                  },
//	{"prefix-kilo",                   prefix_kilo_bva                   },
//	{"prefix-mega",                   prefix_mega_bva                   },
//	{"prefix-giga",                   prefix_giga_bva                   },
//	{"prefix-tera",                   prefix_tera_bva                   },
//	{"prefix-peta",                   prefix_peta_bva                   },
//	{"prefix-exa",                    prefix_exa_bva                    },
//	{"prefix-zetta",                  prefix_zetta_bva                  },
//	{"prefix-yotta",                  prefix_yotta_bva                  },
//	{"dim-mass",                      dim_mass_bva                      },
//	{"mass-g",                        mass_g_bva                        },
//	{"dim-length",                    dim_length_bva                    },
//	{"length-m",                      length_m_bva                      },
//	{"dim-time",                      dim_time_bva                      },
//	{"time-s",                        time_s_bva                        },
//	{"dim-charge",                    dim_charge_bva                    },
//	{"charge-C",                      charge_C_bva                      },
//	{"dim-temp",                      dim_temp_bva                      },
//	{"temp-K",                        temp_K_bva                        },
//	{"temp-celsius",                  temp_celsius_bva                  },
//	{"dim-angle",                     dim_angle_bva                     },
//	{"angle-rad",                     angle_rad_bva                     },
//	{"angle-deg",                     angle_deg_bva                     },
//	{"dim-solid-angle",               dim_solid_angle_bva               },
//	{"solid-angle-sr",                solid_angle_sr_bva                },
//	{"dim-freq",                      dim_freq_bva                      },
//	{"freq-Hz",                       freq_Hz_bva                       },
//	{"dim-area",                      dim_area_bva                      },
//	{"area-sq-m",                     area_sq_m_bva                     },
//	{"dim-volume",                    dim_volume_bva                    },
//	{"volume-cu-m",                   volume_cu_m_bva                   },
//	{"volume-L",                      volume_L_bva                      },
//	{"dim-force",                     dim_force_bva                     },
//	{"force-N",                       force_N_bva                       },
//	{"dim-energy",                    dim_energy_bva                    },
//	{"energy-J",                      energy_J_bva                      },
//	{"dim-power",                     dim_power_bva                     },
//	{"power-W",                       power_W_bva                       },
//	{"dim-pressure",                  dim_pressure_bva                  },
//	{"pressure-Pa",                   pressure_Pa_bva                   },
//	{"dim-current",                   dim_current_bva                   },
//	{"current-A",                     current_A_bva                     },
//	{"dim-voltage",                   dim_voltage_bva                   },
//	{"voltage-V",                     voltage_V_bva                     },
//	{"dim-resistance",                dim_resistance_bva                },
//	{"resistance-ohm",                resistance_ohm_bva                },
//	{"dim-torque",                    dim_torque_bva                    },
//	{"torque-Nm",                     torque_Nm_bva                     },
//	{"perceptual-dimension",          perceptual_dimension_bva          },
//	{"dim-luminous-intensity",        dim_luminous_intensity_bva        },
//	{"luminous-intensity-cd",         luminous_intensity_cd_bva         },
//	{"dim-luminous-flux",             dim_luminous_flux_bva             },
//	{"luminous-flux-lm",              luminous_flux_lm_bva              },
//	{"dim-illuminance",               dim_illuminance_bva               },
//	{"illuminance-lx",                illuminance_lx_bva                },
//	{"ratio",                         ratio_bva                         },
//	{"logratio",                      logratio_bva                      },
//	{"logunit",                       logunit_bva                       },
//	{"power-dBmW",                    power_dBmW_bva                    },
//	{"nonLinearity",                  nonLinearity_bva                  },
//	{"scalable-nonLinearity",         scalable_nonLinearity_bva         },
//	{"normalized-nonlinearity",       normalized_nonlinearity_bva       },
//	{"nonlin-log",                    nonlin_log_bva                    },
//	{"nonlin-log10",                  nonlin_log10_bva                  },
//	{"nonlin-ln",                     nonlin_ln_bva                     },
//	{"nonlin-squareLaw",              nonlin_squareLaw_bva              },
//	{"normalized-square-law",         normalized_square_law_bva         },
//	{"nonlin-S-curve",                nonlin_S_curve_bva                },
//	{"nonlin-S-curve-precise",        nonlin_S_curve_precise_bva        },
//	{"nonlin-monotonic",              nonlin_monotonic_bva              },
//	{"normalized-monotonic",          normalized_monotonic_bva          },
//	{"abstractPriority",              abstractPriority_bva              },
//	{"priority",                      priority_bva                      },
//	{"priorityZeroOff",               priorityZeroOff_bva               },
//	{"driven",                        driven_bva                        },
//	{"driver",                        driver_bva                        },
//	{"target",                        target_bva                        },
//	{"unattainableAction",            unattainableAction_bva            },
//	{"currentTarget",                 currentTarget_bva                 },
//	{"trippable",                     trippable_bva                     },
//	{"limit",                         limit_bva                         },
//	{"limitMinExc",                   limitMinExc_bva                   },
//	{"limitMinInc",                   limitMinInc_bva                   },
//	{"limitMaxExc",                   limitMaxExc_bva                   },
//	{"limitMaxInc",                   limitMaxInc_bva                   },
//	{"limitByAccess",                 limitByAccess_bva                 },
//	{"limitNetWrite",                 limitNetWrite_bva                 },
//	{"relativeTarget",                relativeTarget_bva                },
//	{"moveTarget",                    moveTarget_bva                    },
//	{"moveRelative",                  moveRelative_bva                  },
//	{"actionTimer",                   actionTimer_bva                   },
//	{"targetTimer",                   targetTimer_bva                   },
//	{"delayTime",                     delayTime_bva                     },
//	{"atTime",                        atTime_bva                        },
//	{"rate",                          rate_bva                          },
//	{"rate1st",                       rate1st_bva                       },
//	{"rate1stLimit",                  rate1stLimit_bva                  },
//	{"rate2nd",                       rate2nd_bva                       },
//	{"rate2ndLimit",                  rate2ndLimit_bva                  },
//	{"suspend",                       suspend_bva                       },
//	{"progressIndicator",             progressIndicator_bva             },
//	{"progressCounter",               progressCounter_bva               },
//	{"progressTimer",                 progressTimer_bva                 },
//	{"maxDriven",                     maxDriven_bva                     },
//	{"minDriven",                     minDriven_bva                     },
//	{"drivenOr",                      drivenOr_bva                      },
//	{"drivenAnd",                     drivenAnd_bva                     },
//	{"maxDrivenPrioritized",          maxDrivenPrioritized_bva          },
//	{"spatialCoordinate",             spatialCoordinate_bva             },
//	{"ordinate",                      ordinate_bva                      },
//	{"datum",                         datum_bva                         },
//	{"localDatum",                    localDatum_bva                    },
//	{"datumProperty",                 datumProperty_bva                 },
//	{"coordinateReference",           coordinateReference_bva           },
//	{"deviceDatum",                   deviceDatum_bva                   },
//	{"deviceDatumDescription",        deviceDatumDescription_bva        },
//	{"length",                        length_bva                        },
//	{"angle",                         angle_bva                         },
//	{"orthogonalLength",              orthogonalLength_bva              },
//	{"ordX",                          ordX_bva                          },
//	{"ordY",                          ordY_bva                          },
//	{"ordZ",                          ordZ_bva                          },
//	{"polarOrdinate",                 polarOrdinate_bva                 },
//	{"radialLength",                  radialLength_bva                  },
//	{"angleX",                        angleX_bva                        },
//	{"angleY",                        angleY_bva                        },
//	{"angleZ",                        angleZ_bva                        },
//	{"point2D",                       point2D_bva                       },
//	{"point3D",                       point3D_bva                       },
//	{"direction",                     direction_bva                     },
//	{"orientation",                   orientation_bva                   },
//	{"direction3D",                   direction3D_bva                   },
//	{"orientation3D",                 orientation3D_bva                 },
//	{"position3D",                    position3D_bva                    },
//	{"publishParam",                  publishParam_bva                  },
//	{"publishMinTime",                publishMinTime_bva                },
//	{"publishMaxTime",                publishMaxTime_bva                },
//	{"publishThreshold",              publishThreshold_bva              },
//	{"publishEnable",                 publishEnable_bva                 },
//	{"pollInterval",                  pollInterval_bva                  },
//	{"minPollInterval",               minPollInterval_bva               },
//	{"maxPollInterval",               maxPollInterval_bva               },
//	{"errorReport",                   errorReport_bva                   },
//	{"connectionDependent",           connectionDependent_bva           },
//	{"connectedSwitch",               connectedSwitch_bva               },
//	{"connectionReporter",            connectionReporter_bva            },
//	{"binding",                       binding_bva                       },
//	{"boundProperty",                 boundProperty_bva                 },
//	{"bindingAnchor",                 bindingAnchor_bva                 },
//	{"windowProperty",                windowProperty_bva                },
//	{"bindingMechanism",              bindingMechanism_bva              },
//	{"binder",                        binder_bva                        },
//	{"binderRef",                     binderRef_bva                     },
//	{"pushBindingMechanism",          pushBindingMechanism_bva          },
//	{"pullBindingMechanism",          pullBindingMechanism_bva          },
//	{"internalSlaveRef",              internalSlaveRef_bva              },
//	{"internalMasterRef",             internalMasterRef_bva             },
//	{"internalBidiRef",               internalBidiRef_bva               },
//	{"DMPbinding",                    DMPbinding_bva                    },
//	{"DMPsetPropBinding",             DMPsetPropBinding_bva             },
//	{"DMPgetPropBinding",             DMPgetPropBinding_bva             },
//	{"DMPeventBinding",               DMPeventBinding_bva               },
//	{"bindingState",                  bindingState_bva                  },
//	{"xenoPropRef",                   xenoPropRef_bva                   },
//	{"xenoPropertyReference",         xenoPropertyReference_bva         },
//	{"xenoBinder",                    xenoBinder_bva                    },
//	{"accessWindow",                  accessWindow_bva                  },
//	{"accessMatch",                   accessMatch_bva                   },
//	{"accessEnable",                  accessEnable_bva                  },
//	{"accessInhibit",                 accessInhibit_bva                 },
//	{"dynamicAccessEnable",           dynamicAccessEnable_bva           },
//	{"connectionMatch",               connectionMatch_bva               },
//	{"contextMatchWindow",            contextMatchWindow_bva            },
//	{"autoAssignContextWindow",       autoAssignContextWindow_bva       },
//	{"preferredValue.abstract",       preferredValue_abstract_bva       },
//	{"preferredValue",                preferredValue_bva                },
//	{"repeatPrefVal",                 repeatPrefVal_bva                 },
//	{"repeatPrefValOffset",           repeatPrefValOffset_bva           },
//	{"selected",                      selected_bva                      },
//	{"selector",                      selector_bva                      },
//	{"choice",                        choice_bva                        },
//	{"enumSelector",                  enumSelector_bva                  },
//	{"fractionalSelector",            fractionalSelector_bva            },
//	{"positionalSelector",            positionalSelector_bva            },
//	{"case",                          case_bva                          },
//	{"cyclicPath",                    cyclicPath_bva                    },
//	{"cyclicPath.increasing",         cyclicPath_increasing_bva         },
//	{"cyclicPath.decreasing",         cyclicPath_decreasing_bva         },
//	{"cyclicPath.shortest",           cyclicPath_shortest_bva           },
//	{"cyclicPath.scalar",             cyclicPath_scalar_bva             },
//	{"cyclicDir.increasing",          cyclicDir_increasing_bva          },
//	{"cyclicDir.decreasing",          cyclicDir_decreasing_bva          },
//	{"cyclicDir.shortest",            cyclicDir_shortest_bva            },
//	{"connectedState",                connectedState_bva                },
//	{"autoConnectedState",            autoConnectedState_bva            },
//	{"explicitConnectedState",        explicitConnectedState_bva        },
//	{"writeConnectedState",           writeConnectedState_bva           },
//	{"readConnectedState",            readConnectedState_bva            },
//	{"autoTrackedConnection",         autoTrackedConnection_bva         },
//	{"trackTargetRef",                trackTargetRef_bva                },
//	{"loadOnAction",                  loadOnAction_bva                  },
//	{"actionSpecifier",               actionSpecifier_bva               },
//	{"actionProperty",                actionProperty_bva                },
//	{"propertyActionSpecifier",       propertyActionSpecifier_bva       },
//	{"propertyLoadAction",            propertyLoadAction_bva            },
//	{"propertyChangeAction",          propertyChangeAction_bva          },
//	{"actionState",                   actionState_bva                   },
//	{"actionStateBefore",             actionStateBefore_bva             },
//	{"actionStateAfter",              actionStateAfter_bva              },
//	{"initializer",                   initializer_bva                   },
//	{"initializationState",           initializationState_bva           },
//	{"initialization.enum",           initialization_enum_bva           },
//	{"initializationBool",            initializationBool_bva            },
//	{"refInArray",                    refInArray_bva                    },
//	{"rangeOver",                     rangeOver_bva                     },
//	{"contextDependent",              contextDependent_bva              },
//	{"controllerContextDependent",    controllerContextDependent_bva    },
//	{"connectionContextDependent",    connectionContextDependent_bva    },
//	{"netInterface",                  netInterface_bva                  },
//	{"netInterfaceItem",              netInterfaceItem_bva              },
//	{"netInterfaceRef",               netInterfaceRef_bva               },
//	{"netCarrierRef",                 netCarrierRef_bva                 },
//	{"accessNetInterface",            accessNetInterface_bva            },
//	{"netInterfaceDirection",         netInterfaceDirection_bva         },
//	{"netAddress",                    netAddress_bva                    },
//	{"myNetAddress",                  myNetAddress_bva                  },
//	{"routerAddress",                 routerAddress_bva                 },
//	{"serviceAddress",                serviceAddress_bva                },
//	{"netInterfaceState",             netInterfaceState_bva             },
//	{"netInterfaceIEEE802.3",         netInterfaceIEEE802_3_bva         },
//	{"netInterfaceIEEE802.11",        netInterfaceIEEE802_11_bva        },
//	{"netAddressIEEE-EUI",            netAddressIEEE_EUI_bva            },
//	{"netIfaceIPv4",                  netIfaceIPv4_bva                  },
//	{"netAddressIPv4",                netAddressIPv4_bva                },
//	{"netAddressIPv6",                netAddressIPv6_bva                },
//	{"netMask",                       netMask_bva                       },
//	{"netMaskIPv4",                   netMaskIPv4_bva                   },
//	{"netNetworkAddress",             netNetworkAddress_bva             },
//	{"netHostAddress",                netHostAddress_bva                },
//	{"myAddressDHCP",                 myAddressDHCP_bva                 },
//	{"myAddressLinkLocal",            myAddressLinkLocal_bva            },
//	{"myAddressStatic",               myAddressStatic_bva               },
//	{"defaultRouteAddress",           defaultRouteAddress_bva           },
//	{"DHCPserviceAddress",            DHCPserviceAddress_bva            },
//	{"DHCPLeaseTime",                 DHCPLeaseTime_bva                 },
//	{"DHCPLeaseRemaining",            DHCPLeaseRemaining_bva            },
//	{"DHCPclientState",               DHCPclientState_bva               },
//	{"netInterfaceDMX512",            netInterfaceDMX512_bva            },
//	{"universeIdDMX512",              universeIdDMX512_bva              },
//	{"netInterfaceDMX512pair",        netInterfaceDMX512pair_bva        },
//	{"netDMX512-XLRpri",              netDMX512_XLRpri_bva              },
//	{"netDMX512-XLRsec",              netDMX512_XLRsec_bva              },
//	{"netIfaceE1.31",                 netIfaceE1_31_bva                 },
//	{"universeIdE1.31",               universeIdE1_31_bva               },
//	{"slotAddressDMX512",             slotAddressDMX512_bva             },
//	{"baseAddressDMX512",             baseAddressDMX512_bva             },
//	{"STARTCode",                     STARTCode_bva                     },
//	{"DMXpropRef",                    DMXpropRef_bva                    },
//	{"DMXpropRef-SC0",                DMXpropRef_SC0_bva                },
//	{"bindingDMXnull",                bindingDMXnull_bva                },
//	{"bindingDMXalt-refresh",         bindingDMXalt_refresh_bva         },
//	{"streamGroup",                   streamGroup_bva                   },
//	{"streamPoint",                   streamPoint_bva                   },
//	{"streamMeasure",                 streamMeasure_bva                 },
//	{"streamSource",                  streamSource_bva                  },
//	{"streamInput",                   streamInput_bva                   },
//	{"streamOuput",                   streamOuput_bva                   },
//	{"streamCoverter",                streamCoverter_bva                },
//	{"beamGroup",                     beamGroup_bva                     },
//	{"beamDiverter",                  beamDiverter_bva                  },
//	{"lightSource",                   lightSource_bva                   },
//	{"colorSpec",                     colorSpec_bva                     },
//	{"colorFilter",                   colorFilter_bva                   },
//	{"beamShape",                     beamShape_bva                     },
//	{"beamTemplate",                  beamTemplate_bva                  },
//	{"opticalLens",                   opticalLens_bva                   },
//	{"simplified-specialized",        simplified_specialized_bva        },

//	{"5def7c40-35c1-11df-b42f-0017316c497d", NULL},  /* acnbaseExt1 */
//	{"advisory",                      advisory_bva                      },
//	{"monitor",                       monitor_bva                       },
//	{"associationRef",                associationRef_bva                },
//	{"valid-state",                   valid_state_bva                   },
//	{"connection.ESTA.DMX512",        connection_ESTA_DMX512_bva        },
//	{"connection.DMX512.sysUniverse", connection_DMX512_sysUniverse_bva },
//	{"connection.E131sourceCID",      connection_E131sourceCID_bva      },
//	{"connection.E131sourceName",     connection_E131sourceName_bva     },
//	{"DMX512streamPriority",          DMX512streamPriority_bva          },
//	{"slotOffsetDMX512",              slotOffsetDMX512_bva              },
//	{"proportional",                  proportional_bva                  },
//	{"drivenDirect",                  drivenDirect_bva                  },
//	{"lightSource-voltageDriven",     lightSource_voltageDriven_bva     },
//	{"beamStop",                      beamStop_bva                      },
//	{"apertureStop",                  apertureStop_bva                  },
//	{"circularAperture",              circularAperture_bva              },
//	{"edgeStop",                      edgeStop_bva                      },
//	{"edgeStop-polar",                edgeStop_polar_bva                },
//	{"edgeStop-straight",             edgeStop_straight_bva             },
//	{"enumerationByName",             enumerationByName_bva             },
//	{"externalNamespace",             externalNamespace_bva             },
//	{"namespaceIdentifier",           namespaceIdentifier_bva           },
//	{"namedTemplate",                 namedTemplate_bva                 },
//	{"dim-perTime1",                  dim_perTime1_bva                  },
//	{"perSecond1",                    perSecond1_bva                    },
//	{"dim-perTime2",                  dim_perTime2_bva                  },
//	{"perSecond2",                    perSecond2_bva                    },

//	{"102dbb3e-3120-11df-962e-0017316c497d", NULL},  /* artnet */
//	{"universeIdArtnet",              universeIdArtnet_bva              },

//	{"98ff901c-f910-11dc-9e36-000475d78133", NULL},  /* midi */
//	{"netInterfaceMIDI",              netInterfaceMIDI_bva              },
//	{"MIDImessage",                   MIDImessage_bva                   },
//	{"MIDIport-in",                   MIDIport_in_bva                   },
//	{"MIDIport-out",                  MIDIport_out_bva                  },

//	{"4ef14fd4-2e8d-11de-876f-0017316c497d", NULL},  /* sl */
//	{"simplifiedLighting",            simplifiedLighting_bva            },
//	{"pan",                           pan_bva                           },
//	{"tilt",                          tilt_bva                          },
//	{"beamDirRotate",                 beamDirRotate_bva                 },
//	{"beamDirAxis",                   beamDirAxis_bva                   },
//	{"beamDirection",                 beamDirection_bva                 },
//	{"movingSource",                  movingSource_bva                  },
//	{"movingSourcePan",               movingSourcePan_bva               },
//	{"movingSourceTilt",              movingSourceTilt_bva              },
//	{"movingMirror",                  movingMirror_bva                  },
//	{"beamDirMechanism",              beamDirMechanism_bva              },
//	{"focus",                         focus_bva                         },
//	{"movingLensFocus",               movingLensFocus_bva               },
//	{"movingMirrorFocus",             movingMirrorFocus_bva             },
//	{"shutterRotate",                 shutterRotate_bva                 },
//	{"shutterIn",                     shutterIn_bva                     },
//	{"iris",                          iris_bva                          },
//	{"imageRotateSpeed",              imageRotateSpeed_bva              },
//	{"imageRotatePosition",           imageRotatePosition_bva           },

	{"d88a9242-ba59-4d04-bbad-4710681aa9a1", NULL},
	{"languagesetID-resolvable",      languagesetID_bva                 },

	{NULL, NULL},
};
