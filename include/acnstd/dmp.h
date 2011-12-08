/* vi: set sw=4 ts=4: */
/************************************************************************/
/*

   Copyright (c) 2011, Philip Nye, Engineering Arts (UK) philip@engarts.com
   All rights reserved.

   Author: Philip Nye

   $Id$

*/
/************************************************************************/

#ifndef __acnstd_dmp_h__
#define __acnstd_dmp_h__ 1

/* get the protocol identifiers */
#include "acnstd/protocols.h"

#define DMP_VECTOR_LEN 1
#define DMP_HEADER_LEN 1

enum {
	DMPAD_A0 = 0x01,
	DMPAD_A1 = 0x02,
	DMPAD_X0 = 0x04,
	DMPAD_X1 = 0x08,
	DMPAD_D0 = 0x10,
	DMPAD_D1 = 0x20,
	DMPAD_R  = 0x40,
	DMPAD_Z  = 0x80,
	/* these bits must be zero */
    DMPAD_ZMASK = (DMPAD_Z | DMPAD_X1 | DMPAD_X0)
};

enum
{
	DMPAD_SINGLE       = 0,
	DMPAD_RANGE_NODATA = 0x10,
	DMPAD_RANGE_SINGLE = 0x10,
	DMPAD_RANGE_ARRAY  = 0x20,
	DMPAD_RANGE_STRUCT = 0x30,
	DMPAD_TYPEMASK     = 0x30
};

enum {
	DMPAD_1BYTE = 0,
	DMPAD_2BYTE = 1,
	DMPAD_4BYTE = 2,
	DMPAD_BADSIZE = 3,
	DMPAD_SIZEMASK = 3
};

#define ADDR_SIZE(adtype) (((adtype) & ADDRESS_SIZE_MASK) + 1 + (((adtype) & ADDRESS_SIZE_MASK) == 2))

/* Reason codes [DMP spec] */
enum dmp_reason_e
{
	DMPRC_SUCCESS = 0,
	DMPRC_UNSPECIFIED = 1,
	DMPRC_NOSUCHPROP = 2,
	DMPRC_NOREAD = 3,
	DMPRC_NOWRITE = 4,
	DMPRC_BADDATA = 5,
	DMPRC_NOEVENT = 10,
	DMPRC_NOSUBSCRIBE = 11,
	DMPRC_NORESOURCES = 12,
	DMPRC_NOPERMISSION = 13,
	DMPRC_MAXINC = 13
};

/* DMP messsage types (commands) */
enum dmp_message_e
{
	DMP_reserved0             = 0,
	DMP_GET_PROPERTY          = 1,
	DMP_SET_PROPERTY          = 2,
	DMP_GET_PROPERTY_REPLY    = 3,
	DMP_EVENT                 = 4,
	DMP_reserved5             = 5,
	DMP_reserved6             = 6,
	DMP_SUBSCRIBE             = 7,
	DMP_UNSUBSCRIBE           = 8,
	DMP_GET_PROPERTY_FAIL     = 9,
	DMP_SET_PROPERTY_FAIL     = 10,
	DMP_reserved11            = 11,
	DMP_SUBSCRIBE_ACCEPT      = 12,
	DMP_SUBSCRIBE_REJECT      = 13,
	DMP_reserved14            = 14,
	DMP_reserved15            = 15,
	DMP_reserved16            = 16,
	DMP_SYNC_EVENT            = 17
};


#endif	/* __acnstd_dmp_h__ */
