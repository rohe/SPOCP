/*!
 * \file lib/log.c
 * \author Roland Hedberg <roland@catalogix.se
 * \brief Function for handling loggin of different types
 */
/***************************************************************************
                          log.c  -  description
                             -------------------

    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/


#include <config.h>

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif

#include <spocp.h>
#include <dbapi.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

/*! The file descriptor to which the logging should go */
FILE	*spocp_logf = 0;
int	spocp_loglevel = 0;
int	spocp_debug = 0;
int	log_syslog = 0;

/*! The process ID of the server startup process */
int	procid = 0;

#ifdef HAVE_LIBPTHREAD
pthread_mutex_t loglock;
#endif

/* Borrowed from Heimdal */
struct s2i {
    const char *s;
    int val;
};

#define L(X) { #X, LOG_ ## X }

static struct s2i syslogvals[] = {
    L(EMERG),
    L(ALERT),
    L(CRIT),
    L(ERR),
    L(WARNING),
    L(NOTICE),
    L(INFO),
    L(DEBUG),

    L(AUTH),
#ifdef LOG_AUTHPRIV
    L(AUTHPRIV),
#endif
#ifdef LOG_CRON
    L(CRON),
#endif
    L(DAEMON),
#ifdef LOG_FTP
    L(FTP),
#endif
    L(KERN),
    L(LPR),
    L(MAIL),
#ifdef LOG_NEWS
    L(NEWS),
#endif
    L(SYSLOG),
    L(USER),
#ifdef LOG_UUCP
    L(UUCP),
#endif
    L(LOCAL0),
    L(LOCAL1),
    L(LOCAL2),
    L(LOCAL3),
    L(LOCAL4),
    L(LOCAL5),
    L(LOCAL6),
    L(LOCAL7),
    { NULL, -1 }
};

static int
find_value(const char *s, struct s2i *table)
{
    while(table->s && strcasecmp(table->s, s))
        table++;
    return table->val;
}

/*!
 * \brief Opens a file to which logging information is to be written
 * \param file The name of the file to be open 
 * \param level The level at which the logging should be made
 * \return A spocp result code
 */
spocp_result_t
spocp_open_log(char *file, int level)
{
	int	lev;
	char	*cp;

	if (file == 0 || *file == 0 )
		return SPOCP_MISSING_ARG;

	spocp_loglevel = (level & SPOCP_LEVELMASK);
	spocp_debug = level;

	cp = index( file, ':' );
	if (cp == 0) 
		return SPOCP_MISSING_ARG;

	*cp++ = '\0' ;

	if (strcmp(file, "syslog") == 0) {
		log_syslog = 1;
		lev = find_value( cp, syslogvals );
		openlog( "spocp", LOG_NDELAY|LOG_CONS, lev);
		return SPOCP_SUCCESS;
	}
	else if (strcmp( file, "file") == 0 ) {

		if (!procid)
			procid = getpid();

#ifdef HAVE_LIBPTHREAD
		if (spocp_logf == 0)
			pthread_mutex_init(&loglock, NULL);
		pthread_mutex_lock(&loglock);
#endif

		/*
		 * Close the present logfile if there is one 
		 */
		if (spocp_logf != 0 && spocp_logf != stderr)
			fclose(spocp_logf);

		if (cp)
			spocp_logf = fopen(cp, "a");
		else
			spocp_logf = stderr;

#ifdef HAVE_LIBPTHREAD
		pthread_mutex_unlock(&loglock);
#endif

		if (spocp_logf == 0)
			return SPOCP_OPERATIONSERROR;
		else {
			traceLog( LOG_INFO, "Using loglevel %d", spocp_loglevel);
			return SPOCP_SUCCESS;
		}
	}
	else {
		traceLog( LOG_WARNING, "Unknown log type [%s]", file);
		return SPOCP_OPERATIONSERROR;
	}
}

static void
tracelog_doit(int priority, const char *fmt, va_list ap)
{
	char            buf[SPOCP_MAXLINE], date[24], *sp = 0;
	int             r, len;
	time_t          t;
	struct tm       tm;

	memset(buf, 0, SPOCP_MAXLINE);

	time(&t);
	localtime_r(&t, &tm);
	strftime(date, 24, "%Y-%m-%d %H:%M:%S", &tm);

	/*
	 * place the timestamp up front 
	 */
	sprintf(buf, "%s [%d]: ", date, procid);
	len = strlen(buf);
	sp = buf + len;

	r = vsnprintf(sp, SPOCP_MAXLINE - len, fmt, ap);	/* this is
								 * safe */

#ifdef HAVE_LIBPTHREAD
	pthread_mutex_lock(&loglock);
#endif

	if (spocp_logf)
		fprintf(spocp_logf, "%s\n", buf);
	else
		fprintf(stderr, "%s\n", buf);

#ifdef HAVE_LIBPTHREAD
	pthread_mutex_unlock(&loglock);
#endif

	/*
	 * do I care ?? 
	 */
	if (r == -1 || r > SPOCP_MAXLINE - 22) {
		;
	}

	fflush(spocp_logf);

	return;
}

/*!
 * \brief Logs information to a logfile if one is opened otherwise to stderr
 * \param fmt A format specifier
 * \param ... A variable amount of arguments of varying type
 */
void
traceLog(int priority, const char *fmt, ...)
{
	va_list         ap;

	va_start(ap, fmt);

	if (log_syslog)
		vsyslog( LOG_WARNING, fmt, ap);
	else
		tracelog_doit(priority, fmt, ap);

	va_end(ap);

	return;
}

/*!
 * \brief Log a fatal error to the logfile
 * \param msg A message format string 
 * \param s A string 
 * \param i and a integer
 */
void
FatalError(char *msg, char *s, int i)
{
	traceLog(LOG_EMERG, "*** Fatal Error ***");
	traceLog(LOG_EMERG, msg, s, i);
	traceLog(LOG_EMERG, "*** Shutting down ***");
	exit(1);
}

/*!
 * \brief calculates and prints the elapsed time between a start and end timepoint
 * \param s A string to be written before the elapsed time
 * \param start The start time
 * \param end The endpoint
 */
void
print_elapsed(char *s, struct timeval start, struct timeval end)
{

	if (end.tv_usec > start.tv_usec)
		end.tv_usec -= start.tv_usec;
	else {
		end.tv_sec--;
		end.tv_usec += 1000000 - start.tv_usec;
	}
	end.tv_sec -= start.tv_sec;

	traceLog(LOG_INFO, "%s: %.3ld.%.6ld\n", s, end.tv_sec, end.tv_usec);
}

/*!
 * \brief Prints a timestamp in the logfile
 * \param txt Text to write after the time
 */
void
timestamp(char *txt)
{
	struct timeval  tv;

	gettimeofday(&tv, 0);

	traceLog(LOG_INFO, "%s: %ld.%06ld", txt, tv.tv_sec, tv.tv_usec);
}
