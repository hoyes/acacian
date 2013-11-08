/**********************************************************************/
/*
Copyright (c) 2013, Philip Nye
All rights reserved.

  $Id$

#tabs=3
*/
/**********************************************************************/
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
void cd_sdtev(int event, void *object, void *info)
{

	LOG_FSTART();
	switch (event) {
	case EV_RCONNECT:  /* object = Lchan, info = memb */
	case EV_LCONNECT:  /* object = Lchan, info = memb */
	case EV_REMDISCONNECT:  /* object = Lchan, info = memb */
	case EV_LOCDISCONNECT:  /* object = Lchan, info = memb */
	case EV_DISCOVER:  /* object = Rcomp, info = discover data in packet */
	case EV_JOINSUCCESS:  /* object = Lchan, info = memb */
	case EV_JOINFAIL:  /* object = Lchan, info = memb->rem.Rcomp */
	case EV_LOCCLOSE:  /* object = , info =  */
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
struct Rcomponent_s *remlist[MAX_REMOTES];
struct member_s *ctlmbrs[MAX_REMOTES] = {NULL,};
int nremotes = 0;
struct uuidset_s devtrees;

/**********************************************************************/
static bool
wordmatch(char **str, const char *word, int minlen)
{
	char *bp;

	bp = *str;
	for (bp = *str; *word && *bp == *word; ++bp, ++word, --minlen);

	if ((*bp == 0 || *bp == ' ') && (*word == 0 || minlen <= 0)) {
		*str = bp;
		return true;
	}
	return false;
}
/**********************************************************************/
int
readint(char **bpp, int *ip, int min, int max)
{
	char *bp, *cp;
	long l;
	
	bp = *bpp;
	l = strtol(bp, &cp, 0);
	if (cp == bp || (*cp != 0 && !isspace(*cp))) {
		return -1;
	}
	if (l < min || l > max) {
		return -2;
	}
	*ip = l;
	*bpp = cp;
	return 0;
}
/**********************************************************************/
int
readremote(char **bpp)
{
	int rem;

	switch (readint(bpp, &rem, 1, nremotes)) {
	case -1:
		fprintf(stdout, "Please specify a remote device by number\n");
		return -1;
	case -2:
		fprintf(stdout, "Specified remote number is out of range\n");
		/* fall through */
	default:
		return -1;
	case 0:
		return rem - 1;
	}
}
/**********************************************************************/
void
ddltree(char **bpp)
{
	struct Rcomponent_s *Rcomp;
	const struct rootdev_s *root;
	const uint8_t *rootdcid;
	int rem;

	if ((rem = readremote(bpp)) < 0) return;
	Rcomp = remlist[rem];
	
	rootdcid = finduuid(&devtrees, Rcomp->slp.dcid);
	root = container_of(rootdcid, struct rootdev_s, dcid[0]);
	if (root == NULL) {
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
		adduuid(&devtrees, root->dcid);
		amap = root->amap;
		adrange = root->maxaddr - root->minaddr;
		ixratio = (adrange * sizeof(void *))
				/ (amap->srch.count * sizeof(struct addrfind_s));

		if (adrange <= 64 || ixratio < 3) {
			/* other criteria for conversion could be used */
			acnlogmark(lgDBUG, "Transforming map\n");
			xformtoindx(amap);
		}
		Rcomp->dmp.amap = amap;
	}
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
			fprintf(stdout, "%3i:  %s %s \"%s\" at %s:%-5d  [%.8s]\n",
				i + 1,
				Rcomp->slp.fctn,
				ctyp,
				Rcomp->slp.uacn,
				inet_ntoa(netx_SINADDR(&Rcomp->sdt.adhocAddr)),
				ntohs(netx_PORT(&Rcomp->sdt.adhocAddr)),
				uuid2str(Rcomp->uuid, uuidstr)
			);
			if (i < MAX_REMOTES) {
				remlist[i] = Rcomp;
			}
			++i;

		} NEXT_UUID()
		nremotes = (i <= MAX_REMOTES) ? i : MAX_REMOTES;
	}
}
/**********************************************************************/
struct dmptcxt_s *ctlcxt = NULL;

/**********************************************************************/
void
dmpconnect(char **bpp)
{
	int rem;
	struct member_s *mbr;
	int i;

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
			ctlcxt->wflags = 0;
			ctlcxt->dest = ctlchan;
		} else {
			ctlchan = ctlcxt->dest;
		}
		
		Rcomp = remlist[rem];
		i = addMember(ctlchan, Rcomp);
		if (i < 0) return;
	} else if (mbr->connect == 0) {
		fprintf(stdout, "SDT established - awaiting DMP connection\n");
	} else {
		fprintf(stdout, "Remote %d is already connected\n", rem);
	}
}
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
	} else if (sdt_register(&cd_sdtev) < 0) {
		acnlogerror(lgERR);
	} else {
		if (sdt_setListener(NULL, &listenaddr) < 0) {
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
			sdt_clrListener();
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
