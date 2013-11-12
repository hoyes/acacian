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
file: controller_demo.c

Simple demonstration controller application.
*/

#include <stdio.h>
#include <ctype.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


/*
#include <expat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

*/
#include "acn.h"
#include "ddl/printtree.h"
#include "demo_utils.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_DDL
/**********************************************************************/
/*
Fix some values
*/

const char fctn[] = "Acacian controller demo";
const char uacn_dflt[] = "Unnamed demo controller";

#define MAXINTERFACES 8
#define MAX_REMOTES 100
//#define LIFETIME SLP_LIFETIME_MAXIMUM
#define LIFETIME 300

/**********************************************************************/
struct dmptcxt_s *ctlcxt = NULL;

/**********************************************************************/
struct Rcomponent_s *remlist[MAX_REMOTES];
struct member_s *ctlmbrs[MAX_REMOTES] = {NULL,};
int nremotes = 0;
struct uuidset_s devtrees;

/**********************************************************************/
struct Lcomponent_s localComponent = {
	.fctn = fctn,
	.uacn = uacn,
	.lifetime = LIFETIME,
	.dmp = {
		.rxvec = {
			[DMP_reserved0]             = NULL,
			[DMP_GET_PROPERTY]          = NULL,
			[DMP_SET_PROPERTY]          = NULL,
			[DMP_GET_PROPERTY_REPLY]    = NULL,
			[DMP_EVENT]                 = NULL,
			[DMP_reserved5]             = NULL,
			[DMP_reserved6]             = NULL,
			[DMP_SUBSCRIBE]             = NULL,
			[DMP_UNSUBSCRIBE]           = NULL,
			[DMP_GET_PROPERTY_FAIL]     = NULL,
			[DMP_SET_PROPERTY_FAIL]     = NULL,
			[DMP_reserved11]            = NULL,
			[DMP_SUBSCRIBE_ACCEPT]      = NULL,
			[DMP_SUBSCRIBE_REJECT]      = NULL,
			[DMP_reserved14]            = NULL,
			[DMP_reserved15]            = NULL,
			[DMP_reserved16]            = NULL,
			[DMP_SYNC_EVENT]            = NULL,
		},
		.flags = 0,
	}
};

/**********************************************************************/
const char *evnames[] = {
	[EV_RCONNECT]         = "remote initiated connect",
	[EV_LCONNECT]         = "local initiated connect",
	[EV_DISCOVER]         = "adhoc info",
	[EV_JOINFAIL]         = "join fail",
	[EV_JOINSUCCESS]      = "join success",
	[EV_LOCCLOSE]         = "local initiated close",
	[EV_LOCDISCONNECT]    = "local initiated disconnect",
	[EV_LOCDISCONNECTING] = "local initiated disconnecting",
	[EV_LOCLEAVE]         = "local initiated leave",
	[EV_LOSTSEQ]          = "lost sequence",
	[EV_MAKTIMEOUT]       = "MAK timeout",
	[EV_NAKTIMEOUT]       = "NAK timeout",
	[EV_REMDISCONNECT]    = "remote initiated disconnect",
	[EV_REMDISCONNECTING] = "remote initiated disconnecting",
	[EV_REMLEAVE]         = "remote initiated leave",
	[EV_CONNECTFAIL]      = "connect fail",
};
/**********************************************************************/
void cd_sdtev(int event, void *object, void *info)
{
	struct Lchannel_s *Lchan;
	struct member_s *mbr;
	char uuidstr[UUID_STR_SIZE];
	int i;

	LOG_FSTART();
	acnlogmark(lgDBUG, "SDT event: %s", evnames[event]);

	switch (event) {
	case EV_RCONNECT:  /* object = Lchan, info = memb */
	case EV_LCONNECT:  /* object = Lchan, info = memb */
		Lchan = (struct Lchannel_s *)object;
		mbr = (struct member_s *)info;

		if (ctlcxt && Lchan == ctlcxt->dest) {
			for (i = 0; i < nremotes; ++i) {
				if (remlist[i] == mbr->rem.Rcomp) {
					ctlmbrs[i] = mbr;
					fprintf(stdout, "Remote %i [%.4s] connected\n", i + 1,
						uuid2str(remlist[i]->uuid, uuidstr));
				}
			}
		}
		break;
	case EV_REMDISCONNECT:  /* object = Lchan, info = memb */
	case EV_LOCDISCONNECT:  /* object = Lchan, info = memb */
		Lchan = (struct Lchannel_s *)object;
		mbr = (struct member_s *)info;

		if (ctlcxt && Lchan == ctlcxt->dest) {
			for (i = 0; i < nremotes; ++i) {
				if (ctlmbrs[i] == mbr) {
					ctlmbrs[i] = NULL;
					fprintf(stdout, "Remote %i [%.6s] disconnected\n", i + 1,
						uuid2str(remlist[i]->uuid, uuidstr));
				}
			}
		}
		break;
	case EV_DISCOVER:  /* object = Rcomp, info = discover data in packet */
	case EV_JOINSUCCESS:  /* object = Lchan, info = memb */
	case EV_JOINFAIL:  /* object = Lchan, info = memb->rem.Rcomp */
	case EV_LOCCLOSE:  /* object = , info =  */
	case EV_LOCDISCONNECTING:  /* object = Lchan, info = memb */
	case EV_LOCLEAVE:  /* object = Lchan, info = memb */
	case EV_LOSTSEQ:  /* object = Lchan, info = memb */
	case EV_MAKTIMEOUT:  /* object = Lchan, info = memb */
	case EV_NAKTIMEOUT:  /* object = Lchan, info = memb */
	case EV_REMLEAVE:  /* object = , info =  */
	default:
		break;
	}
	LOG_FEND();
}

/**********************************************************************/
static bool
wordmatch(char **str, const char *word, int minlen)
{
	char *bp;

	bp = *str;
	for (bp = *str; *word && *bp == *word; ++bp, ++word, --minlen);

	if ((*bp == 0 || isspace(*bp)) && (*word == 0 || minlen <= 0)) {
		*str = bp;
		return true;
	}
	return false;
}
/**********************************************************************/
static int
readU32(char **bpp, uint32_t *ip)
{
	char *bp, *cp;
	uint32_t l;
	
	bp = *bpp;
	l = strtoul(bp, &cp, 0);
	if (cp == bp || (*cp != 0 && !isspace(*cp))) {
		return -1;
	}
	*ip = l;
	*bpp = cp;
	return 0;
}

#define readS32(bpp, lp) readU32(bpp, (unsigned long *)(lp))
/**********************************************************************/
int
readremote(char **bpp)
{
	uint32_t rem;

	if (readU32(bpp, &rem) < 0) {
		fprintf(stdout, "Please specify a remote device by number\n");
		return -1;
	}
	--rem;  /* change to zero-based */
	if (rem >= nremotes) {
		fprintf(stdout, "Specified remote number is out of range\n");
		return -1;
	}
	return (int)rem;
}
/**********************************************************************/
void
ddltree(char **bpp)
{
	struct Rcomponent_s *Rcomp;
	const struct rootdev_s *root;
	const uint8_t *rootdcid;
	int rem;
	int i;

	if ((rem = readremote(bpp)) < 0) return;
	Rcomp = remlist[rem];
	
	if ((rootdcid = finduuid(&devtrees, Rcomp->slp.dcid)) != NULL) {
		root = container_of(rootdcid, struct rootdev_s, dcid[0]);
	} else {
		char dcidstr[UUID_STR_SIZE];
		union addrmap_u *amap;
		uint32_t adrange;
		unsigned int ixratio;

		fprintf(stdout, "Parsing DDL\n");
		uuid2str(Rcomp->slp.dcid, dcidstr);
		root = parsedevice(dcidstr);
		if (root == NULL) {
			acnlog(lgERR, "Can't generate device %.8s...", dcidstr);
			return;
		}
		acnlog(lgDBUG, "Add new DCID %.8s...", dcidstr);
		rootdcid = root->dcid;
		adduuid(&devtrees, root->dcid);
		amap = root->amap;
		assert(amap != NULL);
		adrange = root->maxaddr - root->minaddr;
		ixratio = (adrange * sizeof(void *))
				/ (amap->srch.count * sizeof(struct addrfind_s));

		acnlog(lgDBUG, "Address range %u, index:search map size ratio %u",
				adrange, ixratio);
		if (adrange <= 64 || ixratio < 3) {
			/* other criteria for conversion could be used */
			acnlogmark(lgDBUG, "Transforming map");
			xformtoindx(amap);
		} else {
			acnlogmark(lgDBUG, "Using search map");
		}
	}
	acnlog(lgDBUG, "Assign new map to all devices of this DCID");
	for (i = 0; i < nremotes; ++i) {
		Rcomp = remlist[i];
		assert(Rcomp && Rcomp->slp.dcid);
		acnlogmark(lgDBUG, "try %d \"%s\"...", i, Rcomp->slp.uacn);
		if (Rcomp->dmp.amap == NULL
			&& uuidsEq(rootdcid, Rcomp->slp.dcid))
		{
			acnlogmark(lgDBUG, "...assigning");
			Rcomp->dmp.amap = root->amap;
		}
	}
	acnlog(lgDBUG, "all assigned");
	fprintf(stdout,
			"DDL tree for device%3u\n"
			"----------------------\n"
			, rem + 1);
	printtree(root->ddlroot);
}
/**********************************************************************/
void
dodiscover()
{
	char *ctyp;
	struct Rcomponent_s *Rcomp;
	int i;
	char uuidstr[UUID_STR_SIZE];
	const uint8_t *rootdcid;

	nremotes = 0;

	discover();
	if (Rcomponents.first == NULL) {
		fprintf(stdout, "Nothing discovered\n");
	} else {
		i = 0;
	
		FOR_EACH_UUID(&Rcomponents, Rcomp, struct Rcomponent_s, uuid[0]) {

			switch (Rcomp->slp.flags & (slp_ctl | slp_dev)) {
			case slp_ctl:
				ctyp = "controller";
				break;
			case slp_dev:
				ctyp = "device";
				break;
			case slp_ctl | slp_dev:
				ctyp = "controller+device";
				break;
			default:
				ctyp = "";
				break;
			}
			fprintf(stdout, "%3i:  %s %s \"%s\" at %s:%-5d  [%.4s]\n",
				i + 1,
				Rcomp->slp.fctn,
				ctyp,
				Rcomp->slp.uacn,
				inet_ntoa(netx_SINADDR(&Rcomp->sdt.adhocAddr)),
				ntohs(netx_PORT(&Rcomp->sdt.adhocAddr)),
				uuid2str(Rcomp->uuid, uuidstr)
			);
			rootdcid = finduuid(&devtrees, Rcomp->slp.dcid);
			if (rootdcid != NULL) {
				const struct rootdev_s *root;

				root = container_of(rootdcid, struct rootdev_s, dcid[0]);
				Rcomp->dmp.amap = root->amap;
			}
			if (i < MAX_REMOTES) {
				remlist[i] = Rcomp;
			}
			++i;

		} NEXT_UUID()
		nremotes = (i <= MAX_REMOTES) ? i : MAX_REMOTES;
	}
}
/**********************************************************************/
void
dmpconnect(char **bpp)
{
	int rem;
	struct member_s *mbr;
	int i;

	LOG_FSTART();
	if ((rem = readremote(bpp)) < 0) return;
	if ((mbr = ctlmbrs[rem]) == NULL) {
		struct Rcomponent_s *Rcomp;
		struct Lchannel_s *ctlchan;

		if (ctlcxt == NULL) {
			ctlcxt = acnNew(struct dmptcxt_s);
			ctlchan = openChannel(CHF_NOCLOSE, NULL);  /* use default parameters */
	
			if (ctlchan == NULL) {
				free(ctlcxt);
				ctlcxt = NULL;
				acnlogmark(lgNTCE, "Unable to open control channel");
				return;
			}
			ctlcxt->wflags = WRAP_ALL_MEMBERS;
			ctlcxt->dest = ctlchan;
		} else if (ctlcxt->wflags & WRAP_ALL_MEMBERS) {
			ctlchan = (struct Lchannel_s *)ctlcxt->dest;
		} else {
			ctlchan = ((struct member_s *)ctlcxt->dest)->rem.Lchan;
		}

		Rcomp = remlist[rem];
		i = addMember(ctlchan, Rcomp);
		if (i < 0) return;
	} else if (mbr->connect == 0) {
		fprintf(stdout, "SDT established - awaiting DMP connection\n");
	} else {
		fprintf(stdout, "Remote %d is already connected\n", rem);
	}
	LOG_FEND();
}
/**********************************************************************/
void
setintprop(
	struct member_s *mbr,
	const struct dmpprop_s *dprop, 
	uint32_t addr,
	uint32_t pval
)
{
	struct adspec_s ads;
	uint8_t *txp;

	LOG_FSTART();
	ads.inc = ads.count = 1;
	ads.addr = addr;

	assert(ctlcxt->pdup == NULL);
	ctlcxt->dest = mbr;
	ctlcxt->wflags = WRAP_REL_ON;
	txp = dmp_openpdu(ctlcxt, DMP_SET_PROPERTY << 8 | DMPAD_SINGLE,
							&ads, dprop->size);
	switch (dprop->size) {
	case 1:
		txp = marshalU8(txp, (uint8_t)pval);
		break;
	case 2:
		txp = marshalU16(txp, (uint16_t)pval);
		break;
	case 4:
		txp = marshalU32(txp, pval);
		break;
	default:
		acnlogmark(lgERR, "Unsupported integer property size %u", dprop->size);
		dmp_abortblock(ctlcxt);
		return;
	}
	dmp_closeflush(ctlcxt, txp);
	LOG_FEND();
}
/**********************************************************************/
void
setstringprop(
	struct member_s *mbr,
	const struct dmpprop_s *dprop,
	uint32_t addr,
	char *str,
	int len
)
{
	struct adspec_s ads;
	uint8_t *txp;

	LOG_FSTART();
	ads.inc = ads.count = 1;
	ads.addr = addr;
	ctlcxt->wflags = WRAP_REL_ON;
	acnlogmark(lgERR, "Set string prop %u to \"%.*s\"", addr, len, str);

	assert(ctlcxt->pdup == NULL);
	ctlcxt->dest = mbr;
	txp = dmp_openpdu(ctlcxt, DMP_SET_PROPERTY << 8 | DMPAD_SINGLE,
							&ads, len + 2);
	txp = marshalVar(txp, (uint8_t *)str, len);
	dmp_closeflush(ctlcxt, txp);
	LOG_FEND();
}
/**********************************************************************/
void
setprop(char **bpp)
{
	int rem;
	struct member_s *mbr;
	uint32_t addr;
	union addrmap_u *amap;
	const struct dmpprop_s *dprop;

	if ((rem = readremote(bpp)) < 0) return;
	if ((mbr = ctlmbrs[rem]) == NULL) {
		fprintf(stdout, "Remote %i \"%s\" is not connected\n", rem,
				remlist[rem]->slp.uacn);
		return;
	}
	if ((amap = remlist[rem]->dmp.amap) == NULL) {
		fprintf(stdout, "Remote %i \"%s\": check DDL\n", rem, 
					remlist[rem]->slp.uacn);
		return;
	}
	if (readU32(bpp, &addr) < 0) {
		fprintf(stdout, "Must specify a property number\n");
		return;
	}
	if ((dprop = addr_to_prop(amap, addr)) == NULL
		|| !(dprop->flags & pflg(write))) {
		fprintf(stdout, "No such property or not writable\n");
		return;
	}
	switch (dprop->etype) {
	case etype_enum:
	case etype_uint:
	case etype_sint: {
		uint32_t pval;

		if (readU32(bpp, &pval) < 0) {
			fprintf(stdout, "Integer property needed\n");
			return;
		}
		setintprop(mbr, dprop, addr, pval);
		break;}
	case etype_string: {
		char *cp, *ep;

		cp = *bpp;
		while (isspace(*cp)) ++cp;
		if (*cp == '"') {
			for (ep = ++cp; *ep && *ep != '"'; ++ep);
		} else {
			for (ep = cp; *ep && !isspace(*ep); ++ep);
		}
		*bpp = ep;

		setstringprop(mbr, dprop, addr, cp, (int)(ep - cp));
		break;}
	case etype_unknown:
	case etype_boolean:
	case etype_float:
	case etype_UTF8:
	case etype_UTF16:
	case etype_UTF32:
	case etype_opaque:
	case etype_uuid:
	case etype_bitmap:
		fprintf(stdout, "%s property unsupported\n", etypes[dprop->etype]);
		break;
	}
}
/*
void
dmpdisconnect(char **bpp)
{
	int rem;
	struct member_s *mbr;
	int i;

	if ((rem = readremote(bpp)) < 0) return;
	if ((mbr = ctlmbrs[rem]) == NULL) {
		fprintf(stdout, "Not connected\n");
		return;
	}
}
*/

/**********************************************************************/
void
term_event(uint32_t evf, void *evptr)
{
	char buf[256];
	int len;
	char *bp;

	len = read(STDIN_FILENO, buf, sizeof(buf) - 1);  /* waiting input */
	if (len < 0) {
		acnlogmark(lgWARN, "read stdin error %d %s", len, strerror(errno));
		return;
	}

	if (buf[len - 1] == '\n') --len;
	buf[len] = 0;	 /* terminate */

	bp = buf;

	if (wordmatch(&bp, "discover", 2)) dodiscover();
	else if (wordmatch(&bp, "describe", 2) || wordmatch(&bp, "ddl", 2))
		ddltree(&bp);
	else if (wordmatch(&bp, "connect", 2))
		dmpconnect(&bp);
	else if (wordmatch(&bp, "setproperty", 2) || wordmatch(&bp, "sp", 2))
		setprop(&bp);
	else if (wordmatch(&bp, "quit", 1)) runstate = rs_quit;
	else {
		fprintf(stdout, "Bad command \"%s\"\n", bp);
	}
}

poll_fn * term_event_ref = &term_event;

/**********************************************************************/
static void
run_controller(const char *uuidstr, uint16_t port, const char **interfaces)
{
	netx_addr_t listenaddr;

	LOG_FSTART();
	/* initialize our component */
	if (initstr_Lcomponent(uuidstr) != 0) {
		acnlogmark(lgERR, "Init local component failed");
		return;
	}

	localComponent.lifetimer.userp = interfaces;

	/* read uacn if its been set */
	uacn_init(uuidstr);

	/* set up for ephemeral port and any interface */
	netx_INIT_ADDR_ANY(&listenaddr, port);

	/* start up ACN */
	/* prepare DMP */
	if (dmp_register() < 0) {
		acnlogerror(lgERR);
	} else if (sdt_register(&cd_sdtev, &listenaddr, ADHOCJOIN_NONE) < 0) {
		acnlogerror(lgERR);
	} else {
		if (sdt_addClient(&dmp_sdtRx, NULL) < 0) {
			acnlogerror(lgERR);
		} else {
			/* now we can advertise ourselves */
			acnlogmark(lgDBUG, "starting SLP");
			slp_register();
		
			evl_register(STDIN_FILENO, &term_event_ref, EPOLLIN);

			evl_wait();
		
			evl_register(STDIN_FILENO, NULL, 0);

			slp_deregister();
		}
		sdt_deregister();
	}
	uacn_close();
	LOG_FEND();
}

/**********************************************************************/
/*
topic: Command line options
*/
const char shortopts[] = "c:p:i:";
const struct option longopta[] = {
	{"cid", required_argument, NULL, 'c'},
	{"port", required_argument, NULL, 'p'},
	{"interface", required_argument, NULL, 'i'},
	{NULL,0,NULL,0}
};
/**********************************************************************/
/*
func: main

Parse command line options and start controller
*/
int
main(int argc, char *argv[])
{
	int opt;
	const char *uuidstr = NULL;
	long int port = netx_PORT_EPHEM;
	const char *interfacesb[MAXINTERFACES + 1];
	int ifc = 0;
	
	LOG_FSTART();

	while ((opt = getopt_long(argc, argv, shortopts, longopta, NULL)) != -1) {
		switch (opt) {
		case 'c':
			uuidstr = optarg;
			break;
		case 'p': {
			char *ep = NULL;
			unsigned long u;

			u = strtoul(optarg, &ep, 0);
			if (ep && *ep == 0 && u <= 0xffff) {
				port = htons(u);
			} else {
				acnlogmark(lgERR, "Bad port specification \"%s\" ignored\n", optarg);
				port = netx_PORT_EPHEM;
			}
			break;
		case 'i':
			if (ifc < MAXINTERFACES) interfacesb[ifc++] = optarg;
			break;
		}
		case '?':
		default:
			exit(EXIT_FAILURE);
		}
	}
	if (uuidstr == NULL) {
		fprintf(stderr, "No CID specified\n");
		exit(EXIT_FAILURE);
	} else if (str2uuid(uuidstr, NULL) != 0) {
		fprintf(stderr, "Cannot parse CID \"%s\"\n", uuidstr);
		exit(EXIT_FAILURE);
	}
	//snprintf(serialno, sizeof(serialno), "%.8s:%.8s", DCID_STR, uuidstr);

	interfacesb[ifc] = NULL;  /* terminate */
	run_controller(uuidstr, port, interfacesb);

	LOG_FEND();
	return 0;
}
