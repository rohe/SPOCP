#include <string.h>

#include <plugin.h>
#include <func.h>
#include <wrappers.h>

time_t cachetime_set( octet_t *str, plugin_t *pi )
{
  cachetime_t *ct, *xct = 0 ;
  int          l, max = -1 ;
  
  for( ct = pi->ct ; ct ; ct = ct->next ) {
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

cachetime_t *cachetime_new( octet_t *s )
{
  cachetime_t *new ;
  long int     li ;
  int          n ;
  octet_t     **arg ;

  arg = oct_split( s, ' ', '\0', 0, 0, &n ) ;

  if( is_numeric( arg[0], &li ) != SPOCP_SUCCESS ) {
    oct_freearr( arg ) ;
    return NULL ;
  }

  new = ( cachetime_t * ) Malloc ( sizeof( cachetime_t )) ;
  new->limit = ( time_t ) li ;
  
  if( n > 0 ) {
    new->pattern.val = arg[1]->val ;
    new->pattern.len = arg[1]->len ;
    new->pattern.size = arg[1]->size ;
    arg[1]->size = 0 ; /* to protect from being freed by oct_freearr */
  }
  else memset( &new->pattern, 0, sizeof( octet_t )) ;

  oct_freearr( arg ) ;

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
