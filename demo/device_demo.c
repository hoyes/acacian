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

struct dmptcxt_s *evcxt = NULL;

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

uint16_t barvals[DIM_bargraph__0] = {0};
/**********************************************************************/
/*
Because properties have their own functions we don't need component ones
(see ACNCFG_PROPEXT_FNS)
*/

int dd_unused(struct dmprcxt_s *rcxt, 
		const struct dmpprop_s *dprop,
		struct adspec_s *ads)
{
	/* should never get called */
	return -1;
}

int getbar(struct dmprcxt_s *rcxt, 
		const struct dmpprop_s *dprop,
		struct adspec_s *ads);

int setbar(struct dmprcxt_s *rcxt, 
		const struct dmpprop_s *dprop,
		struct adspec_s *ads, const uint8_t *data, bool multi);

int subscribebar(struct dmprcxt_s *rcxt, 
		const struct dmpprop_s *dprop,
		struct adspec_s *ads);

int unsubscribebar(struct dmprcxt_s *rcxt, 
		const struct dmpprop_s *dprop,
		struct adspec_s *ads);

int getconststr(struct dmprcxt_s *rcxt, 
		const struct dmpprop_s *dprop,
		struct adspec_s *ads);

int getUACN(struct dmprcxt_s *rcxt, 
		const struct dmpprop_s *dprop,
		struct adspec_s *ads);

int setUACN(struct dmprcxt_s *rcxt, 
		const struct dmpprop_s *dprop,
		struct adspec_s *ads, const uint8_t *data, bool multi);

void dd_connect(struct cxn_s *cxn, bool connect);

struct Lcomponent_s localComponent = {
	.fctn = FCTN,
	.uacn = UACN,
	.dmp = {
		.amap = &addr_map,
		.rxvec = {
			[DMP_reserved0]             = NULL,
			[DMP_GET_PROPERTY]          = &dd_unused,
			[DMP_SET_PROPERTY]          = &dd_unused,
			[DMP_GET_PROPERTY_REPLY]    = NULL,
			[DMP_EVENT]                 = NULL,
			[DMP_reserved5]             = NULL,
			[DMP_reserved6]             = NULL,
			[DMP_SUBSCRIBE]             = &dd_unused,
			[DMP_UNSUBSCRIBE]           = &dd_unused,
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
static void
dmp2flat(const struct dmpprop_s *dprop, struct adspec_s *dmpads, struct adspec_s *flatads)
{
	int nprops;
	uint32_t ofs;
	uint32_t ix, aix, pinc;
	const struct dmpdim_s *dp;

	nprops = 1;
	ofs = dmpads->addr - dprop->addr;
	pinc = 0;
	aix = 0;

	assert((dprop->flags & pflg(overlap)) == 0);  /* wont work for self overlapping dims */
	for (dp = dprop->dim; dp < dprop->dim + dprop->ndims; dp++) {
		ix = ofs / dp->inc;
		aix = aix * dp->cnt + ix;
		ofs = ofs % dp->inc;
		if (dmpads->inc % dp->inc == 0) {  /* ranging over this dimension */
			pinc = dmpads->inc / dp->inc;
			nprops = (dp->cnt - ix) / pinc;
			if (nprops > dmpads->count) nprops = dmpads->count;
		} else pinc *= dp->cnt;
	}
	assert(ofs == 0);  /* If not then addr_to_prop or other logic is wrong */
	flatads->addr = aix;
	flatads->inc = pinc;
	flatads->count = nprops;
}

/**********************************************************************/
static void
flat2dmp(const struct dmpprop_s *dprop, struct adspec_s *flatads, struct adspec_s *dmpads)
{
	uint32_t ofs, pinc;
	const struct dmpdim_s *dp;

	dmpads->addr = dprop->addr;
	dmpads->count = flatads->count;
	ofs = flatads->addr;
	pinc = flatads->inc;

	for (dp = dprop->dim + dprop->ndims; --dp >= dprop->dim;) {
		dmpads->addr += (ofs % dp->cnt) * dp->inc;
		ofs = ofs / dp->cnt;
		if (pinc) {
			if (pinc % dp->cnt) {
				dmpads->inc = pinc * dp->inc;
				pinc = 0;
			} else {
				pinc = pinc / dp->cnt;
			}
		}
	}
}

/**********************************************************************/
void
declare_bars(
	struct dmptcxt_s *tcxt, 
	struct adspec_s *dmpads,
	struct adspec_s *flatads,
	uint8_t vec
)
{
	uint8_t *txp;
	int i;
	unsigned int ofs;
	struct dmpprop_s *dprop = &DMP_bargraph;

	LOG_FSTART();
	dmpads->count = flatads->count;
	txp = dmp_openpdu(tcxt, vec << 8 | DMPAD_RANGE_STRUCT, dmpads, dprop->size * flatads->count);

	for (ofs = flatads->addr, i = flatads->count; --i;) {
		txp = marshalU16(txp, barvals[ofs]);
		ofs += flatads->inc;
	}
	dmp_closepdu(tcxt, txp);
	LOG_FEND();
}
/**********************************************************************/
int getbar(struct dmprcxt_s *rcxt, 
		const struct dmpprop_s *dprop,
		struct adspec_s *dmpads)
{
	struct adspec_s flatads;

	dmp2flat(dprop, dmpads, &flatads);
	declare_bars(&rcxt->rspcxt, dmpads, &flatads, DMP_GET_PROPERTY_REPLY);
	return flatads.count;
}

/**********************************************************************/
int setbar(struct dmprcxt_s *rcxt, 
		const struct dmpprop_s *dprop,
		struct adspec_s *dmpads, const uint8_t *data, bool multi)
{
	struct adspec_s flatads;
	int i;
	unsigned int ofs;

	LOG_FSTART();
	assert(dprop == &DMP_bargraph);

	dmp2flat(dprop, dmpads, &flatads);

	for (ofs = flatads.addr, i = flatads.count; i--;) {
		barvals[ofs] = unmarshalU16(data);
		data += 2;
		ofs += flatads.inc;
	}

	if ((dprop->flags & pflg(event)) && evcxt) {
		declare_bars(evcxt, dmpads, &flatads, DMP_EVENT);
		dmp_flushpdus(evcxt);
	}
	LOG_FEND();
	return flatads.count;
}
/**********************************************************************/
/*
*/
struct cxnpars_s ev_cxnparams = {
	.expiry_sec = 10,
};

int subscribebar(struct dmprcxt_s *rcxt, 
		const struct dmpprop_s *dprop,
		struct adspec_s *dmpads)
{
	struct cxn_s *evcxn;
	struct cxngp_s *cxngp;
	int i;
	struct adspec_s flatads;

	dmp2flat(dprop, dmpads, &flatads);

	if (!evcxt) {
		evcxt = acnNew(struct dmptcxt_s);
		cxngp = new_cxngp(0, NULL);  /* use default parameters */

		if (cxngp == NULL) {
			free(evcxt);
			evcxt = NULL;
			acnlogmark(lgNTCE, "Unable to open event group");
			return -1;
		}
		evcxt->dest.cxngp = cxngp;
		evcxt->wflags = WRAP_ALL_MEMBERS /* | WRAP_REL_ON */;
	}

	/*
	request the connection - sync event gets sent when we get a callback
	(in dd_connect()) to say connection is made.
	*/
	i = dmp_connectRq(cxngp, cxnRcomp(rcxt->cxn));
	if (i < 0) return i;
	return flatads.count;
}
/**********************************************************************/

int unsubscribebar(struct dmprcxt_s *rcxt, 
		const struct dmpprop_s *dprop,
		struct adspec_s *dmpads)
{
	return dmpads->count;
}
/**********************************************************************/

int getconststr(struct dmprcxt_s *rcxt, 
		const struct dmpprop_s *dprop,
		struct adspec_s *dmpads)
{
	return dmpads->count;
}
/**********************************************************************/

int getUACN(struct dmprcxt_s *rcxt, 
		const struct dmpprop_s *dprop,
		struct adspec_s *dmpads)
{
	return dmpads->count;
}
/**********************************************************************/

int setUACN(struct dmprcxt_s *rcxt, 
		const struct dmpprop_s *dprop,
		struct adspec_s *dmpads, const uint8_t *data, bool multi)
{
	return dmpads->count;
}

/**********************************************************************/
void dd_connect(struct cxn_s *cxn, bool connect)
{
	if (connect && evcxt && getcxngp(cxn) == evcxt->dest.cxngp) {
		struct adspec_s flatads = {0, 1, DIM_bargraph__0};
		struct adspec_s dmpads = {DMP_bargraph.addr, INC_bargraph__0, DIM_bargraph__0};

		declare_bars(evcxt, &dmpads, &flatads, DMP_SYNC_EVENT);
		dmp_flushpdus(evcxt);
	}
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
sel_X(int x)
{
	barsel += x;
	if (barsel > DIM_bargraph__0 - 1) barsel = DIM_bargraph__0 - 1;
	else if (barsel < 0) barsel = 0;
}
/**********************************************************************/
static void
bar_Y(int y)
{
	int v;
	

	v = barvals[barsel] + y;
	if (v > IMMP_barMax) v = IMMP_barMax;
	else if (v < 0) v = 0;
	barvals[barsel] = v;

	if (evcxt) {
		struct adspec_s dmpads;
		struct adspec_s flatads = {barsel, 1, 1};

		flat2dmp(&DMP_bargraph, &flatads, &dmpads);
		declare_bars(evcxt, &dmpads, &flatads, DMP_EVENT);
		dmp_flushpdus(evcxt);
	}
}
/**********************************************************************/
#define Ctl(x) (x + 1 - 'A')
#define ESC Ctl('[')
#define MV_U 'A'
#define MV_D 'B'
#define MV_R 'C'
#define MV_L 'D'
#define MV_HOME 'H'
#define MV_END 'F'

void
term_event(uint32_t evf, void *evptr)
{
	char buf[16];
	int i;
	int n;
	int c;
	static unsigned int escp = 0;
	int unsigned escn;
	static int csav;

	n = read(STDIN_FILENO, buf, sizeof(buf));  /* read character or control sequence */
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
		escn = 0;  /* reset escape state by default */
		switch (escp) {
		case 0:	/* normal case */
			switch (c) {
			case Ctl('C'): case Ctl('D'): case Ctl('Q'):
			case 'q': case 'Q':
				runstate = rs_quit;
				break;
			case ESC:
				escn = 1;
				break;
			case 'l': case '<':
				sel_X(-1);
				break;
			case 'r': case '>':
				sel_X(1);
				break;
			case '+': case 'u':
				bar_Y(1);
				break;
			case '-': case 'd':
				bar_Y(-1);
				break;
			case '0': case 'Z': case 'z':
				barvals[barsel] = 0;
				break;
			case 'F': case 'f':
				barvals[barsel] = IMMP_barMax;
				break;
			}
			break;
		case 1:
			switch (c) {
			case '[':  /* CSI */
				escn = 2;
				break;
			case 'O':  /* Home/End introducer */
				escn = 4;
				break;
			}
			break;
		case 2:  /* ESC [ (CSI) */
			switch (c) {
			case MV_L:  /* Left arrow */
				sel_X(-1);
				break;
			case MV_R:  /* Right arrow */
				sel_X(1);
				break;
			case MV_D:  /* Down arrow */
				bar_Y(-1);
				break;
			case MV_U:  /* Up arrow */
				bar_Y(1);
				break;
			case '5':
			case '6':
				escn = 3;
				break;
			case '1':
				escn = 5;
				break;
			}
			break;
		case 3:  /* ESC '[' '5|6' */
			switch (c) {
			case '~':  /* Page Up/Down */
				if (csav == '5') bar_Y(10);
				else bar_Y(-10);
				break;
			}
		case 4:  /* ESC 'O' */
			switch (c) {
			case MV_HOME:  /* Home */
				barsel = 0;
				break;
			case MV_END:  /* End */
				barsel = DIM_bargraph__0 - 1;
				break;
			}
			break;
		case 5:  /* ESC '[' '1' */
			switch (c) {
			case ';':
				escn = 6;
				break;
			}
			break;
		case 6:  /* ESC '[' '1' ';' */
			switch (c) {
			case '2': case '3':
				escn = 7;
				break;
			}
			break;
		case 7:  /* ESC '[' '1' ';' '2' */
			switch (c) {
			case MV_U:  /* Shift Up arrow, Command Up arrow */
				bar_Y(10);
				break;
			case MV_D:  /* Shift Down arrow, Command Down arrow */
				bar_Y(-10);
				break;
			case MV_L:  /* Shift Left arrow, Command Left arrow (= Page left) */
				sel_X(-10);
				break;
			case MV_R:  /* Shift Right arrow, Command Right arrow (= Page right) */
				sel_X(10);
				break;
			}
			break;
		}
		escp = escn;
		csav = c;
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
