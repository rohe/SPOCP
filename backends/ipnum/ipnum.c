
/***************************************************************************
                          ipnum.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <rvapi.h>
#include <wrappers.h>
#include <log.h>

/*
 * ============================================================== 
 */

befunc          ipnum_test;

/*
 * needed if you want to use traceLog from the backend 
 */
extern FILE    *spocp_logf;

int             ipnumcmp(octet_t * ipnum, char *ipser);
spocp_result_t  ipnum_syntax(octet_t * arg);

/*
 * ============================================================== 
 */

/*
 * first argument is filename second argument is the key and if there is any
 * the third key is a ipnumber 
 */

/*
 * The format of the file should be
 * 
 * comment = '#' whatever
 * line = keyword ':' value *( ',' value )
 * value = ipnumber "/" netpart
 * 
 * for instance
 * 
 * staff:130.239.0.0/16,193.193.7.13
 * 
 */

int
ipnumcmp(octet_t * ipnum, char *ipser)
{
	char           *np, h1[16], h2[19];
	int             netpart;
	struct in_addr  ia1, ia2;

	oct2strcpy(ipnum, h1, 16, 0);
	if (inet_aton(h1, &ia1) == 0)
		return -1;

	if( strlcpy(h2, ipser, 18) >= 18)
		return -1;
	
	if ((np = index(h2, '/')))
		*np++ = 0;

	if (inet_aton(h2, &ia2) == 0)
		return -1;

	/*
	 * converts it to network byte order before comparing 
	 */
	ia1.s_addr = htonl(ia1.s_addr);
	ia2.s_addr = htonl(ia2.s_addr);

	if (np) {
		netpart = atoi(np);
		if (netpart > 31 || netpart < 0)
			return 1;

		ia1.s_addr >>= netpart;
		ia2.s_addr >>= netpart;
	}

	if (ia1.s_addr == ia2.s_addr)
		return 0;
	else
		return 1;
}

/*
 * returns evaluation 
 */

spocp_result_t
ipnum_test(cmd_param_t * cpp, octet_t * blob)
{
	spocp_result_t  r = SPOCP_DENIED;

	octet_t        *oct = 0, cb, *o;
	octarr_t       *argv = 0;
	char            line[256];
	char           *cp, **arr, *file = 0, *sp;
	FILE           *fp = 0;
	int             j, ne, cv = 0;
	becon_t        *bc = 0;
	pdyn_t         *dyn = cpp->pd;

	if (cpp->arg == 0 || cpp->arg->len == 0)
		return SPOCP_MISSING_ARG;

	if ((oct = element_atom_sub(cpp->arg, cpp->x)) == 0)
		return SPOCP_SYNTAXERROR;

	if (dyn)
		cv = cached(dyn->cv, oct, &cb);

	if (cv) {
		traceLog(LOG_DEBUG,"ipnum: cache hit");

		if (cv == EXPIRED) {
			cached_rm(dyn->cv, oct);
			cv = 0;
		}
	}

	if (cv == 0) {

		sp = oct2strdup(oct, 0);
		traceLog(LOG_DEBUG,"ipnum[expanded arg]: \"%s\"", sp);
		free(sp);

		argv = oct_split(oct, ':', '\\', 0, 0);

		o = argv->arr[0];

		if (dyn == 0 || (bc = becon_get(o, dyn->bcp)) == 0) {
			file = oct2strdup(o, 0);
			fp = fopen(file, "r");
			free(file);
			if (fp == 0)
				r = SPOCP_UNAVAILABLE;
			else if (dyn && dyn->size) {
				if (!dyn->bcp)
					dyn->bcp = becpool_new(dyn->size);
				bc = becon_push(o, &P_fclose, (void *) fp,
						dyn->bcp);
			}
		} else {
			fp = (FILE *) bc->con;
			if (fseek(fp, 0L, SEEK_SET) == -1) {
				/*
				 * try to reopen 
				 */
				file = oct2strdup(o, 0);
				fp = fopen(file, "r");
				free(file);
				if (fp == 0) {	/* not available */
					becon_rm(dyn->bcp, bc);
					bc = 0;
					r = SPOCP_UNAVAILABLE;
				} else
					becon_update(bc, (void *) fp);
			}
			traceLog(LOG_DEBUG,"Using connection from the pool");
		}

		if (argv->n >= 2 && r == SPOCP_DENIED) {

			while (r == SPOCP_DENIED && fgets(line, 256, fp)) {
				if (*line == '#')
					continue;
				if ((cp = index(line, ':')) == 0)
					continue;

				rmcrlf(cp);

				*cp++ = 0;

				if (oct2strcmp(argv->arr[1], line) == 0) {
					traceLog(LOG_DEBUG,"Matched key");
					if( *cp == '\0' ) {
						r = SPOCP_SUCCESS;
						break;
					}
					else if (argv->n == 2 || (argv->n == 3 &&
                            *(argv->arr[2]->val) == '\0' )) {
						r = SPOCP_SUCCESS;
						break;
					} else {
						arr = line_split(cp, ',', '\\', 0, 0, &ne);

						for (j = 0; j <= ne; j++) {
							sp = rmlt(arr[j], 0);
							if (ipnumcmp(argv->arr[2], sp) == 0)
								break;
						}

						if (j <= ne)
							r = SPOCP_SUCCESS;

						charmatrix_free(arr);
					}
				}
			}
		}

		if (bc)
			becon_return(bc);
		else if (fp)
			fclose(fp);

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

plugin_t        ipnum_module = {
	SPOCP20_PLUGIN_STUFF,
	ipnum_test,
	NULL,
	NULL,
	NULL
};
