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
/**********************************************************************/
/*
Logging level for this source file.
If not set it will default to the global CF_LOG_DEFAULT

options are

lgOFF lgEMRG lgALRT lgCRIT lgERR lgWARN lgNTCE lgINFO lgDBUG
*/
//#define LOGLEVEL lgDBUG

/**********************************************************************/
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>
#include "acn.h"

/**********************************************************************/
/*
file: resolve.c

Resolve a DCID (UUID) to a DDL module and return an open file 
descriptor. The returned file may be a socket or pipe.

FIXME:
The resolver should perform the following steps.

1. Check the file cache locations and known DDL directories and if the file 
   is present, open and return it.
2. Search for the file using the supplied URL list. The URL list may 
specify devices which must supply the file via TFTP (EPI-11), 
remote sites which may supply the file if an internet connection is 
available, other local machines which may cache DDL modules, etc.
3. Split the file if it contains multiple modules
4. Return an open file descriptior for the file

Currently it only implements step one. It looks for the file in a 
path, optionally with one of the supplied extensions.
*/
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
openpath(const char *path, const char *name, const char *exts)
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
const char default_path[] = "/.acacian/ddlcache";

/**********************************************************************/
char *
gethomedir(void)
{
	char *hp;
	struct passwd *pwd;
	
	if ((hp = getenv("HOME")) != NULL)
		return hp;

	if ((pwd = getpwuid(geteuid())) != NULL)
		return pwd->pw_dir;

	return NULL;
}
/**********************************************************************/
/*
func: openddlx

Open a ddl file or exit on failure.

If the supplied name looks like a UUID string it is converted to lower case 
which is the convention used for UUID file-names (should use a full 
case insensitive file search here). Then the path is searched for 
`name`, `name.ddl` or `name.xml`. If the file is fouind the opened 
file descriptor is returned. If it cannot be found openddlx() quits 
(should do better here!).

The path is given by the environment variable `DDL_PATH`. If this isn't
found then the default `$HOME/.acacian/ddlcache` is used.
*/
int
openddlx(ddlchar_t *name)
{
	const char *path;
	int fd;
	const char *nm;
	char buf[UUID_STR_SIZE];
	char dfpath[100];

	nm = name;
	if (str2uuid(name, NULL) == 0) {  /* is name a UUID? */
		char *bp = buf;

		while ((*bp++ = tolower(*nm++)) != 0) {}
		nm = buf;
	}

	if ((path = getenv("DDL_PATH")) == NULL
		&& (path = gethomedir()) != NULL
	) {
		char *cp;

		acnlogmark(lgDBUG, "constructing default path");
		cp = stpncpy(dfpath, path, sizeof(dfpath));
		cp = stpncpy(cp, default_path, dfpath + sizeof(dfpath) - cp);
		path = dfpath;
	}
	acnlogmark(lgDBUG, "DDL_PATH \"%s\"", path);
	fd = openpath(path, nm, ":.ddl:.xml");
	if (fd >= 0) return fd;

	acnlogerror(lgERR);
	exit(EXIT_FAILURE);
}

