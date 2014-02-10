/*
This file forms part of Acacian a full featured implementation of 
ANSI E1.17 Architecture for Control Networks (ACN)

#tabs=3t
*/
#include <stdio.h>
#include "acn.h"
#include "ddl/parsetokens.h"

#undef _TOKEN_
#define _TOKEN_(name, str) [name] = str

const ddlchar_t *alltokens[] = {
	ALL_TOKENS
};

/**********************************************************************/
int
main(int argc, char *argv[])
{
	printf("Size %lu, param diff %u, %s\n", ARRAYSIZE(alltokens), PARAMTOK(0), alltokens[PARAMTOK(TK_write)]);
	return 0;
}
