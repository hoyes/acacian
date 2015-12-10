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
header: evloop.h

Basic event and timing loop for Acacian
*/

#ifndef __evloop_h__
#define __evloop_h__ 1

#include <assert.h>
#if defined(__linux__) || defined(__linux)
#include <sys/epoll.h>

typedef void poll_fn(uint32_t evf, void *evptr);
extern int evl_pollfd;

/*
Register for events on a file descriptor. Note that the callback is 
a reference to a function pointer, not the function pointer itself. 
This allows the function pointer to be embedded in a structure which 
can then be extracted from the evptr argument passed to the callback.
To de-register call with cb == NULL.
*/
static inline int
evl_register(int fd, poll_fn **cb, uint32_t events)
{
	struct epoll_event evs;
	int i;

	i = (cb != NULL) ? EPOLL_CTL_ADD : EPOLL_CTL_DEL;
	evs.events = events;
	evs.data.ptr = cb;
	return epoll_ctl(evl_pollfd, i, fd, &evs);
}
#endif  /* defined(__linux__) || defined(__linux) */

/**********************************************************************/
enum runstate_e {rs_loop, rs_quit};
extern int runstate;

#define stopPoll() (runstate = rs_quit)
/**********************************************************************/
typedef struct acnTimer_s acnTimer_t;

typedef void timeout_fn(struct acnTimer_s *timer);

#if CF_TIME_ms
typedef int32_t acn_time_t;
#elif CF_TIME_POSIX_timeval
#include <sys/time.h>
typedef struct timeval acn_time_t;
#elif CF_TIME_POSIX_timespec
#include <time.h>
typedef struct timespec acn_time_t;
#endif

struct acnTimer_s {
	/* private fields - do not interfere */
	dlLink(acnTimer_t, lnk);
	acn_time_t  exptime;
	/* public fields - set these before calling */
	timeout_fn  *action;
	void        *userp;
};

#if CF_TIME_ms

#define timerval_ms(Tms) (Tms)
#define timerval_s(Ts) ((Ts) * 1000)
#define time_in_ms(t) (t)
#define time_in_s(t) ((t) / 1000)

#define set_timer(timer, timeout) _set_timer(timer, timeout)

#define schedule_action(timerp, act, time) (\
	(timerp)->action = (act), _set_timer(timerp, time))

#define inTimeOrder(A, B) (((B) - (A)) > 0)
#define timediff(a, b) ((a) - (b))
#define timeadd(a, b) ((a) + (b))
#define timeaddto(a, b) ((a) += (b))
#define ACN_NO_TIME -1

#include <time.h>

static inline acn_time_t
get_acn_time()
{
	struct timespec tvnow;
	int rslt;

	rslt = clock_gettime(CLOCK_MONOTONIC, &tvnow);
	assert(rslt == 0);
	return tvnow.tv_sec * 1000 + tvnow.tv_nsec / 1000000;
}
#elif CF_TIME_POSIX_timeval

#define timerval_ms(Tms) {(Tms) / 1000, ((Tms) % 1000) * 1000}
#define timerval_s(Ts) {(Ts), 0}
#define time_in_ms(t) ((t).tv_sec * 1000 + (t).tv_usec / 1000)
#define time_in_s(t) ((t).tv_sec)

#define set_timer(timer, timeout) {\
	acn_time_t _arg = timeout;\
	_set_timer(timer, _arg);\
}

#define schedule_action(timerp, act, time) {\
	acn_time_t _arg = time;\
	(timerp)->action = (act);\
	_set_timer(timerp, _arg);\
}

#define inTimeOrder(A, B) \
			((((B).tv_sec - (A).tv_sec) != 0 ? \
			(B).tv_sec - (A).tv_sec : \
			(B).tv_usec - (A).tv_usec) > 0)

static inline acn_time_t
timediff(acn_time_t a, acn_time_t b)
{
	acn_time_t rslt;
	rslt.tv_usec = a.tv_usec - b.tv_usec;
	if (rslt.tv_usec >= 0) {
		rslt.tv_sec = a.tv_sec - b.tv_sec;
	} else {
		rslt.tv_usec += 1000000;
		rslt.tv_sec = a.tv_sec - b.tv_sec - 1;
	}
	return rslt;
}

static inline acn_time_t
timeadd(acn_time_t a, acn_time_t b)
{
	acn_time_t rslt;
	rslt.tv_usec = a.tv_usec + b.tv_usec;
	if (rslt.tv_usec < 1000000) {
		rslt.tv_sec = a.tv_sec + b.tv_sec;
	} else {
		rslt.tv_usec -= 1000000;
		rslt.tv_sec = a.tv_sec + b.tv_sec + 1;
	}
	return rslt;
}

#define timeaddto(a, b) { \
		a.tv_usec += b.tv_usec; \
		if (a.tv_usec < 1000000) a.tv_sec += b.tv_sec; \
		else {a.tv_usec -= 1000000; a.tv_sec += b.tv_sec + 1;} \
	}

static const acn_time_t ACN_NO_TIME = {-1,-1};

static inline acn_time_t
get_acn_time()
{
	struct timeval tvnow;
	int rslt;

	rslt = gettimeofday(&tvnow, NULL);
	assert(rslt == 0);
	return tvnow;
}

#elif CF_TIME_POSIX_timespec

#define timerval_ms(Tms) {(Tms) / 1000, ((Tms) % 1000) * 1000000}
#define timerval_s(Ts) {(Ts), 0}
#define time_in_ms(t) ((t).tv_sec * 1000 + (t).tv_nsec / 1000000)
#define time_in_s(t) ((t).tv_sec)

#define set_timer(timer, timeout) {\
	acn_time_t _arg = timeout;\
	_set_timer(timer, _arg);\
}

#define schedule_action(timerp, act, time) {\
	acn_time_t _arg = time;\
	(timerp)->action = (act);\
	_set_timer(timerp, _arg);\
}

#define inTimeOrder(A, B) \
			((((B).tv_sec - (A).tv_sec) != 0 ? \
			(B).tv_sec - (A).tv_sec : \
			(B).tv_nsec - (A).tv_nsec) > 0)

static inline acn_time_t
timediff(acn_time_t a, acn_time_t b)
{
	acn_time_t rslt;
	rslt.tv_nsec = a.tv_nsec - b.tv_nsec;
	if (rslt.tv_nsec >= 0) {
		rslt.tv_sec = a.tv_sec - b.tv_sec;
	} else {
		rslt.tv_nsec += 1000000000;
		rslt.tv_sec = a.tv_sec - b.tv_sec - 1;
	}
	return rslt;
}

static inline acn_time_t
timeadd(acn_time_t a, acn_time_t b)
{
	acn_time_t rslt;
	rslt.tv_nsec = a.tv_nsec + b.tv_nsec;
	if (rslt.tv_nsec < 1000000000) {
		rslt.tv_sec = a.tv_sec + b.tv_sec;
	} else {
		rslt.tv_nsec -= 1000000000;
		rslt.tv_sec = a.tv_sec + b.tv_sec + 1;
	}
	return rslt;
}

#define timeaddto(a, b) { \
		a.tv_nsec += b.tv_nsec; \
		if (a.tv_nsec < 1000000000) a.tv_sec += b.tv_sec; \
		else {a.tv_nsec -= 1000000000; a.tv_sec += b.tv_sec + 1;} \
	}

static const acn_time_t ACN_NO_TIME = {-1,-1};

static inline acn_time_t
get_acn_time()
{
	struct timespec tvnow;
	int rslt;

	rslt = clock_gettime(CLOCK_MONOTONIC, &tvnow);
	assert(rslt == 0);
	return tvnow;
}

#endif /* CF_TIME_POSIX_timespec */

extern int evl_init(void);
extern void evl_wait(void);
extern int init_timers(void);
extern void _set_timer(acnTimer_t *timer, acn_time_t timeout);
extern void cancel_timer(acnTimer_t *timer);
#define is_active(timerp) ((timerp)->lnk.l != NULL)
#define inittimer(timerp) ((timerp)->lnk.l = (timerp)->lnk.r = NULL)

#endif  /* __evloop_h__ */
