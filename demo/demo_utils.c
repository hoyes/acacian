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
file: demo_utils.c

Utilities common to multiple demo programs.
*/


#include <sys/types.h>
//#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <slp.h>
#include "acn.h"
#include "demo_utils.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_APP

/**********************************************************************/
/*
UACN stuff
*/
const char cfgdir[] = "/.acacian/";
const char uacnsuffix[] = ".uacn";
char uacn[ACN_UACN_SIZE + 1];  /* allow for trailing newline */
extern const char uacn_dflt[];
char *uacncfg;  /* path and name of UACN config file */

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
void
uacn_init(const char *cidstr)
{
	char *cp;
	int fd;
	int len;

	cp = gethomedir();
	if (cp == NULL) {
		acnlogmark(lgERR, "Cannot find HOME directory");
		exit(EXIT_FAILURE);
	}
	uacncfg = mallocx(strlen(cp) + strlen(cfgdir) + UUID_STR_SIZE + strlen(uacnsuffix));
	sprintf(uacncfg, "%s%s%s%s", cp, cfgdir, cidstr, uacnsuffix);
	len = 0;
	if ((fd = open(uacncfg, O_RDONLY)) >= 0) {
		if ((len = read(fd, uacn, ACN_UACN_SIZE - 1)) > 0) {
			if (uacn[len - 1] == '\n') --len;
			uacn[len] = 0;
		}
		close(fd);
	}
	if (len <= 0) {
		strcpy(uacn, uacn_dflt);
	}
}
/**********************************************************************/
void
uacn_change(const uint8_t *dp, int size)
{
	int fd;
	int rslt;

	memcpy(uacn, dp, size);
	fd = open(uacncfg, O_WRONLY | O_TRUNC);
	if (fd >= 0) {
		uacn[size++] = '\n';
		if (write(fd, uacn, size) < size) {
			acnlogerror(lgERR);
		}
		close(fd);
		--size;
	}
	uacn[size] = 0;
	/* readvertise */
	if ((rslt = slp_register(ifMC(Lcomp))) < 0) {
		acnlogmark(lgERR, "Re-registering UACN: %s", slperrs[-rslt]);
	}
}
/**********************************************************************/
void
uacn_close()
{
	free(uacncfg);
}
