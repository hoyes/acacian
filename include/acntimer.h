/**********************************************************************/
/*

Copyright (c) 2010, Engineering Arts (UK)

All rights reserved.

  $Id$

#tabs=3s
*/
/**********************************************************************/

#ifndef __acntimer_h__
#define __acntimer_h__ 1

#include <assert.h>
#include "acnlists.h"

/************************************************************************/
typedef struct acnTimer_s acnTimer_t;

typedef void timeout_fn(struct acnTimer_s *timer);

#if CONFIG_TIMEFORMAT == TIME_ms
typedef int32_t acn_time_t;
#elif CONFIG_TIMEFORMAT == TIME_POSIX_timeval
#include <sys/time.h>
typedef struct timeval acn_time_t;
#elif CONFIG_TIMEFORMAT == TIME_POSIX_timespec
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

#if CONFIG_TIMEFORMAT == TIME_ms

typedef int32_t acn_time_t;

#define timerval_ms(Tms) (Tms)
#define timerval_s(Ts) ((Ts) * 1000)
#define time_in_ms(t) (t)
#define time_in_s(t) ((t) / 1000)

#define set_timer(timer, timeout) _set_timer(timer, timeout)

#define schedule_action(timerp, act, time) (\
	(timerp)->action = (act), _set_timer(timerp, time))

#define inTimeOrder(A, B) (((B) - (A)) > 0)
#define timediff(a, b) ((a) - (b))
#define ACN_NO_TIME -1

#if ACN_POSIX
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
#endif

#elif CONFIG_TIMEFORMAT == TIME_POSIX_timeval

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

#define timediff(a, b) ???
#define ACN_NO_TIME ???

static inline acn_time_t
get_acn_time()
{
   struct timeval tvnow;
   int rslt;

   rslt = gettimeofday(&tvnow, NULL);
   assert(rslt == 0);
   return tvnow;
}

#elif CONFIG_TIMEFORMAT == TIME_POSIX_timespec

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

#define timediff(a, b) ???
#define ACN_NO_TIME ???

static inline acn_time_t
get_acn_time()
{
   struct timespec tvnow;
   int rslt;

   rslt = clock_gettime(CLOCK_MONOTONIC, &tvnow);
   assert(rslt == 0);
   return tvnow;
}

#endif

extern int init_timers(void);
//extern void stop_all_timers(void);
extern void _set_timer(acnTimer_t *timer, acn_time_t timeout);
extern void cancel_timer(acnTimer_t *timer);
#define is_active(timerp) ((timerp)->lnk.l != NULL)
#define inittimer(timerp) ((timerp)->lnk.l = (timerp)->lnk.r = NULL)
acn_time_t processtimers(void);

#endif  /* __acntimer_h__ */
