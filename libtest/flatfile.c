/*
 *  flatfile.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 2/10/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <rvapi.h>

befunc          flatfile_test;

extern FILE    *spocp_logf;

/*
 * first argument is filename second argument is the key and if there are any
 * the third and following arguments are permissible values 
 *
 * The format of the file should be
 * 
 * comment = '#' whatever 
 * line = keyword [ ':' value *( ',' value ) ]
 * 
 * for instance
 * 
 * staff:roland,harald,leif
 * 
 */

spocp_result_t
flatfile_test(cmd_param_t * cpp, octet_t * blob)
{
	spocp_result_t  r = SPOCP_SUCCESS;
    
	char            line[256], *str;
	char           *cp, **arr, *sp;
	octarr_t       *argv;
	octet_t        *oct, *o, cb;
	FILE           *fp;
	int             j, i, ne, cv = 0;
	becon_t        *bc = 0;
    
	if (cpp->arg == 0)
		return SPOCP_MISSING_ARG;
    
	if ((oct = element_atom_sub(cpp->arg, cpp->x)) == 0)
		return SPOCP_SYNTAXERROR;
    
	if (cpp->pd)
		cv = cached(cpp->pd->cv, oct, &cb);
    
	if (cv) {
		traceLog(LOG_DEBUG,"ipnum: cache hit");
        
		if (cv == EXPIRED) {
			cached_rm(cpp->pd->cv, oct);
			cv = 0;
		}
	}
    
	if (cv == 0) {
		argv = oct_split(oct, ':', 0, 0, 0);
        
		o = argv->arr[0];
        
		if (cpp->pd == 0 || (bc = becon_get(o, cpp->pd->bcp)) == 0) {
            
			str = oct2strdup(argv->arr[0], 0);
			fp = fopen(str, "r");
			free(str);
            
			if (fp == 0)
				r = SPOCP_UNAVAILABLE;
			else if (cpp->pd && cpp->pd->size) {
				if (!cpp->pd->bcp)
					cpp->pd->bcp =
                    becpool_new(cpp->pd->size);
				bc = becon_push(o, &P_fclose, (void *) fp,
                                cpp->pd->bcp);
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
					becon_rm(cpp->pd->bcp, bc);
					bc = 0;
					r = SPOCP_UNAVAILABLE;
				} else
					becon_update(bc, (void *) fp);
			}
		}
        
		if (argv->n == 1 || r != SPOCP_SUCCESS);
		else {
            
			r = SPOCP_DENIED;
            
			while (r == SPOCP_DENIED && fgets(line, 4096, fp)) {
				if (*line == '#')
					continue;
                
				rmcrlf(line);
                
				if ((cp = index(line, ':'))) {
					*cp++ = 0;
				}
                
				if (oct2strcmp(argv->arr[1], line) == 0) {
					if (argv->n == 2) {
						r = SPOCP_SUCCESS;
						break;
					} else if (cp) {
						arr =
                        line_split(cp, ',', '\\',
							       0, 0, &ne);
                        
						for (j = 0; j < ne; j++) {
							sp = rmlt(arr[j]);
							for (i = 2; i < argv->n; i++) {
								if (oct2strcmp(argv->arr[i], sp) == 0)
									break;
							}
                            
							if (i < argv->n)
								break;
						}
						if (j < ne)
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
		if (cpp->pd && cpp->pd->ct &&
		    (r == SPOCP_SUCCESS || r == SPOCP_DENIED)) {
			time_t          t;
			t = cachetime_set(oct, cpp->pd->ct);
			cpp->pd->cv =
            cache_value(cpp->pd->cv, oct, t, (r | CACHED), 0);
		}
	}
    
	if (oct != cpp->arg)
		oct_free(oct);
    
	return r;
}

plugin_t        flatfile_module = {
	SPOCP20_PLUGIN_STUFF,
	flatfile_test,
	NULL,
	NULL,
	NULL
};
