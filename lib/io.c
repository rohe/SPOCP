
/***************************************************************************
                          io.c  -  description
                             -------------------

    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include <func.h>
#include <spocp.h>

size_t
Writen(int fd, size_t n, char *str)
{
	size_t          nleft, nwritten;
	char           *sp;
	fd_set          wset;
	int             retval;
	struct timeval  to;

	/*
	 * if( spocp_loglevel == SPOCP_DEBUG ) gettimeofday( &start, NULL ) ; 
	 */

	/*
	 * wait max 3 seconds 
	 */
	to.tv_sec = 3;
	to.tv_usec = 0;

	sp = str;
	nleft = n;
	FD_ZERO(&wset);

	while (nleft > 0) {
		FD_SET(fd, &wset);
		retval = select(fd + 1, NULL, &wset, NULL, &to);

		if (retval) {
			if ((nwritten = write(fd, sp, nleft)) <= 0) {;
				if (errno == EINTR)
					nwritten = 0;
				else
					return -1;
			}

			nleft -= nwritten;
			sp += nwritten;
		} else {	/* timed out */
			break;
		}
	}

#ifdef HAVE_FDATASYNC
	fdatasync(fd);
#else
#ifdef HAVE_FSYNC
	fsync(fd);
#endif
#endif

	/*
	 * if( spocp_loglevel == SPOCP_DEBUG ) { gettimeofday( &stop, NULL ) ;
	 * printElapsed( "WRITE", start, stop ) ; } 
	 */

	return (n - nleft);
}

/*
 * read returns ssize_t which normally is an int 
 */

ssize_t
Readn(int fd, size_t max, char *str)
{
	fd_set          rset;
	int             retval;
	ssize_t         n = 0;
	struct timeval  to;

	/*
	 * wait max 2 seconds 
	 */
	to.tv_sec = 1;
	to.tv_usec = 0;

	FD_ZERO(&rset);

	FD_SET(fd, &rset);
	retval = select(fd + 1, &rset, NULL, NULL, &to);

	if (retval) {
		if ((n = read(fd, str, max)) <= 0) {
			if (errno == EINTR)
				n = 0;
			else
				return -1;
		}
	} else {		/* timed out */
		/*
		 * traceLog(LOG_INFO, "Timed out waiting for intput") ; 
		 */
	}

	return (n);
}
