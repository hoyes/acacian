/************************************************************************/
/*
Copyright (c) 2010, Engineering Arts (UK)

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

 * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
 * Neither the name of Engineering Arts nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  $Id$

#tabs=3s
*/
/************************************************************************/

#define _XOPEN_SOURCE 600
#include <signal.h>
#include <assert.h>

#include "acncommon.h"
#include "acntimer.h"
#include "acnlog.h"

/************************************************************************/
#define lgFCTY LOG_SDT
/************************************************************************/
/*
Prototypes
*/
void alarmtimer(int signum);

/************************************************************************/
/*
Global and static data
*/
#if defined(__GNUC__) && (__GNUC__ == 2)
#undef sa_handler
#define init_sa_handler_static(val) .__sigaction_handler = {.sa_handler = val}
#else
#define init_sa_handler_static(val) .sa_handler = val
#endif

/* A pointer to the head of our timer list */
struct acnTimer_s *timerqueue;

#if 0
/* Action to catch alarm signal */
static struct sigaction act = {
   init_sa_handler_static(&alarmtimer)
};

/* Action to ignore alarm signal */
static struct sigaction sigoff = {
   init_sa_handler_static(SIG_IGN)
};

/* Structure to stop our timer */
const struct itimerval stoptime = {{0, 0}, {0, 0}};
#define stoptimer() setitimer(ITIMER_REAL, &stoptime, NULL)
#endif
/************************************************************************/
/*
   Initialize the timerqueue
*/
int init_timers(void)
{
   static bool initialized;
//   int i;

   if (initialized) {
      acnlogmark(LOG_INFO | LOG_TIMER, "already initialized");
      return 0;
   }

//   i = stoptimer();
//   assert(i == 0);
   timerqueue = NULL;

   /* setup alarm action */
//   return sigaction(SIGALRM, &act, NULL);
   return 0;
}

/************************************************************************/
/*
   Stop all the timerqueue
*/
#if 0
void
stop_all_timers(void)
{
   int i;
   struct acnTimer_s *tp;
   
   i = stoptimer();
   assert(i == 0);
   i = sigaction(SIGALRM, &sigoff, NULL);
   assert(i == 0);

   while (timerqueue) {
      tp = timerqueue->lnk.l;
      dlUnlink(timerqueue, tp, lnk);
      tp->lnk.l = tp->lnk.r = NULL;
   }
}
#endif
/************************************************************************/
#if 0
static struct timeval now;
static struct itimerval itime = { .it_interval = {0, 0}};

void
restartTimers(void)
{
   int i;

   if ((itime.it_value.tv_sec = timerqueue->exptime.tv_sec - now.tv_sec) < 0
      || ((itime.it_value.tv_usec = timerqueue->exptime.tv_usec - now.tv_usec) < 0
         && (itime.it_value.tv_usec += 1000000, --itime.it_value.tv_sec) < 0)
   ) {
      /* missed timeout! set it for very soon */
      itime.it_value.tv_sec = 0;
      itime.it_value.tv_usec = 1;
   }

   i = setitimer(ITIMER_REAL, &itime, NULL);
   assert(i == 0);
   return;
}
#endif

/************************************************************************/
/*
   Call with timeout
*/
void
_set_timer(struct acnTimer_s *timer, acn_time_t timeout)
{
   struct acnTimer_s *tp;
//   int i;
   acn_time_t now;


/*
   i = stoptimer();
   assert(i == 0);

   acnlogmark(lgDBUG, "set timer + %ld-%ld", timeout.tv_sec, timeout.tv_usec);
*/

   if (timer->lnk.r) dlUnlink(timerqueue, timer, lnk);   /* unlink if it was already in queue */

   now = get_acn_time();


   /* add timeout to now to get expiry */
/*
   timer->exptime.tv_sec = now.tv_sec + timeout.tv_sec;
   if ((timer->exptime.tv_usec = now.tv_usec + timeout.tv_usec) >= 1000000) {
      timer->exptime.tv_usec -= 1000000;
      ++timer->exptime.tv_sec;
   }
*/
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
//   restartTimers();
   return;
}

/************************************************************************/
void
cancel_timer(struct acnTimer_s *timer)
{
//   bool reschedule;
//   int i;

   assert(timer != NULL);
   if (timer->lnk.r == NULL) return;   /* may have gone off already */

//   reschedule = (timerqueue == timer);

   dlUnlink(timerqueue, timer, lnk);

/*
   if (reschedule) {
      if (timerqueue == NULL) {
         i = stoptimer();
         assert(i == 0);
      } else {
         gettimeofday(&now, NULL);
         restartTimers();
      }
   }
*/

   timer->lnk.r = timer->lnk.l = NULL;    /* mark as unused */
   return;
}

/************************************************************************/
acn_time_t
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
/************************************************************************/

#if 0
void alarmtimer(int signum UNUSED)
{
   int i;
   struct acnTimer_s *tp;

//   UNUSED_ARG(signum)
   if (timerqueue == NULL) return;   /* spurious */

   i = gettimeofday(&now, NULL);
   assert(i == 0);

   /* process and un-queue expired timerqueue */
   while ((tp = timerqueue) != NULL) {
      if (inTimeOrder(now, tp->exptime)) {
         restartTimers();
         return;
      }
      dlUnlink(timerqueue, tp, lnk);
      tp->lnk.r = tp->lnk.l = NULL;   /* mark it as unlinked */
      if (tp->action) (*tp->action)(tp);
   }
   /* Only get here if all timed out */
   i = stoptimer();
   assert(i == 0);
}
#endif
