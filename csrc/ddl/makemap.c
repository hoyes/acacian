/**********************************************************************/
/*

	Copyright (C) 2012, Engineering Arts. All rights reserved.

	Author: Philip Nye

	$Id$

#tabs=3
*/
/**********************************************************************/

#include <stdio.h>
#include <expat.h>
#include <assert.h>
#include <unistd.h>
/*
#include <errno.h>
#include <fcntl.h>
#include <string.h>
*/
#include "acncommon.h"
#include "acnlog.h"
#include "acnmem.h"
#include "uuid.h"
#include "ddl/parse.h"
#include "propmap.h"
#include "ddl/behaviors.h"
#include "propmap.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL
/**********************************************************************/
void
insprop(struct proptab_s *ptab, prop_t *prop, uint32_t addr, uint32_t count)
{
	uint32_t mod;
	struct propfind_s *pf;
	uint32_t i;

	mod = addr % ptab->inc;
	pf = ptab->props;
	i = 0;
	while (i < ptab->i) {
		if (pf->mod > mod || (pf->mod == mod && pf->lo > addr)) {
			memmove((void *)(pf + 1), (void *)pf, (ptab->i - i) * sizeof(*pf));
			break;
		}
		++i;
		++pf;
	}

	pf->mod = mod;
	pf->lo = addr;
	pf->count = count;
	pf->prop = prop;
	++ptab->i;
	acnlogmark(lgDBUG, "add %d of %d", ptab->i, ptab->nprops);
	assert(ptab->i <= ptab->nprops);
#if 0
	if (dpf > pf)
		assert(spf->mod < mod
			|| (spf->lo + spf->count * ptab->inc <= addr));
	spf = dpf + 1;
	if (spf < pf + ptab->i) {
		assert(spf->mod > mod
			|| (spf->lo >= addr + count * ptab->inc));
	}
#endif
}

/**********************************************************************/
void
addprops(struct proptab_s *ptab, prop_t *prop, prop_t *app, prop_t *dimprop, uint32_t addr)
{
	/* go up the tree to find an array dimension (if any) */
	while (dimprop->arraytotal > 1
			&& (dimprop->array == 1 || dimprop == app))
		dimprop = dimprop->parent;

	if (dimprop->arraytotal == 1)
		insprop(ptab, prop, addr, app->array);	/* reached the top */
	else {
		uint32_t eaddr;
		int32_t inc;

		inc = (dimprop == prop) ? prop->v.net.inc : dimprop->childinc;

		acnlogmark(lgDBUG, "table inc %d, prop inc %d", ptab->inc, inc);
		eaddr = addr + dimprop->array * inc;
		while (eaddr > addr) {
			eaddr -= inc;
			addprops(ptab, prop, app, dimprop->parent, eaddr);
		}
	}
}

/**********************************************************************/
void
insertprops(struct proptab_s *ptabs, prop_t *prop)
{
	prop_t *app;

	app = prop->v.net.maxarrayprop;
	
	acnlogmark(lgDBUG, "insert %u, count %u, inc %d", prop->v.net.addr, prop->array, prop->v.net.inc);
	if (app == NULL) {
		assert(ptabs->inc == 1);
		insprop(ptabs, prop, prop->v.net.addr, 1);
	} else {
		int32_t inc;

		inc = (app == prop) ? app->v.net.inc : app->childinc;
	
		/* find the right table */
		while (ptabs->inc != inc) {
			ptabs = ptabs->nxt;
			assert(ptabs != NULL);
		}
		addprops(ptabs, prop, app, prop, prop->v.net.addr);
	}
}

/**********************************************************************/
void
mapwalk(struct proptab_s *ptabs, prop_t *prop)
{
	//acnlogmark(lgDBUG, "mapwalk");
	if (prop->vtype == VT_network) insertprops(ptabs, prop);
	for (prop = prop->children; prop != NULL; prop = prop->siblings)
		mapwalk(ptabs, prop);
}

/**********************************************************************/
struct proptab_s *
makemap(rootprop_t *root)
{
	struct propfind_s *propfs;
	struct proptab_s *ptab;
	uint32_t ui;

	acnlogmark(lgDBUG, "Allocate %lu for maps", sizeof(struct propfind_s) * root->nnetprops);
	
	propfs = mallocx(sizeof(struct propfind_s) * root->nnetprops);

	/* go through our list of tables and assign sections of the array */
	ui = 0;
	for (ptab = root->ptabs; ptab != NULL; ptab = ptab->nxt) {
		acnlogmark(lgDBUG, "Table: inc=%d, length=%u", ptab->inc, ptab->nprops);
		acnlogmark(lgDBUG, "Root: nnetprops=%u, nflatprops=%u", root->nnetprops, root->nflatprops);
		acnlogmark(lgDBUG, "This %lu, Next: %lu", (uintptr_t)ptab, (uintptr_t)ptab->nxt);
		ptab->props = propfs + ui;
		ui += ptab->nprops;
		acnlogmark(lgDBUG, "ui=%u", ui);
		assert(ui <= root->nnetprops);
	}
	acnlogmark(lgDBUG, "No more tables");
	sleep(1);

	/* now walk the tree and insert each property into it's proper table */
	mapwalk(root->ptabs, &(root->prop));
	return root->ptabs;
}
