
/***************************************************************************
                          difftime.c  -  description
                             -------------------
    begin                : Thur Jul 10 2003
    copyright            : (C) 2003 by Stockholm University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#define _XOPEN_SOURCE 500	/* glibc2 needs this */
/*
 * #define _POSIX_C_SOURCE 200112 
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <rvapi.h>

#define MAXPARTS 4

/*
 * ======================================================== 
 */

befunc          difftime_test;

/*
 * ======================================================== 
 */

/*
 * format of time definition diff = YYMMDD_HH:MM:SS;YYMMDDTHH:MM:SS; ('+' /
 * '-' ) ('+' / '-' )
 * 
 * 000007_00:00:00;2003-09-21T08:00:00;++
 * 
 * 7 days
 * 
 * 1 year = 365 days 1 month = 30 days 1 day = 86400 seconds
 * 
 * The cases: pt = present time (+) gt = given time (o) delta = time
 * difference
 * 
 * [--] pt < gt && pt < gt - delta + ¤<---- o
 * 
 * [-+] pt < gt && pt > gt - delta ¤<-+-------o
 * 
 * [+-] pt > gt && pt < gt + delta o-------+----->¤
 * 
 * [++] pt > gt && pt > gt + delta o---->¤ +
 * 
 */

static time_t
diff2seconds(octet_t * arg)
{
	time_t          n, days = 0, sec = 0;
	char           *s = arg->val;

	n = *s++ - '0';
	if (n)
		days += n * 3650;

	n = *s++ - '0';
	if (n)
		days += n * 365;

	n = *s++ - '0';
	if (n)
		days += n * 300;

	n = *s++ - '0';
	if (n)
		days += n * 30;

	n = *s++ - '0';
	if (n)
		days += n * 10;

	n = *s++ - '0';
	if (n)
		days += n;

	s++;

	if (days)
		sec = days * 86400;

	n = *s++ - '0';
	if (n)
		sec += n * 36000;

	n = *s++ - '0';
	if (n)
		sec += n * 3600;

	n = *s++ - '0';
	if (n)
		sec += n * 600;

	n = *s++ - '0';
	if (n)
		sec += n * 60;

	n = *s++ - '0';
	if (n)
		sec += n * 10;

	n = *s - '0';
	if (n)
		sec += n;

	return sec;
}

spocp_result_t
difftime_test(cmd_param_t * cpp, octet_t * blob)
{
	spocp_result_t  res = SPOCP_DENIED;
	time_t          pt, gt, sec = 0;
	struct tm       ttm;
	char            tmp[30], *sp;
	octarr_t       *argv;
	octet_t        *oct;

	if (cpp->arg == 0)
		return SPOCP_MISSING_ARG;

	if ((oct = element_atom_sub(cpp->arg, cpp->x)) == 0)
		return SPOCP_SYNTAXERROR;

	argv = oct_split(oct, ';', 0, 0, 0);

	if (oct != cpp->arg)
		oct_free(oct);

	/*
	 * get the time 
	 */
	time(&pt);

	oct = argv->arr[0];
	if (argv->n < 2 || oct->len != 15)
		return res;

	sec = diff2seconds(oct);

	oct = argv->arr[1];
	if (oct->len > 30)
		return SPOCP_SYNTAXERROR;

	if (oct2strcpy(oct, tmp, 30, 0) < 0) {
		octarr_free(argv);
		return 0;
	}

	strptime(tmp, "%Y-%m-%dT%H:%M:%S", &ttm);

	gt = mktime(&ttm);

	sp = argv->arr[2]->val;
	if (*sp == '+') {
		if (pt > gt) {
			if (*(sp + 1) == '+') {
				if ((pt - gt) > sec)
					res = SPOCP_SUCCESS;
			} else if (*(sp + 1) == '-') {
				if ((pt - gt) < sec)
					res = SPOCP_SUCCESS;
			}
		}
	} else if (*sp == '-') {
		if (pt < gt) {
			if (*(sp + 1) == '+') {
				if ((gt - pt) > sec)
					res = SPOCP_SUCCESS;
			} else if (*(sp + 1) == '-') {
				if ((gt - pt) < sec)
					res = SPOCP_SUCCESS;
			}
		}
	}

	octarr_free(argv);
	return res;
}

plugin_t        difftime_module = {
	SPOCP20_PLUGIN_STUFF,
	difftime_test,
	NULL,
	NULL
};
