/***************************************************************************
                          verify.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <syslog.h>

#include <octet.h>
#include <spocp.h>
#include <verify.h>
#include <proto.h>
#include <wrappers.h>
#include <log.h>

/* #define AVLUS 1 */

/*
 * date-fullyear = 4DIGIT 
 * date-month = 2DIGIT ; 01-12 
 * date-mday = 2DIGIT ; 01-28, 01-29, 01-30, 01-31 based on ; month/year 
 * time-hour = 2DIGIT ; 00-23
 * time-minute = 2DIGIT ; 00-59 
 * time-second = 2DIGIT ; 00-58, 00-59, 00-60 based on leap second ; 
 * rules 
 * time-secfrac = "." 1*DIGIT 
 * time-numoffset = ("+" / "-") time-hour ":" time-minute 
 * time-offset = "Z" / time-numoffset
 * 
 * partial-time = time-hour ":" time-minute ":" time-second [time-secfrac]
 * full-date = date-fullyear "-" date-month "-" date-mday 
 * full-time = partial-time time-offset
 * 
 * date-time = full-date "T" full-time
 * 
 * 2002-12-31T23:59:59Z 2002-12-31T23:59:59+01:30 
 */

spocp_result_t
is_date(octet_t * op)
{
	char           *c = op->val;
	int             i, ns = 0, corlen;
    
	if (op->len < 20) {
#ifdef AVLUS
        printf("Too short\n");
#endif
		return SPOCP_SYNTAXERROR;
    }
    
	/*
	 * year 
	 */
	for (i = 0; i < 4; i++, c++)
		if (*c < '0' || *c > '9') {
#ifdef AVLUS
            printf("Wrong character: %c\n", *c);
#endif
			return SPOCP_SYNTAXERROR;
        }
    
    
	if (*c++ != '-') {
#ifdef AVLUS
        printf("Wrong character: %c\n", *c);
#endif
		return SPOCP_SYNTAXERROR;
    }
    
	/*
	 * month 01-12 
	 */
	if ((*c == '0' && (*(c + 1) >= '1' && *(c + 1) <= '9')) ||
	    (*c == '1' && (*(c + 1) >= '0' && *(c + 1) <= '2')));
	else {
#ifdef AVLUS
        printf("Wrong month spec: %c %c\n", *c, *(c+1));
#endif
		return SPOCP_SYNTAXERROR;
    }
    
	c += 2;
	if (*c++ != '-') {
#ifdef AVLUS
        printf("Wrong character: %c\n", *c);
#endif
		return SPOCP_SYNTAXERROR;
    }
    
	/*
	 * day 01-31 
	 */
	if (((*c == '0') && (*(c + 1) >= '1' && *(c + 1) <= '9')) ||
	    ((*c == '1' || *c == '2') && (*(c + 1) >= '0' && *(c + 1) <= '9'))
	    || (*c == '3' && (*(c + 1) == '0' || *(c + 1) == '1')));
	else {
#ifdef AVLUS
        printf("Wrong day spec: %c %c\n", *c, *(c+1));
#endif
		return SPOCP_SYNTAXERROR;
    }
    
	c += 2;
	if (*c++ != 'T') {
#ifdef AVLUS
        printf("Wrong character: %c should have been 'T'\n", *c);
#endif
		return SPOCP_SYNTAXERROR;
    }
    
	/*
	 * hour 00-24 
	 */
	if (((*c == '0' || *c == '1') && (*(c + 1) >= '0' && *(c + 1) <= '9'))
	    || (*c == '2' && (*(c + 1) >= '0' && *(c + 1) <= '4')));
	else {
#ifdef AVLUS
        printf("Wrong hour spec: %c %c\n", *c, *(c+1));
#endif
		return SPOCP_SYNTAXERROR;
    }
    
	c += 2;
	if (*c++ != ':') {
#ifdef AVLUS
        printf("Wrong character: %c should have been ':'\n", *c);
#endif
		return SPOCP_SYNTAXERROR;
    }
    
	/*
	 * minut 00-59 
	 */
	if (((*c >= '0' && *c <= '5')
	     && (*(c + 1) >= '0' && *(c + 1) <= '9')));
	else {
#ifdef AVLUS
        printf("Wrong minut spec: %c %c\n", *c, *(c+1));
#endif
		return SPOCP_SYNTAXERROR;
    }
    
	c += 2;
	if (*c++ != ':')
		return SPOCP_SYNTAXERROR;
    
	/*
	 * Second 00-59 
	 */
	if (((*c >= '0' && *c <= '5')
	     && (*(c + 1) >= '0' && *(c + 1) <= '9')));
	else
		return SPOCP_SYNTAXERROR;
    
    c += 2;
    
    if (*c == '.') {
        c++;
        ns++;
        /* should be a sequence of digits here */
        if (*c < '0' || *c > '9') {
            return SPOCP_SYNTAXERROR;
        }
        while (*c >= '0' && *c <= '9') {
            c++;
            ns++;
        }
    }    
    
    corlen = op->len - ns ;
    
	if (*c == 'Z' && corlen == 20)
		return SPOCP_SUCCESS;
    
	if (*c != '+' && *c != '-') {
#ifdef AVLUS
        printf("Wrong character: %c should have been '+'/'-'\n", *c);
#endif
		return SPOCP_SYNTAXERROR;
    }
    
	if (corlen < 22) {
#ifdef AVLUS
        printf("Too short %d\n", (int) corlen);
#endif
		return SPOCP_SYNTAXERROR;
    }
    
	c++;
    
	/*
	 * hour 00-24 
	 */
	if (((*c == '0' || *c == '1') && (*(c + 1) >= '0' && *(c + 1) <= '9'))
	    || (*c == '2' && (*(c + 1) >= '0' && *(c + 1) <= '4')));
	else {
#ifdef AVLUS
        printf("Wrong timezone hour spec: '%c%c'\n", *c, *(c+1));
#endif
		return SPOCP_SYNTAXERROR;
    }
    
	if (corlen == 22)
		return SPOCP_SUCCESS;
	if (corlen != 25) {
#ifdef AVLUS
        printf("Too short %d (expected 25 chars)\n", (int) op->len);
#endif
		return SPOCP_SYNTAXERROR;
    }
    
	c += 2;
	if (*c != ':') {
#ifdef AVLUS
        printf("Wrong character: %c should have been ':'\n", *c);
#endif
        return SPOCP_SYNTAXERROR;
    }
    
    c++;
	/*
	 * minut 00-59 
	 */
	if (((*c >= '0' && *c <= '5')
	     && (*(c + 1) >= '0' && *(c + 1) <= '9')));
	else {
#ifdef AVLUS
        printf("Wrong timezone minut spec: '%c%c'\n", *c, *(c+1));
#endif
		return SPOCP_SYNTAXERROR;
    }
    
	return SPOCP_SUCCESS;
}

spocp_result_t
is_time(octet_t * op)
{
	char           *c;
    
	if (op == 0)
		return SPOCP_SYNTAXERROR;
    
	if (op->len != 8)
		return SPOCP_SYNTAXERROR;
    
	c = op->val;
    
	/*
	 * hour 00-24 
	 */
	if (((*c == '0' || *c == '1') && (*(c + 1) >= '0' && *(c + 1) <= '9'))
	    || (*c == '2' && (*(c + 1) >= '0' && *(c + 1) <= '4')));
	else
		return SPOCP_SYNTAXERROR;
    
	c += 2;
	if (*c++ != ':')
		return SPOCP_SYNTAXERROR;
    
	/*
	 * minut 00-59 
	 */
	if (((*c >= '0' && *c <= '5')
	     && (*(c + 1) >= '0' && *(c + 1) <= '9')));
	else
		return SPOCP_SYNTAXERROR;
    
	c += 2;
	if (*c++ != ':')
		return SPOCP_SYNTAXERROR;
    
	/*
	 * Second 00-59 
	 */
	if (((*c >= '0' && *c <= '5')
	     && (*(c + 1) >= '0' && *(c + 1) <= '9')));
	else
		return SPOCP_SYNTAXERROR;
    
	return SPOCP_SUCCESS;
}

spocp_result_t
is_numeric(octet_t *op, long *num)
{
	char	*sp;
	size_t	l;
	long    n;

	if (op->val == 0 || *(op->val) == 0)
		return SPOCP_SYNTAXERROR;

	for ( n = 0L, l = 0, sp = op->val ; l < op->len && DIGIT(*sp) ; l++,sp++) {
		if (n)
			n *= 10;

		n += *sp -'0' ;
	}

	if (n == LONG_MIN || n == LONG_MAX)
		return SPOCP_SYNTAXERROR;

	if (l == op->len) {
		*num = n;
		return SPOCP_SUCCESS;
	} else
		return SPOCP_SYNTAXERROR;
}

spocp_result_t
numstr(char *str, long *l)
{
	octet_t tmp;

	if (str == 0 || *str == 0)
		return SPOCP_SYNTAXERROR;

	oct_assign( &tmp, str);

	return is_numeric( &tmp, l);
}

spocp_result_t
is_ipv4(octet_t * op, struct in_addr * ia)
{
	char    *str;

    str = oct2strdup(op, 0);

#ifdef HAVE_INET_NTOP
	if (inet_pton(AF_INET, str, ia) <= 0) {
#else
	if (inet_aton(str, ia) == 0) {
#endif
        Free(str);
#ifdef AVLUS
		traceLog(LOG_ERR,"Error in IP number definition: %s", strerror(errno));
#endif
		return SPOCP_SYNTAXERROR;
	} else {
        Free(str);
		ia->s_addr = htonl( ia->s_addr );
		return SPOCP_SUCCESS;
	}
}

spocp_result_t
is_ipv4_s(char *ip, struct in_addr * ia)
{
#ifdef HAVE_INET_NTOP
	if (inet_pton(AF_INET, ip, ia) <= 0)
#else
	if (inet_aton( ip, ia) == 0) 
#endif
		return SPOCP_SYNTAXERROR;
	else
		return SPOCP_SUCCESS;
}

#ifdef USE_IPV6
spocp_result_t
is_ipv6(octet_t * op, struct in6_addr * ia)
{
	char            c;

	c = op->val[op->len];
	op->val[op->len] = 0;
	if (inet_pton(AF_INET6, op->val, ia) <= 0) {
		op->val[op->len] = c;
		traceLog(LOG_ERR,"Error in IP number definition: %s", strerror(errno));
		return SPOCP_SYNTAXERROR;
	} else {
		op->val[op->len] = c;
		return SPOCP_SUCCESS;
	}
}

spocp_result_t
is_ipv6_s(char *ip, struct in6_addr * ia)
{
	if (inet_pton(AF_INET6, ip, ia) <= 0)
		return SPOCP_SYNTAXERROR;
	else
		return SPOCP_SUCCESS;
}
#endif

