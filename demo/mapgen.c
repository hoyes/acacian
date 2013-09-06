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

const char mainarrayname[] = "findprop_array";
const char overarrayname[] = "testprop_array";
const char addrmapname[] = "addr_map";

const char theamap[] =
"union addrmap_u %s = {.srch = {\n"
"\t.type = am_srch,\n"
"\t.size = 0,\n"
"\t.map = %s,\n"
"\t.count = %u\n"
"}};\n\n"
;

#define PPX "DMP_"

FILE *hfile = NULL;
FILE *cfile = NULL;
ddlchar_t *hmacro;

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
printhheader(const char *hfilename, const char *dcidstr)
{
	ddlchar_t namebuf[256];
#define endp (namebuf + sizeof(namebuf))
	ddlchar_t *cp;

	LOG_FSTART();

	if ((cp = strrchr(hfilename, DIRSEP))) hfilename = cp + 1;
	
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
printcheader(const char *hfilename, const char *dcidstr)
{
	ddlchar_t *cp;

	LOG_FSTART();
	if ((cp = strrchr(hfilename, DIRSEP))) hfilename = cp + 1;
	fprintf(cfile,
		"#include \"acn.h\"\n"
		"#include \"%s\"\n\n"
		"const char DCID_str[UUID_STR_SIZE] = \"%s\";\n\n",
		hfilename,
		dcidstr
	); 
	LOG_FEND();
}

/**********************************************************************/
static void
printhfooter(void)
{
	fprintf(hfile,
		"\nextern union addrmap_u %s;\n"
		"\n#endif  /* %s */\n", addrmapname, hmacro);
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
			fprintf(hfile, "extern struct dmpprop_s " PPX "%s\n", cname);
			fprintf(cfile,
				"struct dmpprop_s " PPX "%s = {\n"
				"\t.flags = 0%s,\n"
				"\t.etype = etype_%s,\n"
				"\t.size = %u,\n"
				"\t.addr = %u,\n"
				"\t.ulim = %u,\n",
				cname,
				flagnames(np->flags, pflgnames, flagbuf, " | pflg_%s"),
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
printmap(struct srch_amap_s *smap)
{
	struct prop_s *pp;
	struct netprop_s *np;
	struct addrfind_s *af;
	union proportest_u *nxt;
	int i, j;
	int d;
	int overp = 0;

	LOG_FSTART();
	fprintf(cfile, "struct addrfind_s %s[] = {\n", mainarrayname);
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
	fprintf(cfile, theamap, addrmapname, mainarrayname, smap->count);
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

	while ((opt = getopt(argc, argv, "h:c:u:")) >= 0) switch (opt) {
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
	default:
		usage(true);
		break;
	}
	if (rootname == NULL) {
		if (optind < argc) rootname = argv[optind];
		else usage(true);
	}

	init_behaviors();

	rootprop = parsedevice(rootname);
	uuid2str(rootprop->dcid, dcidstr);
	hfile = srcfile(hfilename, dcidstr);
	cfile = srcfile(cfilename, dcidstr);

	printhheader(hfilename, dcidstr);
	printcheader(hfilename, dcidstr);  /* need hfilename, not cfilename */
	printprops(&rootprop->prop);
	printtests(&rootprop->amap->srch);
	printmap(&rootprop->amap->srch);
	printhfooter();
	printcfooter();

	freerootprop(rootprop);
	//freemap(map);

	return 0;
}
