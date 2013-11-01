/**********************************************************************/
/*
Copyright (c) 2013, Philip Nye
All rights reserved.

	$Id$

#tabs=3t
*/
/**********************************************************************/

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
uacn_change(const uint8_t *dp, int size, const char *interfaces[])
{
	int fd;
	int rslt;

	memcpy(uacn, dp, size);
	uacn[size++] = '\n';
	uacn[size] = 0;
	fd = open(uacncfg, O_WRONLY);
	if (fd >= 0) {
		if (write(fd, uacn, size) < size) {
			acnlogerror(lgERR);
		}
		close(fd);
	}
	/* readvertise */
	if ((rslt = slp_register(ifMC(Lcomp,) interfaces)) < 0) {
		acnlogmark(lgERR, "Re-registering UACN: %s", slperrs[-rslt]);
	}
}
/**********************************************************************/
void
uacn_close()
{
	free(uacncfg);
}
