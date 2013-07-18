/**********************************************************************/
/*

	Copyright (C) 2011, Engineering Arts. All rights reserved.

	Author: Philip Nye

	$Id$

#tabs=3
*/
/**********************************************************************/

#include <stdlib.h>
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
const ddlchar_t ddl_location[] = "/home/philip/engarts/acn/ddl/modules/ddl-modules/%";

int
openddlx(uint8_t *uuid, ddlchar_t *urls[])
{
	ddlchar_t fname[sizeof(ddl_location) + UUID_STR_SIZE];
	const ddlchar_t *sp;
	ddlchar_t *dp;
	ddlchar_t c;
	int fd;

	sp = ddl_location;
	dp = fname;
	
	while ((c = *sp++) != '%') *dp++ = c;
	uuid2str(uuid, dp);
	dp += UUID_STR_SIZE - 1;
	while ((*dp++ = *sp++));
	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		acnlogerror(lgERR);
		exit(EXIT_FAILURE);
	}
	return fd;
}

