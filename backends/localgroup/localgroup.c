
/***************************************************************************
                          be_localgroup.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Stockholm university, Sweden
    email                : roland@it.su.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

/*
 * localgroup:leifj:foo:[/etc/group] 
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <grp.h>
#include <sys/types.h>

#include <be.h>
#include <plugin.h>
#include <rvapi.h>
#include "../../server/srvconf.h"

#define ETC_GROUP "/etc/group"

befunc          localgroup_test;


static int
_ismember(octet_t * member, char *grp)
{
	char          **p;
	int             i, n, r = FALSE;

	if (grp == 0 || *grp == 0)
		return FALSE;

	p = line_split(grp, ',', 0, 0, 0, &n);

	for (i = 0; r == FALSE && i <= n; i++) {
		if (oct2strcmp(member, p[i]) == 0)
			r = TRUE;
	}

	charmatrix_free(p);

	return r;
}

/*
 * format of group files 
 */
/*
 * /etc/group is an ASCII file which defines the groups to which users belong. 
 * There is one entry per line, and each line has the format:
 * 
 * group_name:passwd:GID:user_list
 * 
 * The query arg should look like this group_name:user[:file]
 * 
 */

spocp_result_t
localgroup_test(cmd_param_t * cpp, octet_t * blob)
{
	spocp_result_t  rc = SPOCP_DENIED;

	FILE           *fp;
	octarr_t       *argv;
	octet_t        *oct, cb, *group, *user, loc;
	char           *fn = ETC_GROUP, *cp;
	int             done, cv = 0;
	char            buf[BUFSIZ];
	becon_t        *bc = 0;
	pdyn_t         *dyn = cpp->pd;

	if (cpp->arg == 0)
		return SPOCP_MISSING_ARG;

	if ((oct = element_atom_sub(cpp->arg, cpp->x)) == 0)
		return SPOCP_SYNTAXERROR;

	if (dyn) 
		cv = cached(dyn->cv, oct, &cb);

	if (cv) {
		/*
		 * traceLog(LOG_DEBUG, "localgroup: cache hit" ) ; 
		 */

		if (cv == EXPIRED && dyn) {
			cached_rm(dyn->cv, oct);
			cv = 0;
		}
	}

	if (cv == 0) {
		argv = oct_split(oct, ':', 0, 0, 0);

		/*
		 * LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG,"n=%d",n); 
		 */

		if (argv->n == 3)
			fn = oct2strdup(argv->arr[2], 0);

		/*
		 * LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG,"groupfile: %s",fn); 
		 */

		oct_assign(&loc, fn);

		if ( dyn == 0 || (bc = becon_get(&loc, dyn->bcp)) == 0) {
			if ((fp = fopen(fn, "r")) == 0)
				rc = SPOCP_UNAVAILABLE;
			else if (dyn && dyn->size) {
				if (!dyn->bcp)
					dyn->bcp = becpool_new(dyn->size);
				bc = becon_push(&loc, &P_fclose, (void *) fp,
						dyn->bcp);
			}
		} else {
			fp = (FILE *) bc->con;
			if (fseek(fp, 0L, SEEK_SET) == -1) {
				/*
				 * try to reopen 
				 */
				fp = fopen(fn, "r");
				if (fp == 0) {	/* not available */
					if (dyn)
						becon_rm(dyn->bcp, bc);
					bc = 0;
					rc = SPOCP_UNAVAILABLE;
				} else
					becon_update(bc, (void *) fp);
			}
		}

		/*
		 * LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG,"open...%s",fn); 
		 */

		if (rc == SPOCP_DENIED || argv->n < 2) {
			done = 0;
			group = argv->arr[0];
			user = argv->arr[1];

			while (!done && fgets(buf, BUFSIZ, fp)) {
				rmcrlf(buf);
				if ((cp = index(buf, ':')) == 0)
					continue;	/* incorrect line */

				*cp++ = '\0';

				/*
				 * LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG,"grp: %s", buf
				 * ); 
				 */

				if ((oct2strcmp(group, buf)) == 0) {
					/*
					 * skip passwd 
					 */
					if ((cp = index(cp, ':')) == 0)
						continue;	/* incorrect
								 * line */
					cp++;
					/*
					 * skip GID 
					 */
					if ((cp = index(cp, ':')) == 0)
						continue;	/* incorrect
								 * line */
					cp++;

					if (_ismember(user, cp) == TRUE)
						rc = SPOCP_SUCCESS;
					done++;
				}

			}
		}

		if (bc)
			becon_return(bc);
		else if (fp)
			fclose(fp);

		if (fn != ETC_GROUP)
			free(fn);

		octarr_free(argv);
	}

	if (cv == (CACHED | SPOCP_SUCCESS)) {
		if (cb.len)
			octln(blob, &cb);
		rc = SPOCP_SUCCESS;
	} else if (cv == (CACHED | SPOCP_DENIED)) {
		rc = SPOCP_DENIED;
	} else {
		if (dyn && dyn->ct && (rc == SPOCP_SUCCESS || rc == SPOCP_DENIED)) {
			time_t          t;
			t = cachetime_set(oct, dyn->ct);
			dyn->cv =
			    cache_value(dyn->cv, oct, t, (rc | CACHED), 0);
		}
	}

	if (oct != cpp->arg)
		oct_free(oct);

	return rc;
}

plugin_t        localgroup_module = {
	SPOCP20_PLUGIN_STUFF,
	localgroup_test,
	NULL,
	NULL,
	NULL
};
