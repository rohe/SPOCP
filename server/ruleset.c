/***************************************************************************
                                ruleset.c 
                             -------------------

    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/


#include "locl.h"
RCSID("$Id$");

/* ---------------------------------------------------------------------- */

ruleset_t *ruleset_new( octet_t *name )
{
  ruleset_t *rs ;

  rs = ( ruleset_t *) Calloc( 1, sizeof( ruleset_t )) ;

  if( name == 0 || name->len == 0 ) {
    rs->name = (char * ) Malloc ( 1 )  ;
    *rs->name = '\0' ;
  }
  else rs->name = oct2strdup( name, 0 ) ;

  traceLog( "New ruleset: %s", rs->name ) ;

  return rs ;
}

/* ---------------------------------------------------------------------- */

void ruleset_free( ruleset_t *rs ) 
{
  if( rs ) {
    if( rs->name ) free( rs->name ) ;

    if( rs->down ) ruleset_free( rs->down ) ;
    if( rs->left ) ruleset_free( rs->left ) ;
    if( rs->right ) ruleset_free( rs->right ) ;

    free( rs ) ;
  }
}

/* ---------------------------------------------------------------------- */

static int next_part( octet_t *a, octet_t *b )
{
  if( a->len == 0 ) return 0 ;

  octln( b, a ) ;
  
  for(  ; b->len && DIRCHAR(*b->val) && *b->val != '/' ; b->val++, b->len-- ) ;
  
  if( b->len && ( !DIRCHAR( *b->val ) && *b->val != '/' )) return 0 ;

  a->len -= b->len ;

  return 1 ;
}

/* ---------------------------------------------------------------------- */

static ruleset_t *search_level( octet_t *name, ruleset_t *rs ) 
{
  int c ;

  do {
    traceLog( "search_level: [%s](%d),[%s]", name->val, name->len, rs->name ) ;
    c = oct2strcmp( name, rs->name ) ;
  
    if( c == 0 ) {
      rs = rs->down ;
      break ;
    }
    else if ( c < 0 ) {
      rs = rs->left ;
    }
    else { 
      rs = rs->right ;
    }
  } while( c && rs ) ;

  return rs ;
}

/* ---------------------------------------------------------------------- */

/* returns 1 if a ruleset is found with the given name */
int ruleset_find( octet_t *name, ruleset_t **rs )
{
  octet_t   loc, p ;
  ruleset_t *r, *nr ;

  if( *rs == 0 ) return 0 ;

  /* the root */
  if( name == 0 || name->len == 0 || ( *name->val == '/' && name->len == 1 )) {
    for( r = *rs ; r->up ; r = r->up ) ;
    *rs = r ;
    return 1 ;
  }

  if( *(name->val) == '/' ) {
    if( *(name->val+1) == '/' ) {
      for( r = *rs ; r->up ; r = r->up ) ;
      loc.val = name->val ;
      loc.len = 2 ;
      if( r->down ) {
        nr = search_level( &loc, r->down ) ;
        if( nr == 0 ){
          *rs = r ;
          return 0 ;
        }
      }
      else return 0 ;

      name->len -= 2 ;
      name->val += 2 ;
    }
    else {
      for( r = *rs ; r->up ; r = r->up ) ;
      name->len-- ;
      name->val++ ;
    }
  }
  else r = *rs ;

  octln( &loc, name ) ;

  do {
    if( *loc.val == '/' ) {
      loc.val++ ;
      loc.len-- ;
    }
 
    if( next_part( &loc, &p ) == 0 ) break ;

    nr = search_level( &loc, r ) ; 
  
    if( nr == 0 ) {
      loc.len += p.len ;
      break ; /* this is as far as it gets */
    }

    octln( &loc, &p ) ;
    r = nr ;
  } while( loc.len ) ;
 
  *rs = r ;

  if( loc.len == 0 ) return 1 ;

  octln( name, &loc ) ;

  return 0 ;
}

/* ---------------------------------------------------------------------- */

static ruleset_t *add_to_level( octet_t *name, ruleset_t *rs )
{
  int c ;
  ruleset_t *new ;

  new = ruleset_new( name ) ;

  if( rs == 0 ) return new ;

  do {
    traceLog( "add_level: [%s](%d),[%s]", name->val, name->len, rs->name ) ;
    c = oct2strcmp( name, rs->name ) ;
  
    if ( c < 0 ) {
      if( !rs->left ) {
        rs->left = new ;
        c = 0 ;
      }
      rs = rs->left ;
    }
    else { 
      if( !rs->right ) {
        rs->right = new ;
        c = 0 ;
      }
      rs = rs->right ;
    }
  } while( c && rs ) ;

  return rs ;
}

/* ---------------------------------------------------------------------- */

ruleset_t *ruleset_create( octet_t *name, ruleset_t **root )
{
  ruleset_t *res = 0, *r = 0, *nr ;
  octet_t   p, loc ;

  if( *root == 0 ) {
    if( name == 0 || name->len == 0 || ( name->len == 1 && *name->val == '/' )) 
      return ruleset_new( 0 ) ;
  }

  if( name ) octln( &loc, name ) ;
  else {
    loc.len = loc.size = 0 ;
    loc.val = 0 ;
  }

  res = *root ;
  if( ruleset_find( &loc, &res ) == 1 ) return res ;

  /* special case */
  if( *loc.val == '/' && *(loc.val+1) == '/' ) {
    p.val = loc.val +2 ;
    p.len = loc.len -2 ;
    loc.len = 2 ;

    if( res == 0 ) {
      res = ruleset_new( 0 ) ;
      r = ruleset_new( &loc ) ;
      res->down = r ;
      r->up = res ; 
      octln( &loc, &p ) ;
    }
    else {
      r = add_to_level( &loc, res ) ;
      r->up = res ;
      octln( &loc, &p ) ;
    }
  }
  else r = res ;

  do {
    if( *loc.val == '/' ) {
      loc.val++ ;
      loc.len-- ;
    }
 
    if( next_part( &loc, &p ) == 0 ) break ;

    if( r->down ) nr = add_to_level( &loc, r->down ) ; 
    else r->down = nr = ruleset_new( &loc ) ;

    nr->up = r ;

    octln( &loc, &p ) ;
    r = nr ;
  } while( loc.len ) ;
 
  return r ;
}

/* ---------------------------------------------------------------------- */

/* should be of the format 
   [ "/" *( string "/" ) ] s-exp 
   string = 1*( %x30-39 / %x61-7A / %x41-5A / %x2E )
 */

spocp_result_t ruleset_name( octet_t *orig, octet_t *rsn )
{
  char   *cp ;
  size_t l ;

  cp = orig->val ;

  if( *cp != '/' ) {
    rsn->val = 0 ;
    rsn->len = 0 ;
  }
  else {
    /* get the name space info */
    for(  l = 0 ; ( DIRCHAR( *cp ) || *cp == '/' ) && l != orig->len ; cp++, l++ ) ;

    if( l == orig->len ) return SPOCP_SYNTAXERROR ; /* very fishy */

    rsn->val = orig->val ;
    rsn->len = l ;

    orig->val = cp ;
    orig->len -= l ;
  }

  return SPOCP_SUCCESS ;
}

spocp_result_t pathname_get( ruleset_t *rs, char *buf, int buflen ) 
{
  int       len = 0 ;
  ruleset_t *rp ;
   
  for( rp = rs ; rp ; rp = rp->up ) {
    len += 2 ;
    len += strlen( rp->name ) ;
  }

  if( len > buflen ) return SPOCP_OPERATIONSERROR ;

  *buf = 0 ;

  for( rp = rs ; rp ; rp = rp->up ) {
    strcat( buf, "/" ) ;
    strcat( buf, rp->name ) ;
  }

  return SPOCP_SUCCESS ;
}

spocp_result_t treeList( ruleset_t *rs, conn_t *conn, octarr_t *oa, int recurs )
{
  spocp_result_t rc = SPOCP_SUCCESS ;
  ruleset_t      *rp ;
  char           pathname[BUFSIZ] ;

  /*  check access to operation */
  if(( rc = operation_access( conn )) != SPOCP_SUCCESS ) {
    LOG( SPOCP_INFO ) {
      if(rs->name) traceLog("List access denied to rule set \"%s\"", rs->name) ;
      else traceLog("List access denied to the root rule set") ;
    }
  }
  else {
    if(( rc = pathname_get( rs, pathname, BUFSIZ  )) != SPOCP_SUCCESS ) return rc ;

    if( rs->db ) {
      /* get read lock, do the query and release lock */
      /* pthread_rdwr_rlock( &rs->rw_lock ) ; */

      rc = spocp_list_rules( rs->db, conn->oparg, oa, pathname ) ; 
 
      /* pthread_rdwr_runlock( &rs->rw_lock ) ; */
    }

    if( recurs && rs->down ) {
      for( rp = rs->down ; rp->left ; rp = rp->left ) ;

      for( ; rp ; rp = rp->right ) {
        rc = treeList( rp, conn, oa, recurs ) ;
      }
    }
  }

  return rc ;
}     

