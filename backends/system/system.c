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

int system_test( octet_t *arg, becpool_t *bcp, octet_t *blob );
/*
  type         = "system"
  typespecific = returnval ";" cmd
  returnval    = d *2( d )
  cmd          = utf8string
  
  returns true if returnvalue of cmd is the same as returnval.
 */

int system_test( octet_t *arg, becpool_t *bcp, octet_t *blob ) {
  octet_t  **argv;
  int n;
  char *cmd;
  int return_val;
  int temp_ret = 0;

  /* Split into returnval and cmd parts */
  argv = oct_split( arg, ';', 0, 0, 0, &n );

  argv[0]->val[argv[0]->len] = 0;
  argv[1]->val[argv[1]->len] = 0;

  sscanf(argv[0]->val,"%d",&return_val);
  cmd = argv[1]->val;

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

  oct_freearr( argv ) ;

  if(WEXITSTATUS(temp_ret) == return_val) {
    return TRUE;
  } else {
    return FALSE;
  }
}

