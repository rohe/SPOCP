#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#include <spocp.h>

/* 
  argument to the call:

  string0:string1[:offset[:num]]

  if the strings contain part separated by '\t' then we can match on a set of 
  parts.

  If only offset is given then matching us done from that part and until end of set
  If num also is given then matching will stop when that number of parts are matched

  offset can be negative !

  Example:

  dc=se\tdc=umu\tcn=person\tuid=benjoh01\t-:dc=se\tdc=umu\tcn=person\tuid=rolhed01\t-:0:3

  returns TRUE since the tree first parts dc=se, dc=umu and cn=person is equal

  dc=se\tdc=umu\tcn=person\tuid=benjoh01\t-:dc=se\tdc=umu\tcn=person\tuid=rolhed01\t-:-1

  returns FALSE since uid=benjoh01 is not equal to uid=rolhed01

*/

#define CASE       0
#define CASEIGNORE 1

static int _strmatch_test( const octet_t *arg, int type)
{
  char **arr1, **arr2 ;
  char **argv, *tmp ;
  int  i = 0, argc, r = FALSE, n = 0, n1, n2 ;
  long l ;

  /* should I check that there is only printable chars in arg->val ?? */
  tmp = (char *) malloc ( arg->len + 1 ) ;
  strncpy( tmp, arg->val, arg->len ) ;
  tmp[ arg->len ] = 0 ;

  argv = line_split( tmp, ':', 0, 0, 0, &argc ) ;

  free( tmp ) ;

  switch( argc ) {
    case 0 :
      r = FALSE ;
      break ;

    case 1 :
      if( type == CASE && strcmp( argv[0], argv[1] ) == 0 ) r = TRUE ;
      else if( type == CASEIGNORE && strcasecmp( argv[0], argv[1] ) == 0 ) r = TRUE ;
      break ;

    case 2 :
      arr1 = line_split( argv[0], '\t', 0, 0, 0, &n1 ) ;
      arr2 = line_split( argv[1], '\t', 0, 0, 0, &n2 ) ;


      r = TRUE ;
      if( numstr( argv[2], &l ) == FALSE ) r = FALSE ; 
      else {
        if( l < 0 ) {
          l = n1 + 1 + l ;

          if( l < 0 ) return FALSE ;
        }
 
        if( type == CASE ) {
          for( i = (int) l ; r == TRUE && i <= n1 && i <= n2 ; i++ ) {
            if( strcmp( arr1[i], arr2[i] ) != 0 ) 
              r = FALSE ;
          }
        }
        else { /* CASEIGNORE */
          for( i = (int) l ; r == TRUE && i <= n1 && i <= n2 ; i++ ) {
            if( strcasecmp( arr1[i], arr2[i] ) != 0 ) 
              r = FALSE ;
          }
        }
      }
      if( r == TRUE && ( i <= n1 || i <= n2 ) ) r = FALSE ;
      break ;

    case 3 :
      arr1 = line_split( argv[0], '\t', 0, 0, 0, &n1 ) ;
      arr2 = line_split( argv[1], '\t', 0, 0, 0, &n1 ) ;
      r = TRUE ;
      if( numstr( argv[3], &l ) == FALSE ) r = FALSE ; 
      else {

        if( l < 0 ) {
          n = n1 + 1 + (int) l ;
          if( n < 0 ) return FALSE ;
        }
 
        if( numstr( argv[2], &l ) == FALSE ) r = FALSE ; 
        else {
          if( type == CASE ) {
            for( i = (int) l ; n && r == TRUE && i <= n1 && i <= n2 ; i++, n-- ) {
              if( strcmp( arr1[i], arr2[i] ) != 0 ) 
                r = FALSE ;
            }
          }
          else {
            for( i = (int) l ; n && r == TRUE && i <= n1 && i <= n2 ; i++, n-- ) {
              if( strcasecmp( arr1[i], arr2[i] ) != 0 ) 
                r = FALSE ;
            }
          }
        }
      }

      if( r == TRUE && n && ( i <= n1 || i <= n2 ) ) r = FALSE ;
      break ;
  
  }

  return r ;
}

spocp_result_t strmatch_test( octet_t *arg, becpool_t *bcp, octet_t *blob )
{
  if( _strmatch_test( arg, CASE ) == TRUE ) return SPOCP_SUCCESS ;
  else return SPOCP_DENIED ;
}

spocp_result_t strcasematch_test( octet_t *arg, becpool_t *bcp, octet_t *blob )
{
  if(_strmatch_test( arg, CASEIGNORE ) == TRUE ) return SPOCP_SUCCESS ;
  else return SPOCP_DENIED ;
}

