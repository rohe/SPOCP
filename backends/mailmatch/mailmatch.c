
/***************************************************************************
                          addrmatch.c  -  description
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

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <rvapi.h>

befunc          addrmatch_test;

/*
 * The format of arg is:
 * 
 * <filename>:<mailaddress> 
 */

/*
 * The format of the file must be
 * 
 * comment = '#' whatever CR
 * line = addr-spec / xdomain CR
 * xdomain = ( "." / "@" ) domain
 * 
 * addr-spec and domain as defined by RFC 822 
 */

spocp_result_t
addrmatch_test(cmd_param_t * cpp, octet_t * blob)
{
	spocp_result_t  r = SPOCP_DENIED;

	char            line[512];
	char           *filename = 0;
	FILE           *fp = 0;
	int             len, n, cv = 0;
	octarr_t       *argv;
	octet_t        *oct, *o, cb;
	becon_t        *bc = 0;
	pdyn_t         *dyn = cpp->pd;

	if (cpp->arg == 0 || cpp->arg->len == 0)
		return SPOCP_MISSING_ARG;

	if ((oct = element_atom_sub(cpp->arg, cpp->x)) == 0)
		return SPOCP_SYNTAXERROR;

	if (dyn)
		cv = cached(dyn->cv, oct, &cb);

	if (cv) {
		traceLog(LOG_DEBUG,"mailmatch: cache hit");

		if (cv == EXPIRED) {
			cached_rm(dyn->cv, oct);
			cv = 0;
		}
	}

	if (cv == 0) {

		argv = oct_split(oct, ':', '\\', 0, 0);

		o = argv->arr[0];

		if (dyn == 0 || (bc = becon_get(o, dyn->bcp)) == 0) {
			filename = oct2strdup(o, 0);
			fp = fopen(filename, "r");
			free(filename);

			if (fp == 0) {
				r = SPOCP_UNAVAILABLE;
			} else if (dyn && dyn->size) {
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
				filename = oct2strdup(o, 0);
				fp = fopen(filename, "r");
				free(filename);
				if (fp == 0) {	/* not available */
					becon_rm(dyn->bcp, bc);
					bc = 0;
					r = SPOCP_UNAVAILABLE;
				} else
					becon_update(bc, (void *) fp);
			}
		}

		if (r == SPOCP_DENIED && argv->n > 1) {
			o = argv->arr[1];

			while (r == SPOCP_DENIED && fgets(line, 512, fp)) {
				if (*line == '#')
					continue;

				rmcrlf(line);
				len = strlen(line);

				if ((n = o->len - len) <= 0)
					continue;

				if (strncasecmp(&o->val[n], line, len) == 0)
					r = SPOCP_SUCCESS;
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

plugin_t        mailmatch_module = {
	SPOCP20_PLUGIN_STUFF,
	addrmatch_test,
	NULL,
	NULL
};
