/***************************************************************************
                          backend.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <string.h>
#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif

#include <struct.h>
#include <func.h>
#include <wrappers.h>
#include <db0.h>
#include <spocp.h>
#include <macros.h>
#include <plugin.h>

ref_t *ref_new( void ) 
{
  ref_t *new ;

  new = ( ref_t *) Calloc (1, sizeof( ref_t )) ;

#ifdef HAVE_LIBPTHREAD
    pthread_mutex_init( &(new->mutex), NULL ) ;
#endif

  return new ;
}

void plugin_display( plugin_t *pl )
{
  octnode_t *on ;
  char      *tmp ;

  for( ; pl ; pl = pl->next ) {
    traceLog( "Active backend: %s", pl->name ) ;
    if(( on = pconf_get_keyval( pl, "description" )) != 0 ) {
      tmp = oct2strdup( &on->oct, '\\' ) ; 
      traceLog( "\tdescription: %s", tmp ) ;
      free( tmp ) ;
    } 
  }
}


/*
extref_t *ref_add( extref_t *erp, ref_t *rp )
{
  ref_t *nr ;

  if( erp == 0 ) erp = extref_new( ) ;

  if( erp->refl == 0 ) erp->refl = rp ;
  else {
    for( nr = erp->refl ; nr->nextref ; nr = nr->nextref ) ;
    nr->nextref = rp ;
  }

  return erp ;
}
*/

ref_t *ref_dup( ref_t *old, ruleinfo_t *ri )
{
  ref_t *new ;

  if( old == 0 ) return 0 ;

  new = ref_new( ) ;

  new->plugin = old->plugin ;

  new->bc.size = 0 ;
  octcpy( &new->bc, &old->bc ) ;

  new->arg.val = new->bc.val + ( old->arg.val - old->bc.val ) ;
  new->arg.len = old->arg.len ;

  new->ctime = old->ctime ;
  new->refc = old->refc ;
  new->inv = old->inv ;

  new->cachedval = ll_dup( old->cachedval ) ;
  new->next = junc_dup( old->next, ri ) ;

  return new ;
}

/************************************************************
*                                                           *
************************************************************/

int do_ref( ref_t *rp, parr_t *pap, becpool_t *bcp, octet_t *blob )
{
  junc_t        *jp = 0 ;
  int            r ;
  octet_t       *arg ;
  spocp_result_t b ;

  jp = 0 ;
  b = SPOCP_DENIED ;

  /* do variable substitution */
  arg = var_sub( &rp->arg, pap ) ;

  /* a variable substitution failed */
  if( !arg ) {
    LOG( SPOCP_ERR ) {
      char *tmp ;
      tmp = oct2strdup( &rp->arg, '\\' ) ;
      traceLog("Variable substitution on \"%s\" failed", tmp ) ;
      free( tmp ) ;
    }
    return FALSE ;
  }
  else 
    DEBUG( SPOCP_DBCOND ) {
      char *tmp ;

      if( arg->len ) {
        tmp = oct2strdup( arg, '\\' ) ;
        traceLog("Checking boundary condition: %s", tmp ) ;
        free( tmp ) ;
      }
      else
        traceLog("Checking boundary condition without arguments " ) ;
    }

  /* have a cached result ? */
  if( rp->ctime ) {
#ifdef HAVE_LIBPTHREAD
    pthread_mutex_lock( &rp->mutex ) ;
#endif 
    r = cached( rp->cachedval, arg, blob ) ;
#ifdef HAVE_LIBPTHREAD
    pthread_mutex_unlock( &rp->mutex ) ;
#endif 
  }
  else r = FALSE ;


  if( r == CACHED ) {
    DEBUG( SPOCP_DBCOND ) {
      if( r & TRUE ) traceLog("Cached value: TRUE" ) ;
      else traceLog("Cached value: FALSE" ) ;
    }
  }
 
  /* Not cached or has expired */
  if( r == (CACHED|EXPIRED) || r == FALSE ) {
    b = rp->plugin->test( arg, bcp, blob ) ;
    if( b == SPOCP_SUCCESS || b == SPOCP_DENIED ) { 
      if( rp->inv ) {
        DEBUG( SPOCP_DBCOND ) traceLog("Inverting response %d", b ) ;
        if( b == SPOCP_SUCCESS ) b = SPOCP_DENIED ;
        else if( b == SPOCP_DENIED ) b = SPOCP_SUCCESS ;
      }

      if( rp->ctime ) {
        if( r == (CACHED|EXPIRED) ) {
          if( b == SPOCP_SUCCESS ) 
            cache_replace_value( rp->cachedval, arg, rp->ctime, TRUE, blob ) ;
          else 
            cache_replace_value( rp->cachedval, arg, rp->ctime, FALSE, blob ) ;
        }
        else {
          if( b == SPOCP_SUCCESS ) 
            cache_value( &rp->cachedval, arg, rp->ctime, TRUE, blob ) ;
          else
            cache_value( &rp->cachedval, arg, rp->ctime, FALSE, blob ) ;
        }
      }
    }
    else {/* backend failures is regarded as denies but not cached */
      b = SPOCP_DENIED ; /* Don't cache or invert failures */
    }
  }

  if( arg != &rp->arg ) oct_free( arg ) ;

  if( b == SPOCP_SUCCESS || r == (CACHED|TRUE) ) return TRUE ;
  else return FALSE ;
}


/************************************************************
*                                                           *
************************************************************/

erset_t *erset_new( size_t size )
{
  erset_t *es ;

  es = ( erset_t *) Malloc ( sizeof( erset_t )) ;

  es->n = 0 ;
  es->size = size ;
  es->ref = ( ref_t ** ) Calloc ( size, sizeof( ref_t * )) ;
  es->next = 0 ;
  es->other = 0 ;

  return es ;
}

void erset_free( erset_t *es )
{
  size_t i ;

  if( es ) {
    if( es->size ) {
      for( i = 0 ; i < es->n ; i++ ) ref_free( es->ref[i] ) ;
      free( es->ref ) ;
    }
    free( es ) ;
  }
}

erset_t *erset_dup( erset_t *old, ruleinfo_t *ri )
{
  erset_t *new ;
  size_t  i ;

  new = erset_new( old->n ) ;
  for( i = 0 ; i < new->n ; i++ ) new->ref[i] = ref_dup( old->ref[i], ri ) ;
  new->n = old->n ;

  if( old->other ) new->other = erset_dup( old->other, ri ) ;

  if( old->next ) new->next = junc_dup( old->next, ri ) ;

  return new ;
}

int erset_cmp( erset_t *es, element_t *ep )
{
  size_t i ;
  int    r = 0 ;
  set_t  *set = ep->e.set ;

  if( es->n != ( size_t ) set->n ) return ( es->n - set->n ) ;
  
  for( i = 0 ; i < es->n ; i++ ) {
    if(( r = octcmp( &es->ref[i]->bc, &set->element[i]->e.atom->val )) != 0 )  break ;
  }

  return r ;
}

erset_t *er_cmp( erset_t *es, element_t *ep )
{
  for( ; es ; es = es->other )
    if( erset_cmp( es, ep ) == TRUE ) return es ;

  return 0 ;
}

/************************************************************
*                                                           *
************************************************************/

/*
extref_t *extref_dup( extref_t *old, ruleinfo_t *ri ) 
{
  extref_t *new ; 
  ref_t    *rp, *nrp ;
 
  if( old == 0 ) return 0 ;

  new = extref_new( ) ;

  for( rp = old->refl ; rp ; rp = rp->nextref ) {
    nrp = ref_dup( rp, ri ) ;
    ref_add( new, nrp ) ;
  }

  return new ;
}
*/

int ref_eq( ref_t *r1, ref_t *r2 )
{
  if( r1->plugin != r2->plugin ) return FALSE ;

  if( octcmp( &r1->arg, &r2->arg ) ) return FALSE ;

  return TRUE ;
}

octet_t *var_sub( octet_t *val, parr_t *gap )
{
  char     *rv ;
  int      subs = 0, i, size, n, m ;
  size_t   l ;
  keyval_t *kv ;
  octet_t  oct, *res = 0, *kvk, *kvv ;

  /* no variable substitutions necessary */
  if(( n = octstr( val, "${" )) < 0 ) return val ;

  /* If there is nothing to substitute with then it can't succeed */
  if( gap == 0 ) return 0 ;

  /* absolutely arbitrary */
  size = val->len * 2 ;
  res = oct_new( size, 0 ) ;
  rv = res->val ;

  oct.val = val->val ;
  oct.len = val->len ;

  while( oct.len ) {
    if(( n = octstr( &oct, "${" )) < 0 ) {
      if( !subs ) return val ;
      else {
        if( res->size - res->len < oct.len ) {
          if( oct_resize( res, res->len + oct.len ) != SPOCP_SUCCESS ) {
            oct_free( res ) ;
            return 0 ;
          }
          rv = res->val + res->len ;
        }
        memcpy( rv, oct.val, oct.len ) ;
        res->len += oct.len ;
        return res ;
      }
    }

    if( n ) {
      if( res->size - res->len < ( size_t ) n ) {
        if( oct_resize( res, res->len + n ) != SPOCP_SUCCESS ) {
          oct_free( res ) ;
          return 0 ;
        }
        rv = res->val + res->len ;
      }
      memcpy( rv, oct.val, n ) ;
      oct.val += n ;
      oct.len -= n ;
      rv += n ;
      res->len += n ;
    }

    /* step past the ${ */
    oct.val += 2 ;
    oct.len -= 2 ;

    /* strange if no } is found */
    if(( n = oct_find_balancing( &oct, '{', '}' )) < 0 ) {
      oct_free( res ) ;
      return 0 ;
    }

    m = oct.len ;
    oct.len = n ;

    for( i = 0 ; i < gap->n ; i++ ) {
      kv = (keyval_t *) gap->vect[i] ;
      kvk = kv->key ;

      if( octcmp( &oct, kvk ) == 0 ) {
        kvv = kv->val ;

        l = kvv->len ;
        
        if( res->size - res->len < l ) {
          if( oct_resize( res, res->len + l ) != SPOCP_SUCCESS ) {
            oct_free( res ) ;
            return 0 ;
          }
          rv = res->val + res->len ;
        }

        memcpy( rv, kvv->val, l ) ;
        rv += l ;
        res->len += l ;
 
        break ;
      }
    }

    /* substitution failed */
    if( gap->n == i ) {
      oct_free( res ) ;
      return 0 ;
    }

    n++ ; /* to step over the trailing '}' too */
    oct.val += n ;
    oct.len = m - n ;

    subs++ ;
  }

  return res ;
}

/*
int init_backend( void *conf, confgetfn *cgf )
{
  int i ;

  for( i = 0 ; spocp_be[i].name ; i++ ) {
    if( spocp_be[i].init ) {
      spocp_be[i].init( cgf, conf, 0 ) ;
    }
  }

  return 1 ;
}
*/
