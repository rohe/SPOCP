
/***************************************************************************
                          dbm.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <stdlib.h>
#include <stdarg.h>
#include <gdbm.h>
#include <string.h>

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <rvapi.h>

befunc          gdbm_test;
int             P_gdbm_close(void *vp);

/*
 * ===================================================== 
 */

/*
 * first argument is filename * second argument is the key and if there are
 * any the third and following arguments are permissible values
 * 
 * file:key:value 
 */

extern gdbm_error gdbm_errno;

int
P_gdbm_close(void *vp)
{
	gdbm_close((GDBM_FILE) vp);

	return 1;
}

spocp_result_t
gdbm_test(cmd_param_t * cmdp, octet_t * blob)
{
	datum           key, res;
	GDBM_FILE       dbf = 0;
	spocp_result_t  r = SPOCP_DENIED;
	int             i, cv = 0;
	octet_t        *oct, *o, cb;
	octarr_t       *argv;
	char           *str = 0;
	becon_t        *bc = 0;

	if (cmdp->arg == 0 || cmdp->arg->len == 0)
		return SPOCP_MISSING_ARG;

	if ((oct = element_atom_sub(cmdp->arg, cmdp->x)) == 0)
		return SPOCP_SYNTAXERROR;

	if (cmdp->pd)
		cv = cached(cmdp->pd->cv, oct, &cb);

	if (cv) {
		traceLog(LOG_DEBUG,"ipnum: cache hit");

		if (cv == EXPIRED) {
			cached_rm(cmdp->pd->cv, oct);
			cv = 0;
		}
	}

	if (cv == 0) {

		argv = oct_split(oct, ':', '\\', 0, 0);

		o = argv->arr[0];

		if (cmdp->pd == 0 || (bc = becon_get(o, cmdp->pd->bcp)) == 0) {
			str = oct2strdup(oct, 0);
			dbf = gdbm_open(str, 512, GDBM_READER, 0, 0);
			free(str);

			if (!dbf)
				r = SPOCP_UNAVAILABLE;
			else if (cmdp->pd && cmdp->pd->size) {
				if (!cmdp->pd->bcp)
					cmdp->pd->bcp =
					    becpool_new(cmdp->pd->size);
				bc = becon_push(oct, &P_gdbm_close,
						(void *) dbf, cmdp->pd->bcp);
			}
		} else
			dbf = (GDBM_FILE) bc->con;

		if (r == SPOCP_DENIED) {

			o = argv->arr[1];

			/*
			 * what happens if someone has removed the GDBM file
			 * under our feets ?? 
			 */

			switch (argv->n) {
			case 0:	/* check if the file is there and
					 * possible to open */
				r = SPOCP_SUCCESS;
				break;

			case 1:	/* Is the key in the file */
				key.dptr = oct->val;
				key.dsize = oct->len;

				r = gdbm_exists(dbf, key);
				break;

			default:	/* have to check both key and value */
				key.dptr = oct->val;
				key.dsize = oct->len;

				res = gdbm_fetch(dbf, key);

				if (res.dptr != NULL) {
					for (i = 2; i < argv->n; i++) {
						if (memcmp
						    (res.dptr,
						     argv->arr[i]->val,
						     argv->arr[i]->len) == 0) {
							r = SPOCP_SUCCESS;
							break;
						}
					}

					free(res.dptr);
				}
				break;
			}
		}

		octarr_free(argv);

		if (bc)
			becon_return(bc);
		else if (dbf)
			gdbm_close(dbf);

	}

	if (cv == (CACHED | SPOCP_SUCCESS)) {
		if (cb.len)
			octln(blob, &cb);
		r = SPOCP_SUCCESS;
	} else if (cv == (CACHED | SPOCP_DENIED)) {
		r = SPOCP_DENIED;
	} else {
		if (cmdp->pd && cmdp->pd->ct &&
		    (r == SPOCP_SUCCESS || r == SPOCP_DENIED)) {
			time_t          t;
			t = cachetime_set(oct, cmdp->pd->ct);
			cmdp->pd->cv =
			    cache_value(cmdp->pd->cv, oct, t, (r | CACHED), 0);
		}
	}

	if (oct != cmdp->arg)
		oct_free(oct);

	return r;
}

plugin_t        gdbm_module = {
	SPOCP20_PLUGIN_STUFF,
	gdbm_test,
	NULL,
	NULL,
	NULL
};
