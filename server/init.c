#include "locl.h"
RCSID("$Id$");

int run_plugin_init( srv_t *srv ) 
{
  plugin_t *pl ;

  for( pl = srv->plugin ; pl ; pl = pl->next ) {
    if( strcmp(pl->name,"dback") == 0 ) continue ; 
 
    if( pl->init ) {
      traceLog( "Running the init function for %s", pl->name ) ;
      pl->init( &pl->conf, &pl->dyn ) ;
    }
  }
  return 0 ;
}

