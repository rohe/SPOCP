
/***************************************************************************
                          be_lastlogin.c  -  description
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <rvapi.h>

/*
 * ============================================================= 
 */

befunc          lastlogin_test;
spocp_result_t  lastlogin_syntax(char *arg);

/*
 * ============================================================= 
 */

/*
 * 
 * typespec = file ";" since ";" userid [ ";" ipnum ]
 * 
 * first argument is filename
 * 
 * second argument is the graceperiod third argument is the key ( user name )
 * and the fourth argument is an ipaddress
 * 
 * The check is whether the ip address is the same as the one used at the last 
 * login. The last login shouldn't be more than half an hour away. 
 */

/*
 * The format of the file is
 * 
 * Jul 25 13:16:42 catalin pop3d: LOGIN, user=aron,
 * ip=[::ffff:213.114.116.145] Jul 25 13:16:44 catalin pop3d: LOGOUT,
 * user=aron, ip=[::ffff:213.114.116.145], top=0, retr=0 Jul 25 19:40:44
 * catalin pop3d: Connection, ip=[::ffff:213.114.116.142] Jul 25 19:40:44
 * catalin pop3d: LOGIN, user=aron, ip=[::ffff:213.114.116.142] Jul 25
 * 19:40:44 catalin pop3d: LOGOUT, user=aron, ip=[::ffff:213.114.116.142],
 * top=0, retr=0 Jul 25 21:53:25 catalin pop3d: Connection,
 * ip=[::ffff:213.67.231.206] Jul 25 21:53:25 catalin pop3d: LOGIN, user=anna, 
 * ip=[::ffff:213.67.231.206] Jul 25 21:53:27 catalin pop3d: LOGOUT,
 * user=anna, ip=[::ffff:213.67.231.206], top=0, retr=6188 Jul 25 22:14:47
 * catalin pop3d: Connection, ip=[::ffff:213.67.231.206] Jul 25 22:14:47
 * catalin pop3d: LOGIN, user=lars, ip=[::ffff:213.67.231.206] Jul 25 22:14:47 
 * catalin pop3d: LOGOUT, user=lars, ip=[::ffff:213.67.231.206], top=0, retr=0
 * 
 */

spocp_result_t
lastlogin_test(cmd_param_t * cpp, octet_t * blob)
{
	spocp_result_t  r = SPOCP_DENIED;
	pdyn_t         *dyn = cpp->pd;
	char            line[256], *str = 0;
	char           *cp, test[512], date[64], *sp;
	FILE           *fp = 0;
	time_t          t, pt;
	struct tm       tm;
	char          **hms;
	int             n, since = 0, cv = 0;
	octarr_t       *argv;
	octet_t        *oct, *o, cb;
	becon_t        *bc = 0;

	if (cpp->arg == 0 || cpp->arg->len == 0)
		return SPOCP_MISSING_ARG;

	if ((oct = element_atom_sub(cpp->arg, cpp->x)) == 0)
		return SPOCP_SYNTAXERROR;

	if (dyn)
		cv = cached(dyn->cv, oct, &cb);

	if (cv) {
		traceLog(LOG_DEBUG,"lastlogin: cache hit");

		if (cv == EXPIRED) {
			cached_rm(dyn->cv, oct);
			cv = 0;
		}
	}

	if (cv == 0) {

		argv = oct_split(oct, ';', '\\', 0, 0);

		if (argv->n == 4) {

			time(&t);

			/*
			 * present year, should be done automagically 
			 */
			strlcpy(date, "2004 ", sizeof(date));

			o = argv->arr[0];

			if (dyn == 0 || (bc = becon_get(o, dyn->bcp)) == 0) {
				str = oct2strdup(o, 0);

				fp = fopen(str, "r");
				free(str);

				if (fp == 0)
					r = SPOCP_UNAVAILABLE;
				else if (dyn && dyn->size) {
					if (!dyn->bcp)
						dyn->bcp =
						    becpool_new(dyn->size);
					bc = becon_push(o, &P_fclose,
							(void *) fp, dyn->bcp);
				}
			} else {
				fp = (FILE *) bc->con;
				if (fseek(fp, 0L, SEEK_SET) == -1) {
					/*
					 * try to reopen 
					 */
					str = oct2strdup(o, 0);
					fp = fopen(str, "r");
					free(str);
					if (fp == 0) {	/* not available */
						becon_rm(dyn->bcp, bc);
						bc = 0;
						r = SPOCP_UNAVAILABLE;
					} else
						becon_update(bc, (void *) fp);
				}
			}

			if (r == SPOCP_DENIED || argv->n < 3) {

				str = oct2strdup(argv->arr[1], 0);
				hms = line_split(str, ':', 0, 0, 0, &n);
				free(str);

				if (hms[2]) {	/* hours, minutes and seconds
						 * are defined */
					since += 3600 * atoi(hms[0]);
					since += 60 * atoi(hms[1]);
					since += atoi(hms[2]);
				} else if (hms[1]) {	/* minutes and seconds 
							 * are defined */
					since += 60 * atoi(hms[0]);
					since += atoi(hms[1]);
				} else {	/* only seconds are defined */
					since += atoi(hms[0]);
				}

				charmatrix_free(hms);

				if (since != 0) {

					str = oct2strdup(argv->arr[2], 0);
					snprintf(test,sizeof(test),
					    "LOGIN, user=%s, ", str);
					free(str);

					if (argv->n == 4)
						str =
						    oct2strdup(argv->arr[3],
							       0);
					else
						str = 0;

					while (fgets(line, 256, fp)) {
						if (strstr(line, test) == 0)
							continue;

						strncpy(date + 5, line, 16);
						date[21] = 0;
						strptime(date,
							 "%Y %b %d %H:%M:%S",
							 &tm);
						pt = mktime(&tm);

						if (t - pt > since)
							continue;

						cp = strstr(line, "ip=[");
						cp += 4;
						if (strncmp(cp, "::ffff:", 7)
						    == 0)
							cp += 7;

						sp = find_balancing(cp, '[',
								    ']');
						*sp = 0;

						if (str == 0
						    || strcmp(cp, str) == 0) {
							r = SPOCP_SUCCESS;
							break;
						}
					}
					if (str)
						free(str);
				}
			}

			if (bc)
				becon_return(bc);
			else if (fp)
				fclose(fp);
		} else
			r = SPOCP_OPERATIONSERROR;

		octarr_free(argv);
	}

	if (cv == (CACHED | SPOCP_SUCCESS)) {
		if (cb.len)
			octln(blob, &cb);
		r = SPOCP_SUCCESS;
	} else if (cv == (CACHED | SPOCP_DENIED)) {
		r = SPOCP_DENIED;
	} else {
		if (dyn && dyn->ct && (r == SPOCP_SUCCESS || r == SPOCP_DENIED)) {
			time_t          t;
			t = cachetime_set(oct, dyn->ct);
			dyn->cv =
			    cache_value(dyn->cv, oct, t, (r | CACHED), 0);
		}
	}

	if (oct != cpp->arg)
		oct_free(oct);

	return r;
}

plugin_t        lastlogin_module = {
	SPOCP20_PLUGIN_STUFF,
	lastlogin_test,
	NULL,
	NULL,
	NULL
};
