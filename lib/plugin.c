#include <config.h>

#include <sys/types.h>
#include <string.h>
#include <dlfcn.h>

#include <spocp.h>
#include <plugin.h>
#include <wrappers.h>
#include <func.h>

/* ------------------------------------------------------------------------------------ */

plugin_t *plugin_get( plugin_t *top, char *name )  ;

/* ------------------------------------------------------------------------------------ */

static pconf_t *pconf_new( char *key, octet_t *val )
{
  pconf_t *new ;

  new = ( pconf_t * ) Malloc ( sizeof( pconf_t )) ;
  new->key = Strdup( key ) ;
  new->val = octarr_add( 0, octdup(val)) ;
  new->next = 0 ;

  return new ;
}

static void pconf_free( pconf_t *pc )
{
  if( pc ) {
    if( pc->key ) free( pc->key ) ;
    if( pc->val ) octarr_free( pc->val ) ;
    if( pc->next ) pconf_free( pc->next ) ;
    free( pc ) ;
  }
}

static plugin_t *pconf_set_keyval( plugin_t *top, char *pname, char *key, char *val )
{
  plugin_t *pl ;
  pconf_t  *pc = 0 ;
  octet_t   oct ;

  pl = plugin_get( top, pname ) ;

  if( top == 0 ) top = pl ;

  if( key ) {
    for( pc = pl->conf ; pc ; pc = pc->next ) {
      if( strcmp( key, pc->key ) == 0 ) break ;
    }
  }

  oct.val = val ;
  oct.size = 0 ;
  oct.len = strlen( val ) ;

  if( pc == 0  ) {
    if( !pl->conf ) 
      pl->conf = pconf_new( key, &oct ) ;
    else {
      for( pc = pl->conf ; pc->next ; pc = pc->next ) ;
      pc->next = pconf_new( key, &oct ) ;
    }
  } 
  else {
    octarr_add( pc->val, octdup( &oct ) ) ;
  }
  
  return top ;
}

octarr_t *pconf_get_keys_by_plugin( plugin_t *top, char *pname )
{
  pconf_t   *pc ;
  octarr_t  *res = 0 ;
  plugin_t  *pl ;
  octet_t   oct ;

  if( pname == 0 || *pname == '\0' ) return 0 ;

  oct.val = pname ;
  oct.len = strlen( pname ) ;
  oct.size = 0 ;

  if(( pl = plugin_match( top, &oct )) == 0 ) return 0 ;

  for( pc = pl->conf ; pc ; pc = pc->next ) {
    oct.val = pc->key ;
    oct.len = strlen( pc->key ) ;
    res = octarr_add( res, octdup( &oct ) ) ;
  }

  return res ;
}

octarr_t *pconf_get_keyval( plugin_t *pl, char *key )
{
  pconf_t   *pc ;

  for( pc = pl->conf ; pc ; pc = pc->next ) {
    if( strcmp( pc->key, key ) == 0 ) return pc->val ;
  }

  return 0 ;
}

octarr_t *pconf_get_keyval_by_plugin( plugin_t *top, char *pname, char *key )
{
  plugin_t  *pl ;
  octet_t   oct ;

  if( pname == 0 || *pname == '\0' ) return 0 ;

  oct.val = pname ;
  oct.len = strlen( pname ) ;
  oct.size = 0 ;

  if(( pl = plugin_match( top, &oct )) == 0 ) return 0 ;

  return pconf_get_keyval( pl, key ) ;
}


/* ------------------------------------------------------------------------------------ */

static plugin_t *plugin_new( char *name ) 
{
  plugin_t *new ;

  new = ( plugin_t * ) Calloc ( 1, sizeof( plugin_t )) ;

#ifdef HAVE_LIBPTHREAD
  pthread_mutex_init( &new->mutex, NULL ) ;
#endif

  new->name = Strdup( name ) ;

  return new ;
}

plugin_t *plugin_match( plugin_t *top, octet_t *oct )
{
  plugin_t *pl ;

  if( top == 0 ) return 0 ;

  for( pl = top ; pl ; pl = pl->next ) 
    if( oct2strcmp( oct, pl->name ) == 0 ) return pl ;
  
  return 0 ;
}

plugin_t *plugin_get( plugin_t *top, char *name ) 
{
  plugin_t *pl ;

  if( top == 0 ) return plugin_new( name ) ;

  for( pl = top ; pl ; pl = pl->next ) 
    if( strcmp( name, pl->name ) == 0 ) return pl ;
  
  /* no matching definition found */

  for( pl = top ; pl->next ; pl = pl->next ) ;

  pl->next = plugin_new( name ) ;
  pl->next->prev = pl ;

  return pl->next ;
}

plugin_t *plugin_add_cachedef( plugin_t *top, char *pname, char *s )
{
  cachetime_t *ct = 0, *ctp ;
  plugin_t    *pl ;
  octet_t      oct ;

  if( s == 0 || *s == '\0' ) return top ;

  pl = plugin_get( top, pname ) ;

  oct_assign( &oct, s ) ;

  ct = cachetime_new( &oct ) ;

  if( ct == 0 ) return 0 ; 

  if( pl->dyn.ct == 0 ) pl->dyn.ct = ct ;
  else {
    for( ctp = pl->dyn.ct ; ctp->next ; ctp = ctp->next ) ;
    ctp->next = ct ;
  }

  if( !top ) return pl ;
  else       return top ; 
}

plugin_t *plugin_add_conf( plugin_t *top, char *pname, char *key, char *val )
{
  if( key == 0 || *key == '\0' ) return 0 ;

  return pconf_set_keyval( top, pname, key, val ) ;
}

/* ------------------------------------------------------------------------------------ */

static void plugin_free( plugin_t *pl )
{
  if( pl ) {
    if( pl->handle ) dlclose( pl->handle ) ;
    if( pl->name ) free( pl->name ) ;
    if( pl->dyn.ct ) cachetime_free( pl->dyn.ct ) ;
    if( pl->conf ) pconf_free( pl->conf ) ;
    free( pl ) ;
  }
}

static plugin_t *plugin_rm( plugin_t **top, plugin_t *pl )
{
  plugin_t *next ;

  next = pl->next ;

  if( pl == *top ) {
    *top = next ;
  }
  else {
    pl->prev->next = next ;
    if( next ) next->prev = pl->prev ;
  }

  plugin_free( pl ) ;

  return next ; 
}

/* ------------------------------------------------------------------------------------ */

plugin_t *init_plugin( plugin_t *top )
{
  plugin_t  *pl ;
  octarr_t  *oa ;
  char      *error ;
  
  for( pl = top ; pl ; pl = pl->next ) {
    oa = pconf_get_keyval( pl, "filename" ) ;
    if( !oa ) {
      traceLog( "No link to the %s plugin library", pl->name ) ;
      pl = plugin_rm( &top, pl ) ;
      continue ;
    }
   
    /* should only be one, disregard the other */
    pl->handle = dlopen( oa->arr[0]->val, RTLD_LAZY ) ;
    if( !pl->handle ) {
      traceLog( "Unable to open %s library: [%s]", pl->name, dlerror() ) ;
      pl = plugin_rm( &top, pl ) ; 
      continue ;
    }  

    oa = pconf_get_keyval( pl, "test" ) ;
    if( oa ) {
      if( oa->n > 1 ) { /* */
        traceLog( "Should only be one test function definition for plugin %s", pl->name ) ;
        pl = plugin_rm( &top, pl ) ; 
        continue ;
      }

      pl->test = ( befunc * ) dlsym( pl->handle, oa->arr[0]->val ) ;
      if(( error = dlerror()) != NULL ) {
        traceLog("Couldn't link the %s function in %s", oa->arr[0]->val, pl->name ) ;
        pl = plugin_rm( &top, pl ) ; 
        continue ;
      }
    }
    else pl->test = 0 ;

    oa = pconf_get_keyval( pl, "init" ) ;
    if( oa ) { /* */
      if( oa->n  > 1 ) {
        traceLog( "Should only be one init function definition for plugin %s", pl->name ) ;
        pl = plugin_rm( &top, pl ) ; 
        continue ;
      }

      pl->init = (beinitfn * ) dlsym( pl->handle, oa->arr[0]->val ) ;
      if(( error = dlerror()) != NULL ) {
        traceLog("Couldn't link the %s function in %s", oa->arr[0]->val, pl->name ) ;
        pl = plugin_rm( &top, pl ) ; 
        continue ;
      }
    }
    else pl->init = 0 ;

    oa = pconf_get_keyval( pl, "poolsize" ) ;
    if( oa ) { /* */
      long l ;

      if( is_numeric( oa->arr[0], &l ) == SPOCP_SUCCESS ) {
        /* any other reasonable sizelimit ? */
        if( l > (long) 0 )  pl->dyn.size = (int) l ;
      }
    }
    else pl->dyn.size = 0 ;

    oa = pconf_get_keyval( pl, "cachetime" ) ;
    if( oa ) { /* */
      cachetime_t *ct ;
      int          i ;

      pl->dyn.ct = ct = cachetime_new( oa->arr[0] ) ;
      for( i = 1 ; i < oa->n ; i++ ) {
        ct->next = cachetime_new( oa->arr[i] ) ;
        ct = ct->next ;
      }
    }
    else pl->dyn.ct = 0 ;

  }

  return top ;
}

void plugin_display( plugin_t *pl )
{
  octarr_t *oa ;
  char      *tmp ;

  for( ; pl ; pl = pl->next ) {
    traceLog( "Active backend: %s", pl->name ) ;
    if(( oa = pconf_get_keyval( pl, "description" )) != 0 ) {
      tmp = oct2strdup( oa->arr[0], '\\' ) ; 
      traceLog( "\tdescription: %s", tmp ) ;
      free( tmp ) ;
    } 
  }
}

