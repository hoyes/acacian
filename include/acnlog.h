/*--------------------------------------------------------------------*/
/*

Copyright (c) 2008, Electronic Theatre Controls, Inc

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

 * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
 * Neither the name of Electronic Theatre Controls nor the names of its
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

This file forms part of Acacian a full featured implementation of 
ANSI E1.17 Architecture for Control Networks (ACN)

#tabs=3s
*/
/**********************************************************************/

#ifndef __acnlog_h__
#define __acnlog_h__ 1

/**********************************************************************/
/*
file: acnlog.h

Macros for logging and debug:

These macros loosely follow syslog syntax but the loglevel is tested 
at compile time and outputs compile to nothing if  the loglevel is 
not high enough.

The calls can be configured to send output to syslog, stdout, stderr or
nothing.

See <Logging> for more detail.
*/
#define ACNLOG_OFF   0
#define ACNLOG_SYSLOG 1
#define ACNLOG_STDOUT 2
#define ACNLOG_STDERR 3

#define LOG_OFF INT_MAX

/*
macros: short version facility and level macros

lgEMRG - (lgFCTY | LOG_EMERG)
lgALRT - (lgFCTY | LOG_ALERT)
lgCRIT - (lgFCTY | LOG_CRIT)
lgERR  - (lgFCTY | LOG_ERR)
lgWARN - (lgFCTY | LOG_WARNING)
lgNTCE - (lgFCTY | LOG_NOTICE)
lgINFO - (lgFCTY | LOG_INFO)
lgDBUG - (lgFCTY | LOG_DEBUG)

lgFCTY must be defined (usually at top of source file)
before using these. e.g. from sdt.c
> #define lgFCTY LOG_SDT

*/

#define lgOFF LOG_OFF

#define lgEMRG (LOG_FACILITY | LOG_EMERG)
#define lgALRT (LOG_FACILITY | LOG_ALERT)
#define lgCRIT (LOG_FACILITY | LOG_CRIT)
#define lgERR  (LOG_FACILITY | LOG_ERR)
#define lgWARN (LOG_FACILITY | LOG_WARNING)
#define lgNTCE (LOG_FACILITY | LOG_NOTICE)
#define lgINFO (LOG_FACILITY | LOG_INFO)
#define lgDBUG (LOG_FACILITY | LOG_DEBUG)

#ifndef LOGLEVEL
#define LOGLEVEL CF_LOG_DEFAULT
#endif

#if CF_ACNLOG == ACNLOG_SYSLOG
/*
macros: Loglevels

If using syslog, use the system's own definitions. Otherwise they are 
defined here as copied directly from FreeBSD

LOG_EMERG   - system is unusable
LOG_ALERT   - action must be taken immediately
LOG_CRIT    - critical conditions
LOG_ERR     - error conditions
LOG_WARNING - warning conditions
LOG_NOTICE  - normal but significant condition
LOG_INFO    - informational
LOG_DEBUG   - debug-level messages
*/
#include <syslog.h>

#define LOG_FACILITY LOG_USER

#else /* CF_ACNLOG == ACNLOG_SYSLOG */

#define LOG_EMERG       0       /* system is unusable */
#define LOG_ALERT       1       /* action must be taken immediately */
#define LOG_CRIT        2       /* critical conditions */
#define LOG_ERR         3       /* error conditions */
#define LOG_WARNING     4       /* warning conditions */
#define LOG_NOTICE      5       /* normal but significant condition */
#define LOG_INFO        6       /* informational */
#define LOG_DEBUG       7       /* debug-level messages */

#define LOG_FACILITY 0

#endif

/*
macros: Log output functions

All log functions are defined as macros which depend on CF_ACNLOG

acnopenlog(), acncloselog() - call at start and end of program
acntestlog()  - use to test whether a particular loglevel would generate
                code which is useful where logging requires aditional
                code or variable definitions.
acnlog()      - generate an arbitrary message (depending on priority)
acnlogmark()  - generate a "function, file, line" message before the log message
acnlogerror() - roughly equivalent to perror() library function
LOG_FSTART()  - mark the entry to a function
LOG_FEND()    - mark the exit from a fnction
*/

#define __LOGERRFORMAT__ "%20.20s %5d: " 

#define _endstr(litstr, nchars) (sizeof(litstr) <= (nchars) ? \
               (litstr) : \
               (litstr) + sizeof(litstr) - 1 - (nchars))
#define __FILE20__ _endstr(__FILE__, 20)

#if CF_ACNLOG == ACNLOG_SYSLOG
/*
Syslog is POSIX defined - try and stay compliant
*/
#define acnopenlog(ident, option, facility) openlog(ident, option, facility)
#define acncloselog() closelog()
#define acntestlog(priority) ((priority) >= 0)
#define acnlog(priority, ...) if acntestlog(priority) syslog(priority, __VA_ARGS__)

#ifndef __GNUC__
/* standard C is a bit awkward - need to call syslog twice */
#define acnlogmark(priority, ...) if acntestlog(priority) { \
      syslog(priority, __LOGERRFORMAT__, __FILE20__, __LINE__); \
      syslog(priority, __VA_ARGS__);}
#else
/* gnu extension: ## __VA_ARGS__ makes it much neater */
#define acnlogmark(priority, format, ...) if acntestlog(priority) \
         syslog(priority, __LOGERRFORMAT__ format, __FILE20__, __LINE__, ## __VA_ARGS__)
#endif

/*
macros for deep debugging - log entry and exit to each function
*/
#if CF_LOG_FUNCS != LOG_OFF
#define LOG_FSTART() if ((CF_LOG_FUNCS) <= LOGLEVEL) \
            syslog((CF_LOG_FUNCS), "+ %s\n", __func__)
#define LOG_FEND() if ((CF_LOG_FUNCS) <= LOGLEVEL) \
            syslog((CF_LOG_FUNCS), "- %s\n", __func__)
#endif  /* CF_LOG_FUNCS */

#elif CF_ACNLOG == ACNLOG_STDOUT || CF_ACNLOG == ACNLOG_STDERR

#include <stdio.h>
#if CF_ACNLOG == ACNLOG_STDOUT
#define STDLOG stdout
#else
#define STDLOG stderr
#endif

#define acnopenlog(ident, option, facility)
#define acncloselog()

#define acntestlog(priority) ((priority) <= LOGLEVEL)
#define acnlog(priority, ...) \
	if (acntestlog(priority)) do {fprintf(STDLOG, __VA_ARGS__); putc('\n', STDLOG);} while (0)

#ifndef __GNUC__
/* standard C is a bit awkward - need to call fprintf twice, then putc */
#define acnlogmark(priority, ...) if acntestlog(priority) do { \
      fprintf(STDLOG, __LOGERRFORMAT__, __FILE20__, __LINE__); \
      fprintf(STDLOG, __VA_ARGS__); putc('\n', STDLOG);} while (0)
#else
/* gnu extension: ## __VA_ARGS__ makes it much neater */
#define acnlogmark(priority, format, ...) if acntestlog(priority) \
         fprintf(STDLOG, __LOGERRFORMAT__ format "\n", __FILE20__, __LINE__, ## __VA_ARGS__)
#endif

/*
macros for deep debugging - log entry and exit to each function
*/
#if CF_LOG_FUNCS != LOG_OFF
#define LOG_FSTART() if ((CF_LOG_FUNCS) <= LOGLEVEL) \
            fprintf(STDLOG, "+ %s\n", __func__)
#define LOG_FEND() if ((CF_LOG_FUNCS) <= LOGLEVEL) \
            fprintf(STDLOG, "- %s\n", __func__)
#endif  /* CF_LOG_FUNCS */

#else /* CF_ACNLOG == ACNLOG_OFF */

#define acntestlog(priority) (0)
#define acnopenlog(ident, option, facility)
#define acncloselog()
#define acnlog(priority, ...)
#define acnlogmark(priority, ...)
#endif /* CF_ACNLOG == ACNLOG_OFF */

#if CF_LOG_FUNCS == lgOFF || CF_ACNLOG == ACNLOG_OFF
#define LOG_FSTART()
#define LOG_FEND()
#endif

#define acnlogerror(priority) acnlogmark(priority, "%s", strerror(errno))

#endif
