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
header: resolve.h

UUID to DDL file resolver. Header for <resolve.c>
*/

#ifndef __resolve_h__
#define __resolve_h__ 1

int openpath(const char *path, const char *name, const char *exts);
int openddlx(ddlchar_t *name);

#endif  /* __resolve_h__ */
