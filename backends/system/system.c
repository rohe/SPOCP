/***************************************************************************
                          be_system.c  -  description
                             -------------------
    begin                : Wed Sep 10 2003
    copyright            : (C) 2003 by Karolinska Institutet, Sweden
    email                : ola.gustafsson@itc.ki.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <stdio.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <rvapi.h>

befunc system_test;
/*
  type         = "system"
  typespecific = returnval ";" cmd
  returnval    = d *2( d )
  cmd          = utf8string
  
  returns true if returnvalue of cmd is the same as returnval.
 */

spocp_result_t system_test(
  element_t *qp, element_t *rp, element_t *xp, octet_t *arg, pdyn_t *bcp, octet_t *blob )
{
  octarr_t  *argv;
  octet_t   *oct ;
  char *cmd, tmp[64];
  int return_val;
  int temp_ret = 0;

  if( arg == 0 ) return SPOCP_MISSING_ARG ;

  if(( oct = element_atom_sub( arg, xp )) == 0 ) return SPOCP_SYNTAXERROR ;

  /* Split into returnval and cmd parts */
  argv = oct_split( oct, ';', 0, 0, 0 ) ;

  if( oct != arg ) oct_free( oct ) ;

  oct2strcpy( argv->arr[0], tmp, 64, 0, 0 ) ;
  sscanf(tmp,"%d",&return_val);
  cmd = oct2strdup(argv->arr[1], 0 ) ;

/*
  DEBUG( SPOCP_DBCOND ){
    traceLog("System cmd: \"%s\" should have return value %d.",cmd,return_val);
  }
*/

  temp_ret = system(cmd);

/*
  DEBUG( SPOCP_DBCOND ){
    traceLog("Command returned %d. WEXITSTATUS is %d",temp_ret,WEXITSTATUS(temp_ret));
  }
*/

  free( cmd ) ;
  octarr_free( argv ) ;

  if(WEXITSTATUS(temp_ret) == return_val) {
    return TRUE;
  } else {
    return FALSE;
  }
}

