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
#include <sys/wait.h>
#include <sys/types.h>
#include <stdarg.h>
#include <string.h>
#include <regex.h>

#include <spocp.h>

int regexp_test( octet_t *arg, becpool_t *bcp, octet_t *blob );
/*
  type         = "regexp"
  typespecific = flags ";" regexp ";" string
  flags        = utf8string    ;at the moment only "i" is an recognized option.
  regexp       = utf8string    ;Posix extended regular expression
  string       = utf8string
  
  returns true if the regexp matches the string.
 */

int regexp_test( octet_t *arg, becpool_t *bcp, octet_t *blob ) {
  octet_t  **argv;
  char *flags;
  char *regexp;
  char *string;
  int icase = FALSE;
  int the_flags;
  regex_t *preg;
  int i,n;
  int answ;

  /* Split into returnval and cmd parts */
  argv = oct_split( arg, ';', 0, 0, 0, &n );

  argv[0]->val[argv[0]->len] = 0;
  argv[1]->val[argv[1]->len] = 0;
  argv[2]->val[argv[2]->len] = 0;

  flags = argv[0]->val;
  regexp = argv[1]->val;
  string = argv[2]->val;

/*
  DEBUG( SPOCP_DBCOND ){
    traceLog("Regexp: flags: %s regexp: %s string: \"%s\"",flags,regexp,string);
  }

  DEBUG( SPOCP_DBCOND ){
    traceLog("Flags len: %d",argv[0]->len);
  }
*/

  if(argv[0]->len > 0) {
    for(i = 0;flags[i];i++) {
      switch(flags[i]) {
      case 'i':
      case 'I':
	icase = TRUE;
	break;
      default:
	break;
      }
    }
  }


  the_flags = REG_EXTENDED|REG_NOSUB;
  if(icase == TRUE) {
    the_flags |= REG_ICASE;
  }


  preg = (regex_t *)malloc(1 * sizeof(regex_t));
  answ = regcomp(preg, regexp, the_flags);
/*
  DEBUG( SPOCP_DBCOND ){
    traceLog("Have compiled expression");
  }
*/

  if(answ != 0) {
/*
    DEBUG( SPOCP_DBCOND ){
      traceLog("Error when trying to compile regular expression: %d",answ);
    }
*/
    regfree(preg);
    oct_freearr( argv ) ;
    return FALSE;
  }


  answ = regexec(preg, string,(size_t)0 ,NULL , 0);
/*
  DEBUG( SPOCP_DBCOND ){
    traceLog("Have executed expression, with answer %d",answ);
  }
*/

  regfree(preg);
  oct_freearr( argv ) ;

  if(answ != 0) {
/*
    DEBUG( SPOCP_DBCOND ){
      traceLog("Error, or no match found: %d",answ);
    }
*/
    return FALSE;
  } else {
    return TRUE;
  }  
}


