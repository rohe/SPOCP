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

pconf_t *pconf_new( char *key, octet_t *val )
{
  pconf_t *new ;

  new = ( pconf_t * ) Malloc ( sizeof( pconf_t )) ;
  new->key = Strdup( key ) ;
  new->val = octnode_add( 0, octdup(val) ) ;
  new->next = 0 ;

  return new ;
}

void pconf_free( pconf_t *pc )
{
  if( pc ) {
    if( pc->key ) free( pc->key ) ;
    if( pc->val ) octnode_free( pc->val ) ;
    if( pc->next ) pconf_free( pc->next ) ;
    free( pc ) ;
  }
}

plugin_t *pconf_set_keyval( plugin_t *top, char *pname, char *key, char *val )
{
  plugin_t *pl ;
  pconf_t  *pc ;
  octet_t   oct ;

  pl = plugin_get( top, pname ) ;

  if( top == 0 ) top = pl ;

  for( pc = pl->conf ; pc ; pc = pc->next ) {
    if( strcmp( key, pc->key ) == 0 ) break ;
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
    octnode_add( pc->val, octdup( &oct ) ) ;
  }
  
  return top ;
}

octnode_t *pconf_get_keyval( plugin_t *pl, char *key )
{
  pconf_t   *pc ;

  for( pc = pl->conf ; pc ; pc = pc->next ) {
    if( strcmp( pc->key, key ) == 0 ) return pc->val ;
  }

  return 0 ;
}

octnode_t *pconf_get_keyval_by_plugin( plugin_t *top, char *pname, char *key )
{
  plugin_t  *pl ;
  octet_t   oct ;

  oct.val = key ;
  oct.len = strlen( key ) ;

  if(( pl = plugin_match( top, &oct )) == 0 ) return 0 ;

  return pconf_get_keyval( pl, key ) ;
}


/* ------------------------------------------------------------------------------------ */

plugin_t *plugin_new( char *name ) 
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

  pl = plugin_get( top, pname ) ;

  oct.val = s ;
  oct.size = 0 ;
  oct.len = strlen( s ) ;

  ct = cachetime_new( &oct ) ;

  if( ct == 0 ) return 0 ; 

  if( pl->ct == 0 ) pl->ct = ct ;
  else {
    for( ctp = pl->ct ; ctp->next ; ctp = ctp->next ) ;
    ctp->next = ct ;
  }

  if( !top ) return pl ;
  else       return top ; 
}

plugin_t *plugin_add_conf( plugin_t *top, char *pname, char *key, char *val )
{
  if( strcmp( key, "cachetime" ) == 0 )
    return plugin_add_cachedef( top, pname, val ) ;
  else 
    return pconf_set_keyval( top, pname, key, val ) ;
}

/* ------------------------------------------------------------------------------------ */

void plugin_free( plugin_t *pl )
{
  if( pl ) {
    if( pl->handle ) dlclose( pl->handle ) ;
    if( pl->name ) free( pl->name ) ;
    if( pl->ct ) cachetime_free( pl->ct ) ;
    if( pl->conf ) pconf_free( pl->conf ) ;
    free( pl ) ;
  }
}

plugin_t *plugin_rm( plugin_t **top, plugin_t *pl )
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
  octnode_t *on, *lib ;
  char      *error ;
  
  for( pl = top ; pl ;  ) {
    lib = pconf_get_keyval( pl, "filename" ) ;
    if( !lib ) {
      traceLog( "No link to the %s plugin library", pl->name ) ;
      pl = plugin_rm( &top, pl ) ;
      continue ;
    }
   
    on = pconf_get_keyval( pl, "test" ) ;
    if( !on || on->next ) { /* */
      traceLog( "Should only be one test function definition for plugin %s", pl->name ) ;
      pl = plugin_rm( &top, pl ) ; 
      continue ;
    }

    pl->handle = dlopen( lib->oct.val, RTLD_LAZY ) ;
    if( !pl->handle ) {
      traceLog( "Unable to open %s library: [%s]", pl->name, dlerror() ) ;
      pl = plugin_rm( &top, pl ) ; 
      continue ;
    }  

    pl->test = ( befunc * ) dlsym( pl->handle, on->oct.val ) ;
    if(( error = dlerror()) != NULL ) {
      traceLog("Couldn't link the %s function in %s", on->oct.val, pl->name ) ;
      pl = plugin_rm( &top, pl ) ; 
      continue ;
    }

    on = pconf_get_keyval( pl, "init" ) ;
    if( on ) { /* */
      if( on->next ) {
        traceLog( "Should only be one init function definition for plugin %s", pl->name ) ;
        pl = plugin_rm( &top, pl ) ; 
        continue ;
      }

      pl->init = (beinitfn * ) dlsym( pl->handle, on->oct.val ) ;
      if(( error = dlerror()) != NULL ) {
        traceLog("Couldn't link the %s function in %s", on->oct.val, pl->name ) ;
        pl = plugin_rm( &top, pl ) ; 
        continue ;
      }
    }

    pl = pl->next ;
  }

  return top ;
}

