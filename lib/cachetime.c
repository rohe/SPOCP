#include <string.h>

#include <plugin.h>
#include <spocp.h>
#include <func.h>
#include <wrappers.h>

time_t cachetime_set( octet_t *str, cachetime_t *ct )
{
  cachetime_t  *xct = 0 ;
  int          l, max = -1 ;
  
  for(  ; ct ; ct = ct->next ) {
    if( ct->pattern.size == 0 && max == -1 ) {
      max = 0 ;
      xct = ct ;
    }
    else {
      l = ct->pattern.len ;
      if( l > max && octncmp( str, &ct->pattern, l ) == 0 ) {
        xct = ct ;
        max = l ;
      }
    }
  }

  if( xct ) return xct->limit ;
  else return 0 ;
}

/* expects the format 
 * time [ 1*SP pattern ]
 */
cachetime_t *cachetime_new( octet_t *s )
{
  cachetime_t *new ;
  long int     li ;
  octarr_t    *arg ;

  arg = oct_split( s, ' ', '\0', 0, 0 ) ;

  if( is_numeric( arg->arr[0], &li ) != SPOCP_SUCCESS ) {
    octarr_free( arg ) ;
    return NULL ;
  }

  new = ( cachetime_t * ) Malloc ( sizeof( cachetime_t )) ;
  new->limit = ( time_t ) li ;
  
  if( arg->n > 1 ) octcpy( &new->pattern, arg->arr[1] ) ; 
  else memset( &new->pattern, 0, sizeof( octet_t )) ;

  octarr_free( arg ) ;

  return new ;
}

void cachetime_free( cachetime_t *ct )
{
  if( ct ) {
    if( ct->pattern.size ) free( ct->pattern.val ) ;
    if( ct->next ) cachetime_free( ct->next ) ;
    free( ct ) ;
  }
}
