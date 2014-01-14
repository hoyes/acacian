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
file: resolve.c

Resolve a UUID into a DDL file
*/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "acn.h"

/**********************************************************************/
/*
file: resolve.c

Resolve a UUID to a DDL module and return an open file descriptor
The returned file may be a socket or pipe

FIXME: The resolver should perform the following steps.

1. Check the file cache locations and known DDL directories and if the file 
   is present, open and return it.
2. Search for the file using the supplied URL list. The URL list may 
specify devices which must supply the file via TFTP (EPI-11), 
remote sites which may supply the file if an internet connection is 
available, other local machines which may cache DDL modules, etc.
3. Split the file if it contains multiple modules
4. Return an open file descriptior for the file

Currently it
only implements step one. It looks for the file in a supplied path, optionally
with one of the supplied extensions.

*/

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL
/**********************************************************************/
/*
func: openpath

Generic function that searches a path for a filename with one of a set of
extensions.

Returns:
an open file descriptor for the first matching file found, or -1 if
no match is found or the file cannot be opened.
Files are opened read only.
*/
int
openpath(const char *path, char *name, const char *exts)
{
	unsigned int namelen;
	unsigned int dirlen;
	unsigned int extlen;

	acnlogmark(lgDBUG, "\"%s\"  ->  \"%s\"  ->  \"%s\"", path, name, exts);

	namelen = strlen(name);
	/* ignore PATH if we've a directory */
	if (path == NULL || strchr(name, DIRSEP)) {
		path = "";
		dirlen = 0;
	} else {
		dirlen = strlen(path);
	}
	if (exts == NULL) {
		exts = "";
		extlen = 0;
	} else {
		extlen = strlen(exts);
	}
	do {
		/*
		ensure our buffer is big enough so we don't need to keep checking.
		add 1 for nul and 1 for DIRSEP
		*/
		char buf[dirlen + namelen + extlen + 2];
		const char *ep;
		char *dp;

		dp = buf;
		if (*path) {
			while (*path && *path != PATHSEP) *dp++ = *path++;
			*dp++ = DIRSEP;
		}
		dp = stpcpy(dp, name);

		ep = exts;
		do {
			char *cp;
			int fd;

			for (cp = dp; *ep && *ep != PATHSEP;) *cp++ = *ep++;
			*cp = 0;
			acnlogmark(lgDBUG, "try \"%s\"", buf);
			if ((fd = open(buf, O_RDONLY)) >= 0 || errno != ENOENT) return fd;
		} while (*ep++);
	} while (*path++);
	return -1;
}

/**********************************************************************/
const char default_path[] = ".:ddl";
#define DEFAULT_PATH_NDIRS 2

/**********************************************************************/
/*
func: openddlx

Open a ddl file or exit on failure.
To specify the path the environment is searched, first for 'DDL_PATH'.
If that is not found, the environment variable 'ACACIAN' (which should 
normally point to the top level of the Acacian source tree) is tried and
if found, the path is set to "$ACACIAN/.:$ACACIAN.ddl". If neither DDL_PATH 
nor ACACIAN exist the path is NULL and the name must specify the location
exactly.
The supplied name is tried with no extension, then with '.ddl' and '.xml'
in turn.
*/
int
openddlx(ddlchar_t *name)
{
	const char *path;
	char *mp = NULL;
	int fd;
	const char *nm;
	char buf[UUID_STR_SIZE];

	nm = name;
	if (str2uuid(name, NULL) == 0) {
		char *bp = buf;

		while (*bp++ = tolower(*nm++)) {}
		nm = buf;
	}

	path = getenv("DDL_PATH");
	acnlogmark(lgDBUG, "DDL_PATH \"%s\"", path);
	if (path == NULL) {
		char *ep;

		ep = getenv("ACACIAN");
		acnlogmark(lgDBUG, "ACACIAN \"%s\"", ep);
		
		if (ep) {
			const char *pp;
			char *cp;
			char c;

			pp = default_path;
			mp = malloc((strlen(ep) + 1) * DEFAULT_PATH_NDIRS + strlen(pp));
			if (mp == NULL) goto fail;

			cp = mp;
			c = PATHSEP;
			do {
				if (c == PATHSEP) {
					cp = stpcpy(cp, ep);
					*cp++ = DIRSEP;
				}
				c = *cp++ = *pp++;
			} while (c);
			path = mp;
		} else path = default_path;
	}
	fd = openpath(path, nm, ":.ddl:.xml");
	if (mp) free(mp);
	if (fd >= 0) return fd;

fail:
	acnlogerror(lgERR);
	exit(EXIT_FAILURE);
}

