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

	$Id: acnlog.h 338 2010-09-04 11:25:10Z philipnye $

#tabs=3s
*/
/*--------------------------------------------------------------------*/

#ifndef __acnlog_h
#define __acnlog_h

#include "acncfg.h"

/************************************************************************/
/*
  ACN specific defines
*/
#if CONFIG_ACNLOG == ACNLOG_SYSLOG
/*
   If using syslog, use the system's own definitions
*/
#include <syslog.h>

#else /* CONFIG_ACNLOG == ACNLOG_SYSLOG */

/*
else use these - copied from FreeBSD
*/
#define LOG_EMERG       0       /* system is unusable */
#define LOG_ALERT       1       /* action must be taken immediately */
#define LOG_CRIT        2       /* critical conditions */
#define LOG_ERR         3       /* error conditions */
#define LOG_WARNING     4       /* warning conditions */
#define LOG_NOTICE      5       /* normal but significant condition */
#define LOG_INFO        6       /* informational */
#define LOG_DEBUG       7       /* debug-level messages */

#endif

/*
All log functions are defined as macros which depend on CONFIG_ACNLOG

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

#if CONFIG_ACNLOG == ACNLOG_SYSLOG
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
#define LOG_FSTART(fclity) if ((fclity) >= 0 && CONFIG_LOGLEVEL == LOG_DEBUG) \
            syslog(LOG_DEBUG | (fclity), "+ %s\n", __func__)
#define LOG_FEND(fclity) if ((fclity) >= 0 && CONFIG_LOGLEVEL == LOG_DEBUG) \
            syslog(LOG_DEBUG | (fclity), "- %s\n", __func__)

#elif CONFIG_ACNLOG == ACNLOG_STDOUT || CONFIG_ACNLOG == ACNLOG_STDERR

#include <stdio.h>
#if CONFIG_ACNLOG == ACNLOG_STDOUT
#define STDLOG stdout
#else
#define STDLOG stderr
#endif

#define acnopenlog(ident, option, facility)
#define acncloselog()

#define acntestlog(priority) ((priority) >= 0 && ((priority) & 7) <= CONFIG_LOGLEVEL)
#define acnlogfile() ((CONFIG_ACNLOG == ACNLOG_STDERR) ? stderr : stdout)
#define acnlog(priority, ...) \
	if (acntestlog(priority)) do {fprintf(STDLOG, __VA_ARGS__); putc('\n', STDLOG);} while (0)

#ifndef __GNUC__
/* standard C is a bit awkward - need to call fprintf twice, then putc */
#define acnlogmark(priority, ...) if acntestlog(priority) { \
      fprintf(STDLOG, __LOGERRFORMAT__, __FILE20__, __LINE__); \
      fprintf(STDLOG, __VA_ARGS__); putc('\n', STDLOG);}
#else
/* gnu extension: ## __VA_ARGS__ makes it much neater */
#define acnlogmark(priority, format, ...) if acntestlog(priority) \
         fprintf(STDLOG, __LOGERRFORMAT__ format "\n", __FILE20__, __LINE__, ## __VA_ARGS__)
#endif

/*
macros for deep debugging - log entry and exit to each function
*/
#define LOG_FSTART(fclity) if ((fclity) >= 0 && CONFIG_LOGLEVEL == LOG_DEBUG) \
            fprintf(STDLOG, "+ %s\n", __func__)
#define LOG_FEND(fclity) if ((fclity) >= 0 && CONFIG_LOGLEVEL == LOG_DEBUG) \
            fprintf(STDLOG, "- %s\n", __func__)

#else /* CONFIG_ACNLOG == ACNLOG_NONE */

#define acntestlog(priority) (0)
#define acnopenlog(ident, option, facility)
#define acncloselog()
#define acnlog(priority, ...)
#define acnlogmark(priority, ...)
#define LOG_FSTART(fclity)
#define LOG_FEND(fclity)
#endif

#define acnlogerror(priority) acnlogmark(priority, "%s", strerror(errno))

/*
short versions - lgFCTY must be defined (usually at top of source file)
before using these:
*/

#define lgEMRG (lgFCTY | LOG_EMERG)
#define lgALRT (lgFCTY | LOG_ALERT)
#define lgCRIT (lgFCTY | LOG_CRIT)
#define lgERR  (lgFCTY | LOG_ERR)
#define lgWARN (lgFCTY | LOG_WARNING)
#define lgNTCE (lgFCTY | LOG_NOTICE)
#define lgINFO (lgFCTY | LOG_INFO)
#define lgDBUG (lgFCTY | LOG_DEBUG)

#endif
