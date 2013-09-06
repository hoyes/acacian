/**********************************************************************/
/*

	Copyright (C) 2011, Engineering Arts. All rights reserved.

	Author: Philip Nye

	$Id$

#tabs=3
*/
/**********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "acn.h"

/**********************************************************************/
/*
Resolve a UUID to a DDL module and return an open file descriptor
The returned file may be a socket or pipe

FIXME: Currently this simply looks for the file in a single compiled-in
Cache location. This needs to be expanded to:
1. Check the cache and if the file is present, open and return it
2. Search for the file using the supplied URL list. The URL list may 
specify local devices which must supply the file via TFTP (epi11), 
remote sites which may supply the file if an internet connection is 
available, other local machines which may cache DDL modules, etc.
3. Split the file if it contains multiple modules
4. Return an open file descriptior for the file
*/

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL
/**********************************************************************/
const ddlchar_t ddlpath_default[] = "/home/philip/engarts/acn/ddl/modules/ddl-modules:/home/philip/engarts/dev/eaacn/eaacn/demo";

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

int
openddlx(ddlchar_t *name)
{
	const char *path;
	char *mp = NULL;
	int fd;

	path = getenv("DDL_PATH");
	acnlogmark(lgDBUG, "DDL_PATH \"%s\"", path);
	if (path == NULL) {
		char *ep;

		ep = getenv("EAACN");
		acnlogmark(lgDBUG, "EAACN \"%s\"", ep);
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
	fd = openpath(path, name, ":.ddl:.xml");
	if (mp) free(mp);
	if (fd >= 0) return fd;

fail:
	acnlogerror(lgERR);
	exit(EXIT_FAILURE);
}

