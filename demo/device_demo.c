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
const char softversion[] = "rev-$HGcset$";

/**********************************************************************/
/*
Fix some values
*/

const char FCTN[] = IMMP_devid_dev_modelname;
char UACN[ACN_UACN_SIZE] = IMMP_devid_dev_defaultname;

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
	(IMM_barMax < 10    ) ? 1 :\
	(IMM_barMax < 100   ) ? 2 :\
	(IMM_barMax < 1000  ) ? 3 :\
	(IMM_barMax < 10000 ) ? 4 :\
	(IMM_barMax < 100000) ? 5 :\
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
void
showbars(int bar)
{
	int i, p;
	char buf[BAR_WIDTH * DIM_bargraph__0 + 2];

	if (!termout) {
		return;
	} else {
		p = 0;
		buf[p++] = '\r';
		for (i = 0; i <= DIM_bargraph__0; ++i) {
			p += sprintf(buf + p, "%s%s%*u%s",
				bar_gap,
				ISSEL(i) ? bar_lsel : bar_lunsel,
				BAR_PLACES, barvals[i],
				ISSEL(i) ? bar_rsel : bar_runsel
			);
		}
	}
}

/**********************************************************************/
void
term_event(uint32_t evf, void *evptr)
{

}

const poll_fn *term_event_ref = &term_event;

/**********************************************************************/
static int
termsetup(void)
{
	int i;
	struct termios ttyset;

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
	if (termout) {
		bars = malloc
	} else {
		acnlogmark(lgWARN, "No terminal output");
	}
}

/**********************************************************************/
static void
run_device(const char *uuidstr, uint16_t port, const char *interfaces[])
{
	netx_addr_t listenaddr;
	char *service_url;
	char *service_atts;
	uint8_t dcid[UUID_SIZE];

	/* get DCID in binary and text */
	str2uuid(DCID_str, dcid);

	/* initialize our component */
	if (initstr_Lcomponent(uuidstr, FCTN, UACN) != 0) {
		acnlogmark(lgERR, "Init local component failed");
		return;
	}

	/* set up for ephemeral port and any interface */
	netx_INIT_ADDR_ANY(&listenaddr, port);

	/* start up ACN */
	if (!dmp_register(&listenaddr)) return;

	/* now we can advertise ourselves */
	acnlogmark(lgDBUG, "starting SLP");
	slp_start_sa(netx_PORT(&listenaddr), DCID_str, interfaces);


	evl_wait();

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

	run_device(uuidstr, port, interfaces);

	return 0;
}
