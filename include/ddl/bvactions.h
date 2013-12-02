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
header: bvactions.h

Handlers for specific DDL behaviors.

For each behavior action function defined in <bvactions.c>, define a
BVA_behaviorset_behaviorname macro
for every behavior which calls that action - these macros then 
create the appropriate entries in the known_bvs table.

*/

#ifndef __bvactions_h__
#define __bvactions_h__ 1

#define bva_func(x) void x ## _bva(struct dcxt_s *dcxp, const struct bv_s *bv)

bva_func( null              );
bva_func( abstract          );
bva_func( persistent        );
bva_func( constant          );
bva_func( volatile          );
bva_func( et_boolean        );
bva_func( et_sint           );
bva_func( et_uint           );
bva_func( et_float          );
bva_func( et_UTF8           );
bva_func( et_UTF16          );
bva_func( et_UTF32          );
bva_func( et_string         );
bva_func( et_enum           );
bva_func( et_opaque_fixsize );
bva_func( et_opaque_varsize );
bva_func( et_uuid           );
bva_func( et_bitmap         );
bva_func( persist_string    );
bva_func( const_string      );

#define BVA_acnbase_NULL                        null_bva
#define BVA_acnbase_r2_NULL                     null_bva
#define BVA_sl_simplifiedLighting               abstract_bva
#define BVA_acnbase_typingPrimitive             abstract_bva
#define BVA_acnbase_reference                   abstract_bva
#define BVA_acnbase_encoding                    abstract_bva
#define BVA_acnbase_type_floating_point         abstract_bva
#define BVA_acnbase_accessClass                 abstract_bva
#define BVA_acnbase_atomicLoad                  abstract_bva
#define BVA_acnbase_algorithm                   abstract_bva
#define BVA_acnbase_time                        abstract_bva
#define BVA_acnbase_date                        abstract_bva
#define BVA_acnbase_propertyRef                 abstract_bva
#define BVA_acnbase_scale                       abstract_bva
#define BVA_acnbase_rate                        abstract_bva
#define BVA_acnbase_direction                   abstract_bva
#define BVA_acnbase_orientation                 abstract_bva
#define BVA_acnbase_publishParam                abstract_bva
#define BVA_acnbase_connectionDependent         abstract_bva
#define BVA_acnbase_pushBindingMechanism        abstract_bva
#define BVA_acnbase_pullBindingMechanism        abstract_bva
#define BVA_acnbase_DMPbinding                  abstract_bva
#define BVA_acnbase_preferredValue_abstract     abstract_bva
#define BVA_acnbase_cyclicPath                  abstract_bva
#define BVA_acnbase_streamFilter                abstract_bva
#define BVA_acnbase_beamDiverter                abstract_bva
#define BVA_acnbase_enumeration                 abstract_bva
#define BVA_acnbase_boolean                     abstract_bva
#define BVA_acnbase_r2_abstractPriority         abstract_bva
#define BVA_acnbase_r2_typingPrimitive          abstract_bva
#define BVA_acnbase_r2_reference                abstract_bva
#define BVA_acnbase_r2_encoding                 abstract_bva
#define BVA_acnbase_r2_type_floating_point      abstract_bva
#define BVA_acnbase_r2_accessClass              abstract_bva
#define BVA_acnbase_r2_atomicLoad               abstract_bva
#define BVA_acnbase_r2_algorithm                abstract_bva
#define BVA_acnbase_r2_time                     abstract_bva
#define BVA_acnbase_r2_date                     abstract_bva
#define BVA_acnbase_r2_propertyRef              abstract_bva
#define BVA_acnbase_r2_scale                    abstract_bva
#define BVA_acnbase_r2_rate                     abstract_bva
#define BVA_acnbase_r2_direction                abstract_bva
#define BVA_acnbase_r2_orientation              abstract_bva
#define BVA_acnbase_r2_publishParam             abstract_bva
#define BVA_acnbase_r2_connectionDependent      abstract_bva
#define BVA_acnbase_r2_pushBindingMechanism     abstract_bva
#define BVA_acnbase_r2_pullBindingMechanism     abstract_bva
#define BVA_acnbase_r2_DMPbinding               abstract_bva
#define BVA_acnbase_r2_preferredValue_abstract  abstract_bva
#define BVA_acnbase_r2_cyclicPath               abstract_bva
#define BVA_acnbase_r2_enumeration              abstract_bva
#define BVA_acnbase_r2_boolean                  abstract_bva
#define BVA_acnbase_persistent                  persistent_bva
#define BVA_acnbase_r2_persistent               persistent_bva
#define BVA_acnbase_constant                    constant_bva
#define BVA_acnbase_r2_constant                 constant_bva
#define BVA_acnbase_volatile                    volatile_bva
#define BVA_acnbase_r2_volatile                 volatile_bva
#define BVA_acnbase_type_boolean                et_boolean_bva
#define BVA_acnbase_r2_type_boolean             et_boolean_bva
#define BVA_acnbase_type_signed_integer         et_sint_bva
#define BVA_acnbase_type_sint                   et_sint_bva
#define BVA_acnbase_r2_type_signed_integer      et_sint_bva
#define BVA_acnbase_r2_type_sint                et_sint_bva
#define BVA_acnbase_type_unsigned_integer       et_uint_bva
#define BVA_acnbase_type_uint                   et_uint_bva
#define BVA_acnbase_r2_type_unsigned_integer    et_uint_bva
#define BVA_acnbase_r2_type_uint                et_uint_bva
#define BVA_acnbase_r2_type_float               et_float_bva
#define BVA_acnbase_type_float                  et_float_bva
#define BVA_acnbase_r2_type_char_UTF_8          et_UTF8_bva
#define BVA_acnbase_type_char_UTF_8             et_UTF8_bva
#define BVA_acnbase_r2_type_char_UTF_16         et_UTF16_bva
#define BVA_acnbase_type_char_UTF_16            et_UTF16_bva
#define BVA_acnbase_r2_type_char_UTF_32         et_UTF32_bva
#define BVA_acnbase_type_char_UTF_32            et_UTF32_bva
#define BVA_acnbase_r2_type_string              et_string_bva
#define BVA_acnbase_type_string                 et_string_bva
#define BVA_acnbase_r2_type_enumeration         et_enum_bva
#define BVA_acnbase_type_enumeration            et_enum_bva
#define BVA_acnbase_r2_type_enum                et_enum_bva
#define BVA_acnbase_type_enum                   et_enum_bva
#define BVA_acnbase_r2_type_fixBinob            et_opaque_fixsize_bva
#define BVA_acnbase_type_fixBinob               et_opaque_fixsize_bva
#define BVA_acnbase_r2_type_varBinob            et_opaque_varsize_bva
#define BVA_acnbase_type_varBinob               et_opaque_varsize_bva
#define BVA_acnbase_r2_UUID                     et_uuid_bva
#define BVA_acnbase_UUID                        et_uuid_bva
#define BVA_acnbase_r2_type_bitmap              et_bitmap_bva
#define BVA_acnbase_UACN                        persist_string_bva
#define BVA_acnbase_r2_UACN                     persist_string_bva
#define BVA_acnbase_FCTN                        const_string_bva
#define BVA_acnbase_r2_FCTN                     const_string_bva
#define BVA_acnbase_r2_FCTNstring               et_string_bva
#define BVA_acnbase_manufacturer                const_string_bva
#define BVA_acnbase_r2_manufacturerURL          const_string_bva
#define BVA_acnbase_maunfacturerURL             const_string_bva
#define BVA_acnbase_hardwareVersion             const_string_bva
#define BVA_acnbase_r2_hardwareVersion          const_string_bva
#define BVA_acnbase_softwareVersion             const_string_bva
#define BVA_acnbase_r2_softwareVersion          const_string_bva
#define BVA_acnbase_devSerialNo                 const_string_bva
#define BVA_acnbase_r2_devSerialNo              const_string_bva


#endif /* __bvactions_h__ */
