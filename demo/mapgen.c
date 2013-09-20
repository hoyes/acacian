/**********************************************************************/
/*

	Copyright (C) 2011, Engineering Arts. All rights reserved.

	Author: Philip Nye

	$Id$

#tabs=3
*/
/**********************************************************************/

#include <stdio.h>
#include <expat.h>
#include <ctype.h>
/*
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

*/
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
FILE *
srcfile(const char *fname, const char *dcidstr) {
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
static void
printhheader(const char *dcidstr, const char *hfilename)
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
	hmacro = savestr(namebuf);
	fprintf(hfile,
		"#ifndef %s\n"
		"#define %s 1\n\n"
		"extern const char DCID_str[UUID_STR_SIZE];\n\n",
		hmacro, hmacro
	);
	LOG_FEND();
}

/**********************************************************************/
static void
printcheader(const char *dcidstr, const char **headers)
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
	fprintf(cfile,
		"\nconst char DCID_str[UUID_STR_SIZE] = \"%s\";\n\n",
		dcidstr
	); 
	LOG_FEND();
}

/**********************************************************************/
static void
printhfooter(void)
{
	fprintf(hfile,
		"\n#endif  /* %s */\n", hmacro);
	freestr(hmacro);
}
/**********************************************************************/
#define printcfooter()

/**********************************************************************/
static int
getcname(struct prop_s *prop, char *buf, int len)
{
	int ofs;
	struct prop_s *dev;

	assert(prop->parent != NULL); /* shouldn't be called for root */
	ofs = 0;
	dev = itsdevice(prop->parent);
	if (dev->parent) {
		ofs = getcname(dev, buf, len);
		buf[ofs++] = '_';
	}
	if (prop->id) {
		const char *cp;

		for (cp = prop->id; *cp; ++cp) {
			if (ofs >= len) goto overlen;
			buf[ofs++] = isalnum(*cp) ? *cp : '_';
		}
		buf[ofs] = 0;
	} else {
		ofs += snprintf(buf + ofs, len - ofs, ((prop->vtype == VT_device) ? "sub%u" : "p%u"), prop->pnum);
	}
	if (ofs >= len) {
overlen:
		acnlogmark(lgWARN, "cname truncated");
		buf[ofs = len - 1] = 0;
	}
	acnlogmark(lgDBUG, "cname: %s", buf);
	return ofs;
}
/**********************************************************************/
extern const struct allowtok_s extendallow;
extern const ddlchar_t *tokstrs[];
#define CNBUFSIZE 1024
#define CNBUFEND (cname + CNBUFSIZE)

void
printprops(struct prop_s *prop)
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
		char cname[CNBUFSIZE];

		getcname(prop, cname, CNBUFSIZE);
		acnlogmark(lgDBUG, "property %s", cname);
		switch (prop->vtype) {
		case VT_network: {
			struct dmpprop_s *np;
			struct dmpdim_s *dp;
			struct EA_propext_s *pe;
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
				"\t.ulim = %u,\n",
				cname,
				flagnames(np->flags, pflgnames, flagbuf, " | pflg(%s)"),
				etypes[np->etype],
				np->size,
				np->addr,
				np->ulim
			);
#ifdef ACNCFG_EXTENDTOKENS
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
			for (dp = np->dim; dp < np->dim + np->ndims; ++dp) {
				fprintf(cfile, "\t\t{.i = %i, .r = %u, .lvl = %i},\n",
					dp->i, dp->r, dp->lvl);
				fprintf(hfile, "#define DIM_%s__%u %u\n", cname, dp->lvl, dp->r + 1);
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
void
printtests(struct srch_amap_s *smap)
{
	struct addrfind_s *af;
	int overp = 0;

	LOG_FSTART();
	for (af = smap->map; af < smap->map + smap->count; ++af) {
		if (af->ntests > map_max_tests) map_max_tests = af->ntests;
		if (af->ntests > 1) {
			int i;
			struct dmpprop_s *prop;

			if (overp == 0)
				fprintf(cfile, "struct dmpprop_s *%s[] = {\n", overarrayname);
			for (i = 0; i < af->ntests; ++i) {
				char cname[CNBUFSIZE];

				getcname(af->p.pa[i]->prop, cname, CNBUFSIZE);
				fprintf(cfile, "\t[%d] = &" PPX "%s,\n", overp++, cname);
			}
		}
	}
	if (overp > 0)
		fprintf(cfile, "};\n\n");
	LOG_FEND();
}

/**********************************************************************/
void
printsrchmap(struct srch_amap_s *smap)
{
	struct prop_s *pp;
	struct addrfind_s *af;
	union proportest_u *nxt;
	int i, j;
	int d;
	int overp = 0;

	LOG_FSTART();
	fprintf(cfile, "struct addrfind_s %s[] = {\n", srcharrayname);
	for (i = 0, af = smap->map; i < smap->count; ++i, ++af) {
		fprintf(cfile, "\t{.adlo = %u, .adhi = %u, .ntests = %i, .p = {",
				af->adlo, af->adhi, af->ntests);
		if (af->ntests < 2) {
			char cname[CNBUFSIZE];

			getcname(af->p.prop->prop, cname, CNBUFSIZE);
			fprintf(cfile, ".prop = &" PPX "%s}},\n", cname);
		} else {
			fprintf(cfile, ".pa = %s + %d}},\n", overarrayname, overp);
			overp += af->ntests;
		}
	}
	fprintf(cfile, "};\n\n");
	fprintf(cfile,
		"union addrmap_u %s = {.srch = {\n"
		"\t.type = am_srch,\n"
		"\t.size = 0,\n"
		"\t.map = %s,\n"
		"\t.maxdims = %u,\n"
		"\t.flags = 0x%04x,\n"
		"\t.count = %u,\n"
		"}};\n\n"
		, addrmapname, srcharrayname, smap->maxdims, smap->flags, smap->count);
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
void
printindxmap(struct indx_amap_s *imap)
{
	char cname[CNBUFSIZE];
	int i;

	LOG_FSTART();
	fprintf(cfile, "struct dmpprop_s *%s[] = {\n", indxarrayname);
	acnlogmark(lgDBUG, "index map. base = %u, range = %u", imap->base, imap->range);
	for (i = 0; i < imap->range; ++i) {
		if (imap->map[i]) {
			getcname(imap->map[i]->prop, cname, CNBUFSIZE);
			acnlogmark(lgDBUG, "%i = %s", i, cname);
			fprintf(cfile, "\t&" PPX "%s,\n", cname);
		} else {
			fputs("\tNULL,\n", cfile);
		}
	}
	fprintf(cfile, "};\n\n");
	fprintf(cfile,
		"union addrmap_u %s = {.indx = {\n"
		"\t.type = am_indx,\n"
		"\t.size = 0,\n"
		"\t.map = %s,\n"
		"\t.flags = 0x%04x\n"
		"\t.maxdims = %u,\n"
		"\t.range = %u,\n"
		"\t.base = %u,\n"
		"}};\n\n"
		, addrmapname, indxarrayname, imap->maxdims, imap->flags, imap->range, imap->base);
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
*/

void
usage(bool fail)
{
	LOG_FSTART();
	fprintf(fail ? stderr : stdout,
				"Usage: mapgen [ -H headerfile ] [ -C sourcefile ] [-u] UUID\n");
	LOG_FEND();
	exit(fail ? EXIT_FAILURE : EXIT_SUCCESS);
}

/**********************************************************************/
int
main(int argc, char *argv[])
{
	struct rootprop_s *rootprop;
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

	headers[hi] = strrchr(hfilename, DIRSEP);

	if (headers[hi]) headers[hi]++;
	else headers[hi] = hfilename;
	headers[hi + 1] = NULL;

	init_behaviors();

	rootprop = parsedevice(rootname);
	uuid2str(rootprop->dcid, dcidstr);
	hfile = srcfile(hfilename, dcidstr);
	cfile = srcfile(cfilename, dcidstr);

	printhheader(dcidstr, headers[hi]);
	printcheader(dcidstr, headers);
	printprops(&rootprop->prop);

	amap = rootprop->amap;	/* get the map - always a srch type initially */
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

	printhfooter();
	printcfooter();

	freerootprop(rootprop);
	//freemap(map);

	return 0;
}
