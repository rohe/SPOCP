
/***************************************************************************
                             input.c  

   -  routines to parse canonical S-expression into the incore representation

                             -------------------

    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#ifdef GLIBC2
#define _XOPEN_SOURCE
#endif

#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <time.h>

#include <basic.h>
#include <octet.h>
#include <atom.h>
/*#include <element.h> */

#include <macros.h>
#include <wrappers.h>
#include <proto.h>

/* #define AVLUS 1 */

/*
 * =============================================================== 
 */

int
ipv4cmp(struct in_addr *ia1, struct in_addr *ia2)
{
    return ia1->s_addr - ia2->s_addr;
}

#ifdef USE_IPV6
int
ipv6cmp(struct in6_addr *ia1, struct in6_addr *ia2)
{
    uint8_t     *la = &(ia1->s6_addr[15]) ;
    uint8_t     *lb = &(ia2->s6_addr[15]) ;
    int             r, i;

    for (i = 15; i >= 0; i++, la--, lb--) {
        r = *la - *lb;
        if (r)
            return r;
    }

    return 0;
}
#endif

/*
 * ========================================================================== 
 */

/* A time with offset has "+"/"-" \d\d\d\d as suffix 
 * +0600 
 */

void
to_gmt(octet_t * s, octet_t * t)
{
    char           *sp, time[20];
    struct tm       tm;
    time_t          tid;

    if (s->len == 19 || s->len == 20) {
        octcpy(t, s);
        t->len = 19;
    } else {        /* with offset, will be at least 21 characters long */
        /* Get me the time without offset */
        strncpy(time, s->val, 19);
        time[19] = '\0';
        strptime(time, "%Y-%m-%dT%H:%M:%S", &tm);
        tid = timegm(&tm);

        /* add/delete the offset */
        sp = s->val + 19;
        if (*sp == '+') {
            sp++;
            tid += (*sp++ - '0') * 36000;
            tid += (*sp++ - '0') * 3600;
            if (s->len > 22) {
                sp++;
                tid += (*sp++ - '0') * 600;
                tid += (*sp++ - '0') * 60;
            }
        } else {
            sp++;
            tid -= (*sp++ - '0') * 36000;
            tid -= (*sp++ - '0') * 3600;
            if (s->len > 22) {
                sp++;
                tid -= (*sp++ - '0') * 600;
                tid -= (*sp++ - '0') * 60;
            }
        }

        gmtime_r(&tid, &tm);

        t->val = (char *) Malloc(20 * sizeof(char));
        t->val[19] = 0;

        strftime(t->val, 20, "%Y-%m-%dT%H:%M:%S", &tm);
        t->len = 19;
    }
}

void
hms2int(octet_t * op, long *num)
{
    char           *cp;

    cp = op->val;

    *num = 0;
    *num += (*cp++ - '0') * 36000;
    *num += (*cp++ - '0') * 3600;

    cp++;

    *num += (*cp++ - '0') * 600;
    *num += (*cp++ - '0') * 60;

    cp++;

    *num += (*cp++ - '0') * 10;
    *num += (*cp++ - '0');
}

/*
 * -------------------------------------------------------------------------- 
 */


