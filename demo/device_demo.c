/**********************************************************************/
/*
Copyright (c) 2013, Philip Nye
All rights reserved.

#tabs=3t
*/
/**********************************************************************/

#include "acn.h"


#include <stdio.h>
#include <expat.h>
#include <unistd.h>
#include <getopt.h>
#include <termios.h>
#include <ctype.h>
/*
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

*/
//#include "ddl/parse.h"
//#include "propmap.h"
//#include "ddl/behaviors.h"
//#include "ddl/printtree.h"
#include "demo_utils.h"
#include "devicemap.h"

/**********************************************************************/
/*
Logging facility
*/

#define lgFCTY LOG_APP


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
const uint16_t default_port = 56789;

const char hardversion[] = "0";
const char softversion[] = "$swrev$";
char serialno[20];

/**********************************************************************/
/*
Fix some values
*/

const char FCTN[] = IMMP_deviceID_modelname;
char UACN[ACN_UACN_SIZE] = IMMP_deviceID_defaultname;

static struct termios savetty;
static bool termin, termout;

/**********************************************************************/
/*
Each property in the "bar graph" is printed as a BAR_PLACES wide 
integer with a select indicator string either side. Between bars 
is a BAR_GAP string.

Select indicators:

The currently selected property has BAR_LSEL and BAR_RSEL strings to
left and right respectively, whilst unselected properties have BAR_LUNSEL
and BAR_RUNSEL strings.
*/
#define BAR_PLACES (\
	(IMMP_barMax < 10    ) ? 1 :\
	(IMMP_barMax < 100   ) ? 2 :\
	(IMMP_barMax < 1000  ) ? 3 :\
	(IMMP_barMax < 10000 ) ? 4 :\
	(IMMP_barMax < 100000) ? 5 :\
	10)

const char bar_gap[]    = "  ";
const char bar_lunsel[] = " ";
const char bar_runsel[] = " ";
const char bar_lsel[]   = "[";
const char bar_rsel[]   = "]";

#define BAR_WIDTH (strlen(bar_gap) + strlen(bar_lunsel) + BAR_PLACES + strlen(bar_runsel))
#define BARVAL(i) 

uint16_t barvals[DIM_bargraph__1 * DIM_bargraph__0] = {0};
/**********************************************************************/
const uint8_t *dd_getprop(struct dmptcxt_s *cxtp, 
		const struct dmpprop_s *dprop, struct dmppdata_s *pdat, bool dmany);
const uint8_t *dd_setprop(struct dmptcxt_s *cxtp, 
		const struct dmpprop_s *dprop, struct dmppdata_s *pdat, bool dmany);
const uint8_t *dd_subscribe(struct dmptcxt_s *cxtp, 
		const struct dmpprop_s *dprop, struct dmppdata_s *pdat, bool dmany);
const uint8_t *dd_unsubscribe(struct dmptcxt_s *cxtp, 
		const struct dmpprop_s *dprop, struct dmppdata_s *pdat, bool dmany);

void dd_connect(struct cxn_s *cxn, bool connect);

struct Lcomponent_s localComponent = {
	.fctn = FCTN,
	.uacn = UACN,
	.dmp = {
		.amap = &addr_map,
		.rxvec = {
			[DMP_reserved0]             = NULL,
			[DMP_GET_PROPERTY]          = &dd_getprop,
			[DMP_SET_PROPERTY]          = &dd_setprop,
			[DMP_GET_PROPERTY_REPLY]    = NULL,
			[DMP_EVENT]                 = NULL,
			[DMP_reserved5]             = NULL,
			[DMP_reserved6]             = NULL,
			[DMP_SUBSCRIBE]             = &dd_subscribe,
			[DMP_UNSUBSCRIBE]           = &dd_unsubscribe,
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
		.cxnev = &dd_connect,
		.flags = 0,
	}
};

/**********************************************************************/
int
declare_prop(
	struct dmptcxt_s *cxtp, 
	const struct dmpprop_s *dprop,
	struct dmppdata_s *pdat,
	bool dmany
)
{
	int count;
	uint32_t indexes[dprop->ndims ? dprop->ndims : 1];

	if (dprop->ndims) {
		fillindexes(dprop, pdat, indexes);
	} else {
		indexes[0] = 0;
	}
	count = 1;
	if (pdat->count > 1) {
		for (i = 0; i < dprop->ndims; ++i) {
			if (dprop->dim[i].i == pdat->inc) {
				count = pdat->count;
				if (indexes[i] + count > dprop->dim[i].r + 1 - indexes[i])
					count = dprop->dim[i].r + 1 - indexes[i];
				break;
			}
		}
	}
}
/**********************************************************************/
const uint8_t *
dd_getprop(
	struct dmptcxt_s *cxtp, 
	const struct dmpprop_s *dprop,
	struct dmppdata_s *pdat,
	bool dmany
)
{
}
/**********************************************************************/
const uint8_t *
dd_setprop(
	struct dmptcxt_s *cxtp, 
	const struct dmpprop_s *dprop,
	struct dmppdata_s *pdat,
	bool dmany
)
{
}
/**********************************************************************/
const uint8_t *
dd_subscribe(
	struct dmptcxt_s *cxtp, 
	const struct dmpprop_s *dprop,
	struct dmppdata_s *pdat,
	bool dmany
)
{

}
/**********************************************************************/
const uint8_t *
dd_unsubscribe(
	struct dmptcxt_s *cxtp, 
	const struct dmpprop_s *dprop,
	struct dmppdata_s *pdat,
	bool dmany
)
{
}
/**********************************************************************/
void dd_connect(struct cxn_s *cxn, bool connect)
{
}
/**********************************************************************/
static int barsel = 0;

void
showbars(void)
{
	int i;
	char buf[BAR_WIDTH * DIM_bargraph__0 + 2];
	char *bp;

	LOG_FSTART();
	if (!termout) {
		barsel = -1;
	}

	bp = buf;
	*bp++ = '\r';
	for (i = 0; i < DIM_bargraph__0; ++i) {
		bp += sprintf(bp, "%s%s%*u%s",
			bar_gap,
			(i == barsel) ? bar_lsel : bar_lunsel,
			BAR_PLACES, barvals[i],
			(i == barsel) ? bar_rsel : bar_runsel
		);
	}
	if ((i = write(STDOUT_FILENO, buf, bp - buf)) != bp - buf) {
		acnlogmark(lgWARN, "write [%ld] error %d %s", bp - buf, i, strerror(errno));
	}
	LOG_FEND();
}

/**********************************************************************/
static void
sel_L(void)
{
	if (barsel > 0) --barsel;
}
/**********************************************************************/
static void
sel_R(void)
{
	if (barsel < DIM_bargraph__0 - 1) ++barsel;
}
/**********************************************************************/
static void
sel_D(void)
{
	if (barsel >= 0 && barvals[barsel] > 0) --barvals[barsel];
}
/**********************************************************************/
static void
sel_U(void)
{
	if (barsel >= 0 && barvals[barsel] < IMMP_barMax) ++barvals[barsel];
}

/**********************************************************************/
#define ESC 0x1b
#define CtlC 3
#define MV_U 'A'
#define MV_D 'B'
#define MV_R 'C'
#define MV_L 'D'

void
term_event(uint32_t evf, void *evptr)
{
	char buf[256];
	int i;
	int n;
	int c;
	static unsigned int escp = 0;
	int unsigned escn;

	n = read(STDIN_FILENO, buf, sizeof(buf));
	if (n < 0) {
		acnlogmark(lgWARN, "read stdin error %d %s", n, strerror(errno));
		return;
	}
	for (i = 0; i < n; ++i) {
		c = buf[i] & 0xff;
		if (isgraph(c)) {
			acnlogmark(lgDBUG, "char '%c'", c);
		} else {
			acnlogmark(lgDBUG, "char 0x%02x", c);
		}
		escn = 0;
		switch (escp) {
		case 0:	/* normal case */
			switch (c) {
			case CtlC:
			case 'q':
			case 'Q':
				runstate = rs_quit;
				break;
			case ESC:
				escn = 1;
				break;
			case 'l':
			case '<':
				sel_L();
				break;
			case 'r':
			case '>':
				sel_R();
				break;
			case '+':
			case 'u':
				sel_U();
				break;
			case '-':
			case 'd':
				sel_D();
				break;
			}
			break;
		case 1:
			switch (c) {
			case '[':
				escn = 2;
				break;
			}
			break;
		case 2:
			switch (c) {
			case MV_L:
				sel_L();
				break;
			case MV_R:
				sel_R();
				break;
			case MV_D:
				sel_D();
				break;
			case MV_U:
				sel_U();
				break;
			}
			break;
		}
		escp = escn;
		showbars();
	}
}

poll_fn * term_event_ref = &term_event;

/**********************************************************************/
static void
termsetup(void)
{
	int i;
	struct termios ttyset;

	LOG_FSTART();
	termin = isatty(STDIN_FILENO);
	termout = isatty(STDOUT_FILENO);

	if (termin) {
		tcgetattr(STDIN_FILENO, &savetty);
		memcpy(&ttyset, &savetty, sizeof(ttyset));
		cfmakeraw(&ttyset);
		tcsetattr(STDIN_FILENO, TCSANOW, &ttyset);

		evl_register(STDIN_FILENO, &term_event_ref, EPOLLIN);
	} else {
		acnlogmark(lgWARN, "No terminal input");
	}
	if (!termout) {
		acnlogmark(lgWARN, "No terminal output");
	}
	LOG_FEND();
}

/**********************************************************************/

static void
termrestore(void)
{
	int i;
	static const char cleanup[] = "\n";

	LOG_FSTART();
	if (termin) {
		tcsetattr(STDIN_FILENO, TCSANOW, &savetty);
		evl_register(STDIN_FILENO, NULL, EPOLLIN);
	}
	if (termout) {
		i = write(STDOUT_FILENO, cleanup, strlen(cleanup));
	}
	LOG_FEND();
}

/**********************************************************************/
static void
run_device(const char *uuidstr, uint16_t port, const char *interfaces[])
{
	netx_addr_t listenaddr;
	char *service_url;
	char *service_atts;
	uint8_t dcid[UUID_SIZE];

	LOG_FSTART();
	/* get DCID in binary and text */
	str2uuid(DCID_str, dcid);

	/* initialize our component */
	if (initstr_Lcomponent(uuidstr) != 0) {
		acnlogmark(lgERR, "Init local component failed");
		return;
	}

	/* set up for ephemeral port and any interface */
	netx_INIT_ADDR_ANY(&listenaddr, port);

	/* start up ACN */
	if (dmp_register(&listenaddr) < 0) return;

	/* now we can advertise ourselves */
	acnlogmark(lgDBUG, "starting SLP");
	slp_start_sa(netx_PORT(&listenaddr), DCID_str, interfaces);

	termsetup();
	showbars();

	evl_wait();

	termrestore();
	LOG_FEND();

/*
unregister SLP
unregister DMP
shut Lcomp
*/
}

/**********************************************************************/
/*
func: main

Parse command line options and start device
*/
int
main(int argc, char *argv[])
{
	int opt;
	const char *uuidstr = NULL;
	uint16_t port = netx_PORT_EPHEM;
	const char *interfaces[8];
	int ifc;

	LOG_FSTART();
	ifc = 0;
	memset(interfaces, 0, sizeof(interfaces));

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
			interfaces[ifc++] = optarg;
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
	snprintf(serialno, sizeof(serialno), "%.8s:%.8s", DCID_str, uuidstr);

	run_device(uuidstr, port, interfaces);

	LOG_FEND();
	return 0;
}
