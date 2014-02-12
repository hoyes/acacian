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
file: mapgen.c

Address map generator for devices:

Mapgen generates the address map and property tables for an ACN 
`device` from its DDL description. These tables are output as C code 
which then gets compiled in to the device code along with other 
sources. Using the DDL to generate the device code both verifies the 
DDL and ensures that it really matches the device and so prevents 
errors and discrepancies when a controller later uses that same DDL.

Mapgen uses the same DDL parser that is used by controllers but it 
is run on the host system at build time. It parses DDL in exactly the
same way to generate an address and property map. It then walks 
through these structures generating a C representation. For an example
of its use see the Acacian device demonstration program.
*/

#include <stdio.h>
#include <expat.h>
#include <ctype.h>
#include <assert.h>
#include <getopt.h>

#include "acn.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL
/**********************************************************************/

const char dflthfilename[] = "devicemap.h";
const char dfltcfilename[] = "devicemap.c";

const char srcharrayname[] = "findprop_array";
const char overarrayname[] = "testprop_array";
const char addrmapname[] = "addr_map";
const char indxarrayname[] = "property_index";

#define PPX "DMP_"

FILE *hfile = NULL;
FILE *cfile = NULL;
ddlchar_t *hmacro;
static int map_max_tests = 0;

/**********************************************************************/
/*
section: Internals

func: src_outf

Open a file for source code output.

The named file is opened and a standard heading text is printed 
including the DCID of the device it represents. Used for both .c and .h 
outputs.

Returns a FILE pointer.
*/
static FILE *
src_outf(const char *fname, const char *dcidstr) {
	char *cp;
	FILE *f;

	if ((f = fopen(fname, "w")) == NULL) {
		fprintf(stderr, "Create file \"%s\": %s", fname, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if ((cp = strrchr(fname, DIRSEP))) fname = cp + 1;

	fprintf(f,
		"/**********************************************************************/\n"
		"/*\n"
		"\tfile: %s\n"
		"\n"
		"\tDevice definition for DCID %s.\n"
		"\n"
		"\tDo not edit. Automatically generated from DDL source.\n"
		"*/\n"
		"/**********************************************************************/\n"
		, fname
		, dcidstr
	);
	return f;
}

/**********************************************************************/
/*
func: h_putheader

Print the opening text of the generated .h file
*/
static void
h_putheader(const char *dcidstr, const char *hfilename)
{
	ddlchar_t namebuf[256];
#define endp (namebuf + sizeof(namebuf))
	ddlchar_t *cp;

	LOG_FSTART();

	cp = namebuf;
	*cp++ = '_';
	*cp++ = '_';
	while (*hfilename) {
		*cp++ = isalnum(*hfilename) ? *hfilename : '_';
		if (cp >= endp - 3) break;
		++hfilename;
	}
	*cp++ = '_';
	*cp++ = '_';
	*cp = 0;
	hmacro = strdup(namebuf);
	fprintf(hfile,
		"#ifndef %s\n"
		"#define %s 1\n\n"
		"#define DCID_STR \"%s\"\n\n",
		hmacro, hmacro, dcidstr
	);
	LOG_FEND();
}

/**********************************************************************/
/*
func: c_putheader

Print the opening text of the generated .c file
*/
static void
c_putheader(const char *dcidstr, const char **headers)
{
	const char **hp;

	LOG_FSTART();
	for (hp = headers; *hp; ++hp) {
		if (**hp == '^')
			fprintf(cfile, "#include \"%s\"\n", *hp + 1);
	}
	fputs("#include \"acn.h\"\n", cfile);
	for (hp = headers; *hp; ++hp) {
		if (**hp != '^')
			fprintf(cfile, "#include \"%s\"\n", *hp);
	}
	/*
	fprintf(cfile,
		"\nconst char DCID_str[UUID_STR_SIZE] = \"%s\";\n\n",
		dcidstr
	); 
	*/
	LOG_FEND();
}

/**********************************************************************/
/*
func: h_putfooter

Print the closing text of the generated .h file
*/
static void
h_putfooter(void)
{
	fprintf(hfile,
		"\n#endif  /* %s */\n", hmacro);
	free(hmacro);
}
/**********************************************************************/
/*
func: c_putfooter

Print the closing text of the generated .c file. This is included for 
completeness but currently does nothing.
*/
static inline void c_putfooter(void) {}
/**********************************************************************/
/*

Format is:
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
or
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
*/
#define DCIDBIN_AS_STRING 0

#if DCIDBIN_AS_STRING
#define DCID_BIN_SIZE (UUID_SIZE * 4 + 3)
#else
#define DCID_BIN_SIZE (UUID_SIZE * 5 + 2)
#endif

/**********************************************************************/
/*
func: dcid_lit

Convert a UUID (DCID) to a C literal which compiles to a binary array.
*/
static char *
dcid_lit(uint8_t *dcid, char *buf)
{
	sprintf(buf,
#if DCIDBIN_AS_STRING
		"\""
		"\\x%02x\\x%02x\\x%02x\\x%02x"
		"\\x%02x\\x%02x\\x%02x\\x%02x"
		"\\x%02x\\x%02x\\x%02x\\x%02x"
		"\\x%02x\\x%02x\\x%02x\\x%02x"
		"\"",
#else
		"{"
		"0x%02x,0x%02x,0x%02x,0x%02x,"
		"0x%02x,0x%02x,0x%02x,0x%02x,"
		"0x%02x,0x%02x,0x%02x,0x%02x,"
		"0x%02x,0x%02x,0x%02x,0x%02x"
		"}",
#endif
		dcid[ 0],
		dcid[ 1],
		dcid[ 2],
		dcid[ 3],
		dcid[ 4],
		dcid[ 5],
		dcid[ 6],
		dcid[ 7],
		dcid[ 8],
		dcid[ 9],
		dcid[10],
		dcid[11],
		dcid[12],
		dcid[13],
		dcid[14],
		dcid[15]
	);
	return buf;
}

/**********************************************************************/
extern const struct allowtok_s extendallow;
extern const ddlchar_t *tokstrs[];
/**********************************************************************/
/*
func: printprops

Recursively walk the property tree printing each property.

Network properties generate entries in the .c output for each DMP 
property together with external declarations in the .h output. 
Immediate properties (values) generate macros in the .h output.

If <Property Extensions> are enabled DMP properties also include function
callbacks for handling get-property, set-property and subscription
messages.
*/
static void
printprops(struct ddlprop_s *prop)
{
	LOG_FSTART();

	switch (prop->vtype) {
	default: break;
	/* only act on these types */
	case VT_network:
	case VT_imm_uint:
	case VT_imm_sint:
	case VT_imm_float:
	case VT_imm_string: {
		char flagbuf[pflg_NAMELEN + pflg_COUNT * sizeof(" | pflg")];
		const char *cname;

		cname = propcname(prop);
		acnlogmark(lgDBUG, "property %s", cname);
		switch (prop->vtype) {
		case VT_network: {
			struct dmpprop_s *np;
			struct dmpdim_s *dp;
			int i;
	
			np = prop->v.net.dmp;
			assert(np);
			fprintf(hfile, "extern struct dmpprop_s " PPX "%s;\n", cname);
			fprintf(cfile,
				"struct dmpprop_s " PPX "%s = {\n"
				"\t.flags = 0%s,\n"
				"\t.etype = etype_%s,\n"
				"\t.size = %u,\n"
				"\t.addr = %u,\n"
				"\t.span = %u,\n",
				cname,
				flagnames(np->flags, pflgnames, flagbuf, " | pflg(%s)"),
				etypes[np->etype],
				np->size,
				np->addr,
				np->span
			);
#ifdef CF_PROPEXT_TOKS
			for (i = 0; i < ARRAYSIZE(np->extends); ++i) {
				fprintf(cfile, "\t.%s = %s,\n", 
							tokstrs[extendallow.toks[i]],
							np->extends[i] ? np->extends[i] : "NULL"
						);
			}
#endif
			fprintf(cfile,
				"\t.ndims = %i,\n"
				"\t.dim = {\n",
				np->ndims
			);
			fprintf(hfile, "#define nDIMS_%s %u\n", cname, np->ndims);
			for (dp = np->dim, i = 0; i < np->ndims; ++dp, ++i) {
				fprintf(cfile, "\t\t{.inc = %i, .cnt = %u, .tref = %i},\n",
					dp->inc, dp->cnt, dp->tref);
				fprintf(hfile, "#define DIM_%s__%u %u\n"
								"#define INC_%s__%u %u\n", 
								cname, i, dp->cnt, cname, i, dp->inc);
			}
			fprintf(cfile, "\t}\n};\n\n");
			break;
		}
		case VT_imm_uint:
			if (prop->v.imm.count == 1) {
				fprintf(hfile, "#define IMMP_%s %u\n", cname, prop->v.imm.t.ui);
			} else {
				int i;
	
				for (i = 0; i < prop->v.imm.count; ++i)
					fprintf(hfile, "#define IMMP_%s__%d %u\n", cname, i, prop->v.imm.t.Aui[i]);
			}
			break;
		case VT_imm_sint:
			if (prop->v.imm.count == 1) {
				fprintf(hfile, "#define IMMP_%s %d\n", cname, prop->v.imm.t.si);
			} else {
				int i;
	
				for (i = 0; i < prop->v.imm.count; ++i)
					fprintf(hfile, "#define IMMP_%s__%d %d\n", cname, i, prop->v.imm.t.Asi[i]);
			}
			break;
		case VT_imm_float:
			if (prop->v.imm.count == 1) {
				fprintf(hfile, "#define IMMP_%s %g\n", cname, prop->v.imm.t.f);
			} else {
				int i;
	
				for (i = 0; i < prop->v.imm.count; ++i)
					fprintf(hfile, "#define IMMP_%s__%d %g\n", cname, i, prop->v.imm.t.Af[i]);
			}
			break;
		case VT_imm_string:
			if (prop->v.imm.count == 1) {
				fprintf(hfile, "#define IMMP_%s \"%s\"\n", cname, prop->v.imm.t.str);
			} else {
				int i;
	
				for (i = 0; i < prop->v.imm.count; ++i)
					fprintf(hfile, "#define IMMP_%s__%d \"%s\"\n", cname, i, prop->v.imm.t.Astr[i]);
			}
			break;
		}
		break; }
	}
	
	if (prop->children) printprops(prop->children);
	if (prop->siblings) printprops(prop->siblings);
	LOG_FEND();
}

/**********************************************************************/
/*
func: printtests

Print the extra array of property pointers needed where multiple
properties interleave in an address range.
*/
static void
printtests(struct srch_amap_s *smap)
{
	struct addrfind_s *af;
	int overp = 0;

	LOG_FSTART();
	for (af = smap->map; af < smap->map + smap->count; ++af) {
		if (af->ntests > map_max_tests) map_max_tests = af->ntests;
		if (af->ntests > 1) {
			int i;

			if (overp == 0)
				fprintf(cfile, "struct dmpprop_s *%s[] = {\n", overarrayname);
			for (i = 0; i < af->ntests; ++i) {
				fprintf(cfile, "\t[%d] = &" PPX "%s,\n", overp++, 
							propcname(af->p.pa[i]->prop));
			}
		}
	}
	if (overp > 0)
		fprintf(cfile, "};\n\n");
	LOG_FEND();
}

/**********************************************************************/
/*
func: printsrchmap

Print an addrmap structure in the generic search map format (<srch_amap_s>).
*/
static void
printsrchmap(struct srch_amap_s *smap)
{
	struct addrfind_s *af;
	int i;
	int overp = 0;
	char dcidb[DCID_BIN_SIZE];

	LOG_FSTART();
	fprintf(cfile, "struct addrfind_s %s[] = {\n", srcharrayname);
	for (i = 0, af = smap->map; i < smap->count; ++i, ++af) {
		fprintf(cfile, "\t{.adlo = %u, .adhi = %u, .ntests = %i, .p = {",
				af->adlo, af->adhi, af->ntests);
		if (af->ntests < 2) {
			fprintf(cfile, ".prop = &" PPX "%s}},\n", propcname(af->p.prop->prop));
		} else {
			fprintf(cfile, ".pa = %s + %d}},\n", overarrayname, overp);
			overp += af->ntests;
		}
	}
	fprintf(cfile, "};\n\n");
	fprintf(cfile,
		"union addrmap_u %s = {.srch = {\n"
		"\t.dcid = %s,\n"
		"\t.type = am_srch,\n"
		"\t.size = 0,\n"
		"\t.map = %s,\n"
		"\t.maxdims = %u,\n"
		"\t.flags = 0x%04x,\n"
		"\t.count = %u,\n"
		"}};\n\n"
		, addrmapname, dcid_lit(smap->dcid, dcidb), srcharrayname, 
		smap->maxdims, smap->flags, smap->count);
	fprintf(hfile,
			"\n"
			"#define MAP_TYPE am_srch\n"
			"#define MAP_HAS_OVERLAP %u\n"
			"#define MAP_MAX_DIMS %u\n"
			"#define MAP_MAX_TESTS %u\n"
			"extern union addrmap_u %s;\n"
			, (smap->flags & pflg(overlap)) != 0, smap->maxdims, map_max_tests, addrmapname);	
	LOG_FEND();
}
/**********************************************************************/
/*
func: printindxmap

Print an addrmap structure in the direct index map format (<indx_amap_s>).
*/
static void
printindxmap(struct indx_amap_s *imap)
{
	int i;
	char dcidb[DCID_BIN_SIZE];

	LOG_FSTART();
	fprintf(cfile, "struct dmpprop_s *%s[] = {\n", indxarrayname);
	acnlogmark(lgDBUG, "index map. base = %u, range = %u", imap->base, imap->range);
	for (i = 0; i < imap->range; ++i) {
		if (imap->map[i]) {
			const char *cname;
			cname = propcname(imap->map[i]->prop);
			acnlogmark(lgDBUG, "%i = %s", i, cname);
			fprintf(cfile, "\t&" PPX "%s,\n", cname);
		} else {
			fputs("\tNULL,\n", cfile);
		}
	}
	fprintf(cfile, "};\n\n");
	fprintf(cfile,
		"union addrmap_u %s = {.indx = {\n"
		"\t.dcid = %s,\n"
		"\t.type = am_indx,\n"
		"\t.size = 0,\n"
		"\t.map = %s,\n"
		"\t.flags = 0x%04x,\n"
		"\t.maxdims = %u,\n"
		"\t.range = %u,\n"
		"\t.base = %u,\n"
		"}};\n\n"
		, addrmapname, dcid_lit(imap->dcid, dcidb), indxarrayname, 
		imap->maxdims, imap->flags, imap->range, imap->base);
	fprintf(hfile,
			"\n"
			"#define MAP_TYPE am_indx\n"
			"#define MAP_HAS_OVERLAP %u\n"
			"#define MAP_MAX_DIMS %u\n"
			"#define MAP_MAX_TESTS 0\n"
			"extern union addrmap_u %s;\n"
			, (imap->flags & pflg(overlap)) != 0, imap->maxdims, addrmapname);	
	LOG_FEND();
}

/**********************************************************************/
/*
Program usage message.
*/
const char usage_str[] = 
"Usage: mapgen [ -h hfile ] [ -c cfile ] [-i hdr.h ] [-u] UUID\n"
"  hfile is name of .h output, (default 'devicemap.h')\n"
"  cfile is name of .c output, (default 'devicemap.c')\n"
"  each -i option generates '#include \"hdr.h\"' in the .c file\n"
;

/*
func: usage

Print a usage message and exit. If fail is false the message goes to 
stdout and the program returns a zero exit code. Otherwise the 
message goes to stderr and the exit code is EXIT_FAILURE.
*/

static void
usage(bool fail)
{
	LOG_FSTART();
	fprintf(fail ? stderr : stdout, usage_str);
	LOG_FEND();
	exit(fail ? EXIT_FAILURE : EXIT_SUCCESS);
}

/**********************************************************************/
/*
section: Main

func: main

Main mapgen program.

> Usage: mapgen [ -h hfile ] [ -c cfile ] [-i hdr.h ] [-u] UUID
>   hfile is name of .h output, (default 'devicemap.h')
>   cfile is name of .c output, (default 'devicemap.c')
>   each -i option generates '#include \"hdr.h\"' in the .c file

- Parse command line arguments
- Parse the root DCID and its subdevices to generate a property tree
and address map in default search format.
- Generate output from the property tree for each DMP property or 
immediate value.
- If the address map meets criteria for conversion, transform it into
an index map for efficiency.
- Generate output for the address map in the chosen format, cross 
referenced to the property structures.

topic: Address map type selection

An index map is a single linear array with one entry per address. 
The starting address and length of the map are stored in its header.
This is very efficient since a single array reference goes straight from
the address to the property structure. However it is only feasible
where there are few properties and they are quite close together in 
the address space. Also, whereas a search-map usually has a single 
entry for an array property, however many elements in the array, an 
index map has an entry for each element. The code here calculates 
the size of the index array that would be needed and transforms the map
to index format if
the size is ‘reasonable’.
*/
int
main(int argc, char *argv[])
{
	struct rootdev_s *rootdev;
	int opt;
	const char *rootname = NULL;
	const char *hfilename = dflthfilename;
	const char *cfilename = dfltcfilename;
	char dcidstr[UUID_STR_SIZE];
	union addrmap_u *amap;
	unsigned int adrange;
	const char *headers[argc];
	int hi = 0;

	while ((opt = getopt(argc, argv, "h:c:u:i:")) >= 0) switch (opt) {
	case 'h':
		hfilename = optarg;
		break;
	case 'c':
		cfilename = optarg;
		break;
	case 'u':
		if (str2uuid(optarg, NULL) != 0) {
			fprintf(stderr, "Bad DCID \"%s\"\n", optarg);
			exit(EXIT_FAILURE);
		}
		rootname = optarg;
		break;
	case 'i':
		headers[hi++] = optarg;
		break;
	default:
		usage(true);
		break;
	}
	if (rootname == NULL) {
		if (optind < argc) rootname = argv[optind];
		else usage(true);
	}

	/* if hfile is a path, we just want to #include the last part */
	headers[hi] = strrchr(hfilename, DIRSEP);
	if (headers[hi]) headers[hi]++;
	else headers[hi] = hfilename;

	headers[hi + 1] = NULL;
	rootdev = parseroot(rootname);

	amap = rootdev->amap;	/* get the map - always a srch type initially */
	uuid2str(amap->any.dcid, dcidstr);
	hfile = src_outf(hfilename, dcidstr);
	cfile = src_outf(cfilename, dcidstr);

	h_putheader(dcidstr, headers[hi]);
	c_putheader(dcidstr, headers);
	printprops(rootdev->ddlroot);

	adrange = amap->srch.map[amap->srch.count - 1].adhi + 1 - amap->srch.map[0].adlo;
	acnlogmark(lgDBUG, "Address range: %u", adrange);
	if (adrange <= 32 || ((adrange * sizeof(void *)) / (amap->srch.count * sizeof(struct addrfind_s))) < 3) {
		/* other criteria for conversion could be used */
		acnlogmark(lgDBUG, "Transforming map\n");
		xformtoindx(amap);
		printindxmap(&amap->indx);
	} else {
		printtests(&amap->srch);
		printsrchmap(&amap->srch);
	}

	h_putfooter();
	c_putfooter();

	freerootdev(rootdev);

	return 0;
}
