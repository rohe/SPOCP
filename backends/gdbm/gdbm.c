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

spocp_result_t gdbm_test( octet_t *arg, becpool_t *bcp, octet_t *blob ) ;

/* ===================================================== */

/* first argument is filename *
   second argument is the key
   and if there are any the
   third and following arguments
   are permissible values 

   file:key:value
*/

extern gdbm_error gdbm_errno ;

int P_gdbm_close( void *vp )
{
  gdbm_close( (GDBM_FILE) vp ) ;

  return 1 ;
}

spocp_result_t gdbm_test( octet_t *arg, becpool_t *bcp, octet_t *blob )
{
  datum          key, res ;
  GDBM_FILE      dbf ;
  spocp_result_t r = SPOCP_DENIED ;
  int            i, n ;
  octet_t      **arr ;
  char          *str = 0 ;
  becon_t       *bc = 0 ;

  if( arg == 0 || arg->len == 0 ) return SPOCP_MISSING_ARG ;

  arr = oct_split( arg, ':', '\\', 0, 0, &n ) ;

  str = oct2strdup( arr[0], '%' ) ;

  if( bcp == 0 || ( bc = becon_get( "dbm", str, bcp )) == 0 ) { 
    dbf = gdbm_open( str, 512, GDBM_READER, 0, 0) ;

    if( !dbf ) {
      r = SPOCP_UNAVAILABLE ;
      goto done ;
    }

    if( bcp != 0 ) bc = becon_push( "dbm", str, &P_gdbm_close, (void *) dbf, bcp ) ;
  }
  else { 
    dbf = ( GDBM_FILE ) bc->con ;
  }

  /* what happens if someone has removed the GDBM file under our feets ?? */

  switch( n ) {
    case 0: /* check if the file is there and possible to open */
      r = SPOCP_SUCCESS ;
      break ;

    case 1: /* Is the key in the file */
      key.dptr = arr[1]->val ;
      key.dsize = arr[1]->len ;

      r =  gdbm_exists( dbf, key ) ;
      break ;

    default: /* have to check both key and value */
      key.dptr = arr[1]->val ;
      key.dsize = arr[1]->len ;

      res = gdbm_fetch( dbf, key);

      if( res.dptr != NULL ) {
        for( i = 2 ; i < n ; i++ ) {
          if( memcmp( res.dptr, arr[i]->val, arr[i]->len ) == 0 ) {
            r = SPOCP_SUCCESS ;
            break ;
          }
        }

        free( res.dptr ) ;
      }
      break ;
  }

done:
  oct_freearr( arr ) ;
  if( str) free( str ) ;
  if( bc ) becon_return( bc ) ;

  return r ;
}

