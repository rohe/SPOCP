
/***************************************************************************
                          be_time.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#ifdef GLIBC2
#define _XOPEN_SOURCE 500	/* glibc2 needs this */
#endif

/*
 * #define _POSIX_C_SOURCE 200112 
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <spocp.h>
#include <func.h>
#include <be.h>
#include <plugin.h>
#include <rvapi.h>

#define MAXPARTS 4

/*
 * ======================================================== 
 */

befunc          time_test;

/*
 * ======================================================== 
 */

static spocp_result_t
check_time_of_day(octet_t * tod, time_t * pt, int flag)
{
	char           *sp = 0, timestr[30];
	spocp_result_t  res = SPOCP_SUCCESS;

	if (tod->len) {

		sp = ctime_r(pt, timestr);
		sp += 11;

		if (flag) {
			if (oct2strncmp(tod, sp, 8) > 0)
				res = SPOCP_DENIED;
		} else if (oct2strncmp(tod, sp, 8) < 0)
			res = SPOCP_DENIED;
	}

	return res;
}

static spocp_result_t
check_date(octet_t * date, time_t * pt, int flag)
{
	octet_t         gmt;
	struct tm       ttm;
	time_t          t;
	spocp_result_t  res = SPOCP_SUCCESS;

	if (date->len) {
		if (is_date(date) == SPOCP_SUCCESS) {
			to_gmt(date, &gmt);

			strptime(gmt.val, "%Y-%m-%dT%H:%M:%S", &ttm);

			t = mktime(&ttm);

			if (flag) {
				if (*pt > t)
					res = SPOCP_DENIED;
			} else if (*pt < t)
				res = SPOCP_DENIED;

			free(gmt.val);
		} else
			res = SPOCP_DENIED;
	}

	return res;
}

/*
 * ======================================================== 
 */

/*
 * format of time definition starttime;endtime;days;startclock;endclock
 * 
 * 2002-08-01T00:00:00Z;2002-12-31T23:59:59Z;12345;08:00:00;16:59:59
 * 
 * Date format according to RFC3339
 * 
 */

spocp_result_t
time_test(cmd_param_t * cpp, octet_t * blob)
{
	spocp_result_t  res = SPOCP_DENIED;
	char           *sp, *ep;
	time_t          pt;
	struct tm      *ptm;
	octarr_t       *argv;
	octet_t        *oct, *day;

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
	ptm = localtime(&pt);

	if (argv->n > 5) {
		octarr_free(argv);
		return SPOCP_SYNTAXERROR;
	}

	switch (argv->n) {
	case 5:		/* time of day not later than */
		if ((res =
		     check_time_of_day(argv->arr[4], &pt, 1)) != SPOCP_SUCCESS)
			break;

	case 4:		/* time of day not earlier than */
		if ((res =
		     check_time_of_day(argv->arr[3], &pt, 0)) != SPOCP_SUCCESS)
			break;

	case 3:		/* the correct day of the week */
		day = argv->arr[2];
		if (day->len) {
			for (sp = day->val, ep = day->val+day->len; sp != ep && *sp ;
			     sp++)
				if ((*sp - '0') == ptm->tm_wday)
					break;

			if (sp == ep) {
				res = SPOCP_DENIED;
				break;
			}
		}

	case 2:		/* before this date */
		if ((res = check_date(argv->arr[1], &pt, 1)) != SPOCP_SUCCESS)
			break;

	case 1:		/* after this date */
		if ((res = check_date(argv->arr[0], &pt, 0)) != SPOCP_SUCCESS)
			break;
	}

	octarr_free(argv);
	return res;
}

plugin_t        time_module = {
	SPOCP20_PLUGIN_STUFF,
	time_test,
	NULL,
	NULL
};

