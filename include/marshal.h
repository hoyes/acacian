/*--------------------------------------------------------------------*/
/*

Copyright (c) 2007, Pathway Connectivity Inc.

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

 * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
 * Neither the name of Pathway Connectivity Inc. nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INUUIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   $Id: marshal.h 357 2010-09-19 17:42:42Z philipnye $

#tabs=3s
*/
/*--------------------------------------------------------------------*/
#ifndef __marshal_h__
#define __marshal_h__ 1

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
/*
file: marshal.h

Marshal and unmarshal native types into packets

If configuration option <ACNCFG_MARSHAL_INLINE> is set these are defined
as inline functions and they are documented that way. If <ACNCFG_MARSHAL_INLINE>
is false then most are defined as macros.

WARNING:
Many of the marshal/unmarshal macros evaluate their arguments multiple times
*/

#if ACNCFG_MARSHAL_INLINE
#include "string.h"

static __inline uint8_t *marshalU8(uint8_t *data, uint8_t u8)
{
   *data++ = u8;
   return data;
}

static __inline uint8_t *marshalU16(uint8_t *data, uint16_t u16)
{
   data[0] = u16 >> 8;
   data[1] = (uint8_t)(u16);
   return data + 2;
}

static __inline uint8_t *marshalU32(uint8_t *data, uint32_t u32)
{
   data[0] = u32 >> 24;
   data[1] = (uint8_t)(u32 >> 16);
   data[2] = (uint8_t)(u32 >> 8);
   data[3] = (uint8_t)(u32);
   return data + 4;
}

static __inline uint8_t *marshalU64(uint8_t *data, uint64_t u64)
{
   data[0] = u64 >> 56;
   data[1] = (uint8_t)(u64 >> 48);
   data[2] = (uint8_t)(u64 >> 40);
   data[3] = (uint8_t)(u64 >> 32);
   data[4] = (uint8_t)(u64 >> 24);
   data[5] = (uint8_t)(u64 >> 16);
   data[6] = (uint8_t)(u64 >> 8);
   data[7] = (uint8_t)(u64);
   return data + 8;
}

static __inline uint8_t *marshalBytes(uint8_t *data, const uint8_t *src, int size)
{
   return (uint8_t *)memcpy(data, src, size) + size;
}

static __inline uint8_t *marshaluuid(uint8_t *data, const uint8_t *uuid)
{
   return (uint8_t *)memcpy(data, uuid, UUID_SIZE) + UUID_SIZE;
}

static __inline uint8_t *marshalVar(uint8_t *data, const uint8_t *src, uint16_t size)
{
   memcpy( marshalU16(data, size + 2), src, size);
   return data + size + 2;
}

/************************************************************************/

static __inline uint8_t unmarshalU8(const uint8_t *data)
{
   return *data;
}

static __inline uint16_t unmarshalU16(const uint8_t *data)
{
   return (data[0] << 8) | data[1];
}

static __inline uint32_t unmarshalU32(const uint8_t *data)
{
   return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | (data[2] << 8) | data[3];
}

static __inline uint64_t unmarshalU64(const uint8_t *data)
{
   return ((uint64_t)data[0] << 56) | ((uint64_t)data[1] << 48) | ((uint64_t)data[2] << 40) | ((uint64_t)data[3] << 32) | ((uint64_t)data[4] << 24) | ((uint64_t)data[5] << 16) | (data[6] << 8) | data[7];
}

static __inline const uint8_t *unmarshaluuid(const uint8_t *data, uint8_t *uuid)
{
   memcpy(uuid, data, UUID_SIZE);
   return data + UUID_SIZE;
}

static __inline const uint8_t *unmarshalBytes(const uint8_t *data, uint8_t *dest, int size)
{
   memcpy(dest, data, size);
   return data + size;
}

static __inline uint16_t unpackVar(const uint8_t *data, uint8_t *dest)
{
   uint16_t len = unmarshalU16(data) - 2;
   memcpy(dest, data + 2, len);
   return len;
}

#define unmarshal8(x)  ((int8_t)unmarshalU8(x))
#define unmarshal16(x)  ((int16_t)unmarshalU16(x))
#define unmarshal32(x)  ((int32_t)unmarshalU32(x))
#define unmarshal64(x)  ((int64_t)unmarshalU64(x))

#else
/*
WARNING
Many of the marshal/unmarshal macros evaluate their arguments multiple times
*/

#include <string.h>

#define marshalU8(datap, u8) (*(uint8_t *)(datap) = (uint8_t)(u8), (uint8_t *)(datap) + 1)
#define unmarshalU8(datap) (*(uint8_t *)(datap))
#define unmarshal8(datap) (*(int8_t *)(datap))

#define marshalU16(datap, u16) (\
               ((uint8_t *)(datap))[0] = (uint8_t)((u16) >> 8), \
               ((uint8_t *)(datap))[1] = (uint8_t)((u16) >> 0), \
               (uint8_t *)(datap) + 2)

#define unmarshal16(datap) (int16_t)(\
               ((uint8_t *)(datap))[0] << 8 \
               | ((uint8_t *)(datap))[1])

#define unmarshalU16(datap) (uint16_t)(\
               ((uint8_t *)(datap))[0] << 8 \
               | ((uint8_t *)(datap))[1])

#define marshalU32(datap, u32) (\
               ((uint8_t *)(datap))[0] = (uint8_t)((u32) >> 24), \
               ((uint8_t *)(datap))[1] = (uint8_t)((u32) >> 16), \
               ((uint8_t *)(datap))[2] = (uint8_t)((u32) >> 8), \
               ((uint8_t *)(datap))[3] = (uint8_t)((u32) >> 0), \
               (uint8_t *)(datap) + 4)

#define unmarshal32(datap)  (int32_t)(\
               ((uint8_t *)(datap))[0] << 24 \
               | ((uint8_t *)(datap))[1] << 16 \
               | ((uint8_t *)(datap))[2] << 8 \
               | ((uint8_t *)(datap))[3])

#define unmarshalU32(datap)  (uint32_t)(\
               ((uint8_t *)(datap))[0] << 24 \
               | ((uint8_t *)(datap))[1] << 16 \
               | ((uint8_t *)(datap))[2] << 8 \
               | ((uint8_t *)(datap))[3])

#define marshalBytes(data, src, size) ((uint8_t*)memcpy((uint8_t *)(data), (uint8_t *)(src), (size)) + (size))
#define unmarshalBytes(data, dest, size) (memcpy((uint8_t *)(dest), (uint8_t *)(data), (size)), (data) + (size))

#define marshalCID(data, cid) ((uint8_t*)memcpy((uint8_t *)(data), (uint8_t *)(cid), sizeof(cid_t)) + CIDSIZE)
#define unmarshalCID(data, cid) (memcpy((uint8_t *)(cid), (uint8_t *)(data), sizeof(cid_t)), (data) + CIDSIZE)

#define marshalVar(datap, srcp, len) \
               ( \
                  memcpy( \
                     marshalU16(((uint8_t *)(datap)), ((uint16_t)(len)) + 2), \
                     ((uint8_t *)(srcp)), \
                     ((uint16_t)(len)) \
                  ) + ((uint16_t)(len)) \
               )

#define unpackVar(data, dest) \
				( \
					memcpy(dest, data + 2, unmarshalU16(data) - 2), \
					unmarshalU16(data) - 2 \
				)

#endif

/*
static macros for marshalling into arrays
*/

#define stmarshal16(x) (((x) >> 8) & 0xff), ((x) & 0xff)
#define stmarshal32(x) (((x) >> 24) & 0xff), (((x) >> 16) & 0xff), (((x) >> 8) & 0xff), ((x) & 0xff)


#ifdef __cplusplus
}
#endif


#endif	/* __marshal_h__ */
