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
file: evloop.c

Simple event loop and timer inplementation
*/

/**********************************************************************/
/*
Logging level for this source file.
If not set it will default to the global CF_LOG_DEFAULT

options are

lgOFF lgEMRG lgALRT lgCRIT lgERR lgWARN lgNTCE lgINFO lgDBUG
*/
//#define LOGLEVEL lgDBUG

/**********************************************************************/
#include "acn.h"
#include <assert.h>

/* A pointer to the head of our timer list */
struct acnTimer_s *timerqueue;
/* file descriptor for epoll */
int evl_pollfd;
/* loop control */
int runstate = rs_loop;

/**********************************************************************/
int evl_init(void)
{
	static bool initialized = 0;

	LOG_FSTART();
	if (initialized) {
		acnlogmark(lgDBUG, "already initialized");
		return 0;
	}

	timerqueue = NULL;
	runstate = rs_loop;

	if ((evl_pollfd = epoll_create1(0)) < 0) {
		acnlogerror(lgERR);
		return -1;
	}

	/* don't process twice */
	initialized = 1;
	LOG_FEND();
	return 0;
}

/************************************************************************/
/*
	evl_wait() - wait for events

*/
#define MAXEVENTS 20

static acn_time_t processtimers(void);
void evl_wait(void)
{
	struct epoll_event eva[MAXEVENTS];
	int nfds;
	acn_time_t to;
	int i;

	LOG_FSTART();

	do {

		to = processtimers();

		if ((nfds = epoll_wait(evl_pollfd, eva, MAXEVENTS, time_in_ms(to))) < 0) {
			acnlogerror(lgERR);
		} else for (i = 0; i < nfds; ++i) {
			poll_fn **pfn;
		
			pfn = (poll_fn **)eva[i].data.ptr;
			(**pfn)(eva[i].events, pfn);
		}
	} while (runstate == rs_loop);
	LOG_FEND();
}

/**********************************************************************/
/*
	Call with timeout
*/
void
_set_timer(struct acnTimer_s *timer, acn_time_t timeout)
{
	struct acnTimer_s *tp;
	acn_time_t now;

	if (timer->lnk.r) dlUnlink(timerqueue, timer, lnk);   /* unlink if it was already in queue */

	now = get_acn_time();

	/* add timeout to now to get expiry */
	timer->exptime = now + timeout;

	tp = timerqueue;
	if (tp && inTimeOrder(tp->exptime, timer->exptime)) {
		/* Not at head so just add it to the queue */
		/* start at tail and work back as more probable that it goes there */
		do {
			tp = tp->lnk.l;
		} while (inTimeOrder(timer->exptime, tp->exptime));
		dlInsertR(tp, timer, lnk);
	} else {
		/* goes straight to head of queue */
		dlAddHead(timerqueue, timer, lnk);
	}
	return;
}

/**********************************************************************/
void
cancel_timer(struct acnTimer_s *timer)
{
	assert(timer != NULL);
	if (timer->lnk.r == NULL) return;   /* may have gone off already */

	dlUnlink(timerqueue, timer, lnk);

	timer->lnk.r = timer->lnk.l = NULL;    /* mark as unused */
	return;
}

/**********************************************************************/
static acn_time_t
processtimers(void)
{
	struct acnTimer_s *tp;
	acn_time_t now;

	if (timerqueue != NULL) {

		now = get_acn_time();
		/* process and un-queue expired timerqueue */
		while ((tp = timerqueue) != NULL && !inTimeOrder(now, tp->exptime)) {
			dlUnlink(timerqueue, tp, lnk);
			tp->lnk.r = tp->lnk.l = NULL;   /* mark it as unlinked */
			if (tp->action) (*tp->action)(tp);
		}
		if (tp) return timediff(tp->exptime, now);
	}
	return ACN_NO_TIME;
}
