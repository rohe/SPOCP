#include <string.h>

#include <spocp.h>
#include <struct.h>
#include <wrappers.h>
#include <macros.h>
#include <func.h>

#include <srv.h>

#define ASCII_LOWER(c) ( (c) >= 'a' && (c) <= 'z' )
#define ASCII_UPPER(c) ( (c) >= 'A' && (c) <= 'Z' )
#define DIGIT(c)       ( (c) >= '0' && (c) <= '9' )
#define ASCII(c)       ( ASCII_LOWER(c) || ASCII_UPPER(c) )
#define DIRCHAR(c)     ( ASCII(c) || DIGIT(c) || (c) == '-' || (c) == '_' || (c) == '/' )


ruleset_t *new_ruleset( char *name, int len )
{
  ruleset_t *rs ;

  rs = ( ruleset_t *) Calloc( 1, sizeof( ruleset_t )) ;

  pthread_mutex_init( &(rs->transaction), NULL ) ;

  pthread_rdwr_init( &(rs->rw_lock) ) ;

  if( len == 0 ) {
    rs->name = (char * ) Malloc ( 1 )  ;
    *rs->name = '\0' ;
  }
  else rs->name = Strndup( name, len ) ;

  return rs ;
}

void free_ruleset( ruleset_t *rs ) 
{
  if( rs ) {
    pthread_mutex_destroy( &rs->transaction );
    if( rs->name ) free( rs->name ) ;
    if( rs->aci ) free_aci_chain( rs->aci ) ;
    if( rs->db ) free_db( rs->db ) ;
    pthread_rdwr_destroy( &rs->rw_lock ) ;

    if( rs->down ) free_ruleset( rs->down ) ;
    if( rs->left ) free_ruleset( rs->left ) ;
    if( rs->right ) free_ruleset( rs->right ) ;

    free( rs ) ;
  }
}

static int next_part( octet_t *a, octet_t *b )
{
  if( a->len == 0 ) return 0 ;

  if( *a->val == '/' ) {
    a->val++ ;
    a->len-- ;
  }

  if( a->len == 0 ) return 0 ;

  b->val = a->val ;
  b->len = a->len ;
  
  for(  ; b->len && *b->val != '/' ; b->val++, b->len-- ) ;
  
  a->len -= b->len ;

  return 1 ;
}

ruleset_t *find_ruleset( octet_t *name, ruleset_t *rs )
{
  octet_t loc, p ;
  int     c ;

  if( rs == 0 ) return 0 ;
  else if( name->len == 0 ) return rs ; 
  else {
    if( *name->val == '/' && name->len == 1 ) {
      while( 1 ) {
        if( *rs->name == '\0' ) return rs ;

        if( rs->left ) rs = rs->left ;
        else           return 0 ;
      } 
    }
    else {
      if( *name->val == '/' && *rs->name == '\0' ) {
        name->val++ ;
        name->len-- ;
        while( rs->up ) rs = rs->up ;
        rs = rs->down ;
        if( rs == 0 ) return 0 ;
      }

      loc.val = name->val ;
      loc.len = name->len ;

      do {
  
        if( next_part( &loc, &p ) == 0 ) break ;

        do {
          c = oct2strcmp( &loc, rs->name ) ;
  
          if( c == 0 ) {
            if( p.len == 1 && *p.val == '/' ) return rs ;
            rs = rs->down ;
          }
          else if ( c < 0 ) {
            rs = rs->left ;
          }
          else { 
            rs = rs->right ;
          }
        } while( c && rs ) ;
  
        if( rs == 0 ) break ;

        loc.val = p.val ;
        loc.len = p.len ;
  
      } while( loc.len ) ;
    }
  }

  return rs ;
}

ruleset_t *create_ruleset( octet_t *name, ruleset_t **root )
{
  ruleset_t *res = 0, *trs = 0 ;
  octet_t   p, loc ;
  int       c ;

  if( *root == 0 ) {

    if( name->len == 0 || ( name->len && *name->val == '/' )) {
      *root = trs = new_ruleset( "", 0 ) ;
      return trs ;
    }

    loc.val = name->val ;
    loc.len = name->len ;

    do {
      if( next_part( &loc, &p ) == 0 ) break ;

      res = new_ruleset( loc.val, loc.len ) ;

      if( trs ) {
        trs->down = res ;
        res->up = trs ;
      }
      trs = res ;

      loc.val = p.val ;
      loc.len = p.len ;

    } while( loc.len ) ;
  }
  else {
    trs = *root ;

    if( name->len == 0 || ( name->len == 1 && *name->val == '/' )) {
      if( *trs->name == '\0' ) return trs ;
    }
    else if( *name->val == '/' ) {  /* rooted at the top */
      if( strcmp( trs->name, "" ) ) {
        while( trs->up ) trs = trs->up ;
      }

      name->val++ ; 
      name->len-- ;

      if( trs->down ) {
        trs = trs->down ; /* to get to the right level */
      }
      else {
        if( name->len == 0 ) return trs ;

        trs->down = create_ruleset( name, &res ) ;
        trs->down->up = trs ;
        while( trs->down ) trs = trs->down ;
        return trs ;
      }
    }

    if( trs == 0 ) return 0 ;

    loc.val = name->val ;
    loc.len = name->len ;

    do {
      if( next_part( &loc, &p ) == 0 ) break ;

      do {
        c = oct2strcmp( &loc, trs->name ) ;

        if( c == 0 ) {
          if( trs->down == 0 ) { 
            p.val++ ; /* to get rid of the leading '/' */
            p.len-- ; 

            if( p.len == 0 ) return trs ;

            trs->down = create_ruleset( &p, &res ) ;
            trs->down->up = trs ;
            while( trs->down ) trs = trs->down ;
            return trs ;
          }

          trs = trs->down ;
        }
        else if ( c < 0 ) {
          if( trs->left == 0 ) { 
            loc.len += p.len ; /* the whole string */
            trs->left = create_ruleset( &loc, &res ) ;
            trs->left->right = trs ;
            trs->left->up = trs->up ;
            for( trs = trs->left ; trs->down ; trs = trs->down ) ;
            return trs ;
          }

          trs = trs->left ;
        }
        else { 
          if( trs->right == 0 ) {
            loc.len += p.len ; /* the whole string */
            trs->right = create_ruleset( &loc, &res ) ;
            trs->right->left = trs ;
            trs->right->up = trs->up ;
            for( trs = trs->right ; trs->down ; trs = trs->down ) ;
            return trs ;
          }
  
          trs = trs->right ;
        }
      } while( c ) ;

      loc.val = p.val ;
      loc.len = p.len ;

    } while( loc.len ) ;
  }

  return trs ;
}

/* should be of the format 
   [ "/" *( string "/" ) ] s-exp 
   string = 1*( %x30-39 / %x61-7A / %x41-5A )
 */

spocp_result_t get_rs_name( octet_t *orig, octet_t *rsn )
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
    for(  l = 0 ; DIRCHAR( *cp ) && l != orig->len ; cp++, l++ ) ;

    if( l == orig->len ) return SPOCP_SYNTAXERROR ; /* very fishy */

    rsn->val = orig->val ;
    rsn->len = l ;

    orig->val = cp ;
    orig->len -= l ;
  }

  return SPOCP_SUCCESS ;
}

/*
ruleset_t *parse_rs_name( ruleset_t *rs, char **str ) 
{
  char      *sp, *cp ;
  ruleset_t *res = 0 ;

  if( rs == 0 || *str == 0 ) return 0 ;

  sp = *str ;

  if( *sp == '(' ) return rs ;  
  
  if( *sp == '/' )                              * start at the top *
    for( res = rs ; res->up ; res = res->up ) ;
  
                                                * get the name space info *
  for( cp = sp ; DIRCHAR( *cp ) ; cp++ ) ;

  if( *cp != '(' )  return 0 ;                  * Not OK *

  *cp++ = '\0' ;

  res = find_ruleset( sp, res ) ;

  *str = cp ;

  return res ;
}
*/
/*
spocp_result_t 
search_in_tree( ruleset_t *rs, octet_t *arg, octnode_t **blob, spocp_req_info_t *sri, int f )
{
  spocp_result_t  r = SPOCP_DENIED ;
  ruleset_t      *rp ;
  octet_t        old, new ;

  *  check access to operation *
  if(( r = rs_access_allowed( rs, sri, QUERY )) != SPOCP_SUCCESS ) {
    LOG( SPOCP_INFO ) traceLog("Query access denied to rule set") ;
    return r ;
  }
  else {
    old.val = arg->val ;
    old.len = arg->len ;

    * get read lock, do the query and release lock *
    * pthread_rdwr_rlock( &rs->rw_lock ) ; *
    r = spocp_allowed( rs->db, arg, blob, sri ) ;
    * pthread_rdwr_runlock( &rs->rw_lock ) ; *

    if( r != SPOCP_SUCCESS && rs->down ) r = search_in_tree( rs->down, arg, blob, sri, 1 ) ;
  }

  if( r == SPOCP_SUCCESS && f ) {

    for( rp = rs->left ;  r != SPOCP_SUCCESS && rp ; rp = rp->left ) {
      arg->val = old.val ;
      arg->len = old.len ;

      r = search_in_tree( rp, arg, blob, sri, 1 ) ;
    }

    for( rp = rs->right ;  r != SPOCP_SUCCESS && rp ; rp = rp->right ) {
      arg->val = old.val ;
      arg->len = old.len ;

      r = search_in_tree( rp, arg, blob, sri, 1 ) ;
    }

    new.val = arg->val ;
    new.len = arg->len ;

  }

  return r ;
}     
*/

spocp_result_t get_pathname( ruleset_t *rs, char *buf, int buflen ) 
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

int
treeList( ruleset_t *rs, octet_t *arg, spocp_req_info_t *sri, octarr_t *oa, int recurs )
{
  spocp_result_t rc = SPOCP_SUCCESS ;
  ruleset_t      *rp, *trs ;
  octet_t        rsn ;
  char           pathname[BUFSIZ] ;

  if( arg->len && *arg->val == '/' ) {
    if(( rc = get_rs_name( arg, &rsn )) != SPOCP_SUCCESS ) {
      LOG( SPOCP_EMERG ) traceLog( "Strange line \"%s\"", arg->val ) ;
      return rc ;
    }

    if(( trs = create_ruleset( &rsn, &rs )) == 0 ) return SPOCP_OPERATIONSERROR ;
  }
  else trs = rs ;

  /*  check access to operation */
  if(( rc = rs_access_allowed( trs, sri, LIST )) != SPOCP_SUCCESS ) {
    LOG( SPOCP_INFO ) {
      if(rs->name) traceLog("List access denied to rule set \"%s\"", trs->name) ;
      else traceLog("List access denied to the root rule set") ;
    }
  }
  else {
    if(( rc = get_pathname( trs, pathname, BUFSIZ  )) != SPOCP_SUCCESS ) return rc ;

    if( trs->db ) {
      /* get read lock, do the query and release lock */
      /* pthread_rdwr_rlock( &rs->rw_lock ) ; */

      if( arg->len == 0 )
        rc = spocp_list_rules( trs->db, 0, oa, sri, pathname ) ; 
      else 
        rc = spocp_list_rules( trs->db, arg, oa, sri, pathname ) ; 
 
      /* pthread_rdwr_runlock( &rs->rw_lock ) ; */
    }

    if( recurs && trs->down ) {
      for( rp = trs->down ; rp->left ; rp = rp->left ) ;

      for( ; rp ; rp = rp->right ) {
        rc = treeList( rp, arg, sri, oa, recurs ) ;
      }
    }
  }

  return rc ;
}     

