#include <config.h>
#include <stdlib.h>

#include <spocp.h>
#include <struct.h>
#include <func.h>
#include <wrappers.h>

static octnode_t *octnode_new( void )
{
  return (octnode_t *) Calloc( 1, sizeof( octnode_t )) ;
}

octnode_t *octnode_add( octnode_t *on, octet_t *oct )
{
  octnode_t *non ;

  if( oct == 0 || oct->len == 0 ) return on ;

  if( on == 0 ) non = on = octnode_new() ;
  else {
    for( non = on ; non->next ; non =  non->next ) ;
    non->next = octnode_new() ;
    non = non->next ;
  }

  non->oct.len = oct->len ;
  non->oct.val = oct->val ;
  non->oct.size = oct->size ;

  return on ;
}

void octnode_free( octnode_t *on )
{
  if( on ) {
    if( on->next ) octnode_free( on->next ) ;

    if( on->oct.size ) free( on->oct.val ) ;
    free( on ) ;
  }
}

octnode_t *octnode_join( octnode_t *a, octnode_t *b)
{
  octnode_t *o ;

  if( a && b ) {
    for( o = a ; o->next ; o = o->next ) ;
    o->next = b ;
  
    return a ;
  }
  else if( a ) return a ;
  else return b ;
}

int octnode_cmp( octnode_t *a, octnode_t *b )
{
  if( a && b ) return octcmp( &a->oct, &b->oct ) ; 
  else if( a ) return 1 ;
  else         return -1 ;
}

