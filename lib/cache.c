/***************************************************************************
                          cache.c  -  description
                             -------------------

    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <time.h>
#include <string.h>

#include <struct.h>
#include <db0.h>
#include <func.h>
#include <wrappers.h>

#define MAX_CACHED_VALUES 1024

unsigned int hash_arr( int argc, char **argv ) ;
cacheval_t *new_cacheval( octet_t *arg, unsigned int h, unsigned int ct, int r) ;

/* ======================================================================= */

/**********************************************************
*               Hash an array of strings                  *
**********************************************************/
/*
Arguments:
  argc	number of strings
  argc  the vector containing the strings

Returns:	unsigned integer representing the total hashvalue
 */

unsigned int hash_arr( int argc, char **argv )
{
  int   i ;
  unsigned int hashv = 0 ;

  for( i = 0 ; i < argc ; i++ )
    hashv = lhash( (unsigned char *) argv[i], strlen( argv[i] ), hashv ) ;

  return hashv ;
}

/**********************************************************
*          Create a new structure for cached values       *
**********************************************************/
/*
Arguments:
  arg	The argument, normally a external reference after variable
	substitutions
  h 	the hashvalue calculated for arg
  ct    cache time
  r     the result of evaluation of the external reference (1/0)

Returns:	a cacheval struct with everything filled in
 */

cacheval_t *cacheval_new( octet_t *arg, unsigned int h, unsigned int ct, int r)
{
  cacheval_t *cvp ;
  time_t      t ;

  time( &t ) ;

  cvp = ( cacheval_t * ) Malloc ( sizeof( cacheval_t )) ;

  cvp->timeout = t + ct ;
  cvp->hash = h ;
  cvp->arg = octdup( arg ) ;
  cvp->res = r ;

  return cvp ;
}

void cacheval_free( void *vp )
{
  if( vp ) {
    cacheval_t *cvp = ( cacheval_t *) vp ;
    if( cvp->arg ) free( cvp->arg ) ;
    free( cvp ) ;
  }
}

void *cacheval_dup( void *vp)
{
  cacheval_t *new, *old ;

  old = ( cacheval_t * ) vp ;

  new = cacheval_new( old->arg, old->hash, 0, old->res ) ;
  new->timeout = old->timeout ;
  
  return (void *) new ;
}

/***********************************************************
* Cache a result of the evaluation of a external reference *
***********************************************************/
/*
Arguments:
  harr	The pointer to a array to which the result should be added
  arg	The argument, normally a external reference after variable
	substitutions
  ct    cache time
  r     the result of evaluation of the external reference (1/0)

Returns:	TRUE if OK
*/

int cache_value( ll_t **ll, octet_t *arg, unsigned int ct, int r, octet_t *blob )
{
  unsigned int hash ;
  cacheval_t  *cvp, *tmp ;

  hash = lhash( (unsigned char *) arg->val, arg->len, 0 ) ;

  cvp = cacheval_new( arg, hash, ct, r ) ;

  if( *ll && (*ll)->n == MAX_CACHED_VALUES ) {
    tmp = ll_pop( *ll ) ; 
    cacheval_free( (void *) tmp ) ;
  }

  if( *ll == 0 ) *ll = ll_new( 0, 0, 0, 0 ) ;

  ll_push( *ll, ( void * ) cvp, 0 ) ;

  if( blob && blob->len ) {
    cvp->blob.len = blob->len ;
    cvp->blob.val = blob->val ;
    cvp->blob.size = blob->size ;
    blob->size = 0 ; /* should be kept and not deleted upstream */
  }

  return TRUE ;
}

/************************************************************************
* Checks whether there exists a cache value for this specific reference *
************************************************************************/
/*
Arguments:
  gp	Pointer to a array where results are cached for this type of reference
  arg	The argument, normally a external reference after variable
	substitutions

Returns:	CACHED | result(0/1)
*/

int cached( ll_t *ll, octet_t *arg, octet_t *blob )
{
  unsigned int hash ;
  cacheval_t  *cvp ;
  time_t       t ;
  node_t      *np ;

  if( ll == 0 ) return FALSE ;

  time( &t ) ;

  hash = lhash( (unsigned char *) arg->val, arg->len, 0 ) ;

  for( np = ll->head ; np ; np = np->next ) {
    cvp = (cacheval_t *) np->payload ;
    if( cvp->hash == hash ) {
      if( (time_t) cvp->timeout < t ) return CACHED|EXPIRED ;
      else {
        if( blob ) {
          blob->len = cvp->blob.len ;
          blob->val = cvp->blob.val ;
          blob->size = 0 ; /* static data, shouldn't be deleted upstream */
        }
        return CACHED|cvp->res ;
      }
    }
  }

  return FALSE ;
}

/************************************************************
*          replaces a previously cached result              *
************************************************************/
/*
Arguments:
  gp	Pointer to a array where results are cached for this type of reference
  arg	The argument, normally a external reference after variable
	substitutions
  ct    Number of seconds this result can be cached
  r     The result to be cached

Returns:	TRUE if OK
*/ 

int cache_replace_value( ll_t *ll, octet_t *arg, unsigned int ct, int r, octet_t *blob )
{
  unsigned int hash ;
  cacheval_t  *cvp ;
  time_t       t ;
  node_t      *np ;

  time( &t ) ;

  hash = lhash( (unsigned char *) arg->val, arg->len, 0 ) ;

  for( np = ll->head ; np ; np = np->next ) {
    cvp = (cacheval_t *) np->payload ;
    if( cvp->hash == hash ) {
      cvp->timeout = t + ct ;
      cvp->res = r ;
      if( blob ) {
        if( cvp->blob.len ) {
          if( octcmp( &cvp->blob, blob ) != 0 ) {
            free( cvp->blob.val ) ;
            cvp->blob.len = blob->len ;
            cvp->blob.val = blob->val ;
            cvp->blob.size = blob->size ;
            blob->size = 0 ; /* mark so it want be deleted by someone else */
          }
        }
        else {
          cvp->blob.len = blob->len ;
          cvp->blob.val = blob->val ;
          cvp->blob.size = blob->size ;
          blob->size = 0 ; /* mark so it want be deleted by someone else */
        }
      }
      return TRUE ;
    }
  }

  return FALSE ;
}
