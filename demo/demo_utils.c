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
/*
section: DMP address array conversions

Convert between DMP property address ranges and Arrays suitable for
handling in C/C++

Because of the DMP addressing mechanism which allows arbitrary
increments between property eddresses and the DDL array declaration
method allowing nested arrays, the relationship between a DMP address 
and a particular element of an array property is not trivial.

Within these demonstrations any array property is treated as a C style
array with the number of dimensions derived from its DDL declaration
and with the DDL array declaration nearest the root as the outermost
dimension of the array. Thus a declaration like this:

| <property array="12"...>
|   ...
|   <property array="1000"...>
|   ...
|   </property>
| </property>

Is treated as a C array like this:

| array2D[12][1000].

Which may be "flattened" into a single dimensional array:

| array2D[column][row]  ==  array1D[column * 1000 + row]

func: addr2ofs

Convert a DMP address specifier to a property array offset.

arguments:
	dprop - the property within which the (first) address falls
	dmpads - the input DMP address specifier
	ofsads - the output offset address specifier (dmpads and ofsads 
	may point to the same structure, in which case the conversion is 
	made in situ).

returns:
	o -1 on failure, ofsads is not modified
	o 0 on success in which case ofsads->addr is the offset into the C array
	of the first element, ofsads->inc is the increment in the C array
	equivalent to the DMP property space increment and ofsads->count is the number of properties for which this
	address specifier is valid (minimum 1, maximum dmpads->count)
*/
int
addr2ofs(const struct dmpprop_s *dprop, struct adspec_s *dmpads, struct adspec_s *ofsads)
{
	uint32_t ofs;
	uint32_t INITIALIZED(inc);
	uint32_t maxprops;
	const struct dmpdim_s *dimp;
	int i;
	int incx;
	uint32_t ixs[dprop->ndims];

	LOG_FSTART();

	acnlogmark(lgDBUG, "Addr %u, inc %u, count %u", dmpads->addr, dmpads->inc, dmpads->count);
	ofs = dmpads->addr - dprop->addr;
	if (dprop->ndims == 0) {
		if (ofs != 0) return -1;
		ofsads->addr = ofs;
		ofsads->inc = 0;
		ofsads->count = 1;
		acnlogmark(lgDBUG, "Offset %u, inc %u, count %u", ofsads->addr, ofsads->inc, ofsads->count);
		LOG_FEND();
		return 0;
	}

	assert((dprop->flags & pflg(overlap)) == 0);  /* wont work for self overlapping dims */

	incx = -1;
	maxprops = 1;
	for (dimp = dprop->dim, i = 0; i < dprop->ndims; ++i, ++dimp) {
		ixs[i] = ofs / dimp->inc;
		ofs = ofs % dimp->inc;
		if (incx < 0 && dmpads->inc % dimp->inc == 0) {
			incx = i;
			inc = dmpads->inc / dimp->inc;
			if (inc == 0) {
				maxprops = dmpads->count;
			} else {
				maxprops = (dimp->cnt - 1 - ixs[i]) / inc + 1;
				if (maxprops > dmpads->count) maxprops = dmpads->count;
			}
		}
	}
	if (ofs != 0) return -1;
	dimp = dprop->dim + dprop->ndims - 1;
	ofs = ixs[dimp->tref];
	while (--dimp >= dprop->dim) {
		uint32_t cnt = dprop->dim[dimp->tref].cnt;
		ofs = ofs * cnt + ixs[dimp->tref];
		if (dimp->tref < incx) inc *= cnt;
	}

	ofsads->addr = ofs;
	ofsads->inc = inc;
	ofsads->count = maxprops;
	acnlogmark(lgDBUG, "Offset %u, inc %u, count %u", ofsads->addr, ofsads->inc, ofsads->count);
	return 0;
	LOG_FEND();
}

/**********************************************************************/
/*
func: ofs2addr

Convert a property array offset to a DMP address specifier.

arguments:
	dprop - the property within which the (first) address falls
	ofsads - the input offset address specifier
	dmpads - the output DMP address specifier (ofsads and dmpads 
	may point to the same structure, in which case the conversion is 
	made in situ).

*/
void
ofs2addr(const struct dmpprop_s *dprop, struct adspec_s *ofsads, struct adspec_s *dmpads)
{
	uint32_t ofs;
	uint32_t pinc;
	int i;

	LOG_FSTART();
	acnlogmark(lgDBUG, "Offset %u, inc %u, count %u", ofsads->addr, ofsads->inc, ofsads->count);
	ofs = ofsads->addr;
	if ((pinc = ofsads->inc) == 0) dmpads->inc = 0;
	dmpads->addr = dprop->addr;
	dmpads->count = ofsads->count;

	for (i = 0; i < dprop->ndims; ++i) {
		const struct dmpdim_s *dimp;
		
		dimp = dprop->dim + dprop->dim[i].tref;
		dmpads->addr += (ofs % dimp->cnt) * dimp->inc;
		ofs = ofs / dimp->cnt;
		if (pinc) {
			if (pinc < dimp->cnt) {
				dmpads->inc = pinc * dimp->inc;
				pinc = 0;
			} else {
				pinc = pinc / dimp->cnt;
			}
		}
	}

	acnlogmark(lgDBUG, "Addr %u, inc %u, count %u", dmpads->addr, dmpads->inc, dmpads->count);
	LOG_FEND();
}

/**********************************************************************/
/*
section: UACN management

The UACN (User Assigned Component Name) is specified in EPI-19 
(discovery). It must be persistent and since it forms part of the 
component's discovery attributes, the component must re-register 
with SLP if it changes.
*/

/**********************************************************************/
/*
prototypes
*/
char *gethomedir(void);
/**********************************************************************/
/*
func: uacn_init

Initialize our UACN from a file. If this fails assign the default value.
*/

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
/*
func: uacn_change

Change the value of our UACN. The new value is stored to file, then
the component must be re-advertised through SLP.
*/
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
/*
func: uacn_close

Call after a component is de-registered
*/

void
uacn_close()
{
	free(uacncfg);
}
/**********************************************************************/
/*
func: gethomedir

Returns a pointer to the current user's home directory. It first tries
the environment variable HOME, then looks up in the system passwd file.
*/

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

