
/***************************************************************************
                          be_regexp.c  -  description
                             -------------------
    begin                : Wed Sep 10 2003
    copyright            : (C) 2003 by Karolinska Institutet, Sweden
    email                : ola.gustafsson@itc.ki.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdarg.h>
#include <string.h>
#include <regex.h>

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <rvapi.h>

befunc          regexp_test;
/*
 * type = "regexp" typespecific = flags ";" regexp ";" string flags =
 * utf8string ;at the moment only "i" is an recognized option. regexp =
 * utf8string ;Posix extended regular expression string = utf8string
 * 
 * returns true if the regexp matches the string. 
 */

spocp_result_t
regexp_test(cmd_param_t * cpp, octet_t * blob)
{
	octarr_t       *argv;
	octet_t        *oct;
	char           *flags = 0;
	char           *regexp;
	char           *string;
	int             icase = FALSE;
	int             the_flags;
	regex_t        *preg;
	int             i;
	int             answ;

	if (cpp->arg == 0 || cpp->arg->len == 0)
		return SPOCP_MISSING_ARG;

	if ((oct = element_atom_sub(cpp->arg, cpp->x)) == 0)
		return SPOCP_SYNTAXERROR;

	/*
	 * Split into returnval and cmd parts 
	 */
	argv = oct_split(oct, ';', 0, 0, 0);

	if (oct != cpp->arg)
		oct_free(oct);

	if (argv->n < 3)
		return SPOCP_MISSING_ARG;

	if (argv->arr[0]->len)
		flags = oct2strdup(argv->arr[0], 0);
	regexp = oct2strdup(argv->arr[1], 0);
	string = oct2strdup(argv->arr[2], 0);

	DEBUG( SPOCP_DBCOND ){
		traceLog(LOG_DEBUG,"Regexp: regexp: \"%s\" string: \"%s\"",
		    regexp,string); 
	}
	
	DEBUG( SPOCP_DBCOND ){ traceLog(LOG_DEBUG,"Flags len: %d",argv->arr[0]->len); } 

	if (argv->arr[0]->len > 0) {
		for (i = 0; flags[i]; i++) {
			switch (flags[i]) {
			case 'i':
			case 'I':
				icase = TRUE;
				break;
			default:
				break;
			}
		}
	}


	the_flags = REG_EXTENDED | REG_NOSUB;
	if (icase == TRUE) {
		the_flags |= REG_ICASE;
	}


	preg = (regex_t *) calloc(1, sizeof(regex_t));
	DEBUG( SPOCP_DBCOND ){ traceLog(LOG_DEBUG,"regcomp with flags %d", the_flags); } 
	answ = regcomp(preg, regexp, the_flags);
	DEBUG( SPOCP_DBCOND ){ traceLog(LOG_DEBUG,"Have compiled expression"); } 

	if (answ != 0) {
		DEBUG( SPOCP_DBCOND ){
		    	traceLog(LOG_DEBUG,
			    "Error when trying to compile regular expression: %d",
			    answ);
		} 
		regfree(preg);
		free(flags);
		free(regexp);
		free(string);
		octarr_free(argv);
		return FALSE;
	}


	DEBUG( SPOCP_DBCOND ){ traceLog(LOG_DEBUG,"Will execute expression"); } 
	answ = regexec(preg, string, (size_t) 0, NULL, 0);
	DEBUG( SPOCP_DBCOND ){
		traceLog(LOG_DEBUG,
		    "Have executed expression, with answer %d",answ); 
	} 

	regfree(preg);
	free(flags);
	free(regexp);
	free(string);
	octarr_free(argv);

	if (answ != 0) {
		DEBUG( SPOCP_DBCOND )
		    traceLog(LOG_DEBUG,"Error, or no match found: %d",answ);
		return FALSE;
	} else {
		return TRUE;
	}
}

plugin_t        regexp_module = {
	SPOCP20_PLUGIN_STUFF,
	regexp_test,
	NULL,
	NULL
};
