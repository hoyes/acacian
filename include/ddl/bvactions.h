/**********************************************************************/
/*

	Copyright (C) 2012, Engineering Arts. All rights reserved.

	Author: Philip Nye

	$Id$

#tabs=4s
*/
/**********************************************************************/

#ifndef __bvactions_h__
#define __bvactions_h__ 1

void null_bvaction(struct dcxt_s *dcxp, const struct bv_s *bv);
void abstract_bvaction(struct dcxt_s *dcxp, const struct bv_s *bv);
void persistent_bvaction(struct dcxt_s *dcxp, const struct bv_s *bv);
void constant_bvaction(struct dcxt_s *dcxp, const struct bv_s *bv);
void volatile_bvaction(struct dcxt_s *dcxp, const struct bv_s *bv);
void et_boolean_bva(struct dcxt_s *dcxp, const struct bv_s *bv);
void et_sint_bva(struct dcxt_s *dcxp, const struct bv_s *bv);
void et_uint_bva(struct dcxt_s *dcxp, const struct bv_s *bv);
void et_float_bva(struct dcxt_s *dcxp, const struct bv_s *bv);
void et_UTF8_bva(struct dcxt_s *dcxp, const struct bv_s *bv);
void et_UTF16_bva(struct dcxt_s *dcxp, const struct bv_s *bv);
void et_UTF32_bva(struct dcxt_s *dcxp, const struct bv_s *bv);
void et_string_bva(struct dcxt_s *dcxp, const struct bv_s *bv);
void et_enum_bva(struct dcxt_s *dcxp, const struct bv_s *bv);
void et_opaque_fixsize_bva(struct dcxt_s *dcxp, const struct bv_s *bv);
void et_opaque_varsize_bva(struct dcxt_s *dcxp, const struct bv_s *bv);
void et_uuid_bva(struct dcxt_s *dcxp, const struct bv_s *bv);
void et_bitmap_bva(struct dcxt_s *dcxp, const struct bv_s *bv);

#define BVA_acnbase_NULL                        null_bvaction
#define BVA_acnbase_r2_NULL                     null_bvaction
#define BVA_sl_simplifiedLighting               abstract_bvaction
#define BVA_acnbase_typingPrimitive             abstract_bvaction
#define BVA_acnbase_reference                   abstract_bvaction
#define BVA_acnbase_encoding                    abstract_bvaction
#define BVA_acnbase_type_floating_point         abstract_bvaction
#define BVA_acnbase_accessClass                 abstract_bvaction
#define BVA_acnbase_atomicLoad                  abstract_bvaction
#define BVA_acnbase_algorithm                   abstract_bvaction
#define BVA_acnbase_time                        abstract_bvaction
#define BVA_acnbase_date                        abstract_bvaction
#define BVA_acnbase_propertyRef                 abstract_bvaction
#define BVA_acnbase_scale                       abstract_bvaction
#define BVA_acnbase_rate                        abstract_bvaction
#define BVA_acnbase_direction                   abstract_bvaction
#define BVA_acnbase_orientation                 abstract_bvaction
#define BVA_acnbase_publishParam                abstract_bvaction
#define BVA_acnbase_connectionDependent         abstract_bvaction
#define BVA_acnbase_pushBindingMechanism        abstract_bvaction
#define BVA_acnbase_pullBindingMechanism        abstract_bvaction
#define BVA_acnbase_DMPbinding                  abstract_bvaction
#define BVA_acnbase_preferredValue_abstract     abstract_bvaction
#define BVA_acnbase_cyclicPath                  abstract_bvaction
#define BVA_acnbase_streamFilter                abstract_bvaction
#define BVA_acnbase_beamDiverter                abstract_bvaction
#define BVA_acnbase_enumeration                 abstract_bvaction
#define BVA_acnbase_boolean                     abstract_bvaction
#define BVA_acnbase_r2_abstractPriority         abstract_bvaction
#define BVA_acnbase_r2_typingPrimitive          abstract_bvaction
#define BVA_acnbase_r2_reference                abstract_bvaction
#define BVA_acnbase_r2_encoding                 abstract_bvaction
#define BVA_acnbase_r2_type_floating_point      abstract_bvaction
#define BVA_acnbase_r2_accessClass              abstract_bvaction
#define BVA_acnbase_r2_atomicLoad               abstract_bvaction
#define BVA_acnbase_r2_algorithm                abstract_bvaction
#define BVA_acnbase_r2_time                     abstract_bvaction
#define BVA_acnbase_r2_date                     abstract_bvaction
#define BVA_acnbase_r2_propertyRef              abstract_bvaction
#define BVA_acnbase_r2_scale                    abstract_bvaction
#define BVA_acnbase_r2_rate                     abstract_bvaction
#define BVA_acnbase_r2_direction                abstract_bvaction
#define BVA_acnbase_r2_orientation              abstract_bvaction
#define BVA_acnbase_r2_publishParam             abstract_bvaction
#define BVA_acnbase_r2_connectionDependent      abstract_bvaction
#define BVA_acnbase_r2_pushBindingMechanism     abstract_bvaction
#define BVA_acnbase_r2_pullBindingMechanism     abstract_bvaction
#define BVA_acnbase_r2_DMPbinding               abstract_bvaction
#define BVA_acnbase_r2_preferredValue_abstract  abstract_bvaction
#define BVA_acnbase_r2_cyclicPath               abstract_bvaction
#define BVA_acnbase_r2_enumeration              abstract_bvaction
#define BVA_acnbase_r2_boolean                  abstract_bvaction
#define BVA_acnbase_persistent                  persistent_bvaction
#define BVA_acnbase_r2_persistent               persistent_bvaction
#define BVA_acnbase_constant                    constant_bvaction
#define BVA_acnbase_r2_constant                 constant_bvaction
#define BVA_acnbase_volatile                    volatile_bvaction
#define BVA_acnbase_r2_volatile                 volatile_bvaction
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

#endif /* __bvactions_h__ */
