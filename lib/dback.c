/*! \file lib/dback.c
 * \author Roland Hedberg <roland@catalogix.se>
 */
/***************************************************************************
                                 dback.c 
                             -------------------
    begin                : Fri Mar 12 2004
    copyright            : (C) 2004 by Stockholm university, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <sys/types.h>
#include <string.h>
#include <dlfcn.h>

#include <spocp.h>
#include <dback.h>
#include <plugin.h>
#include <wrappers.h>
#include <func.h>
#include <macros.h>
#include <proto.h>

/* ---------------------------------------------------------------------- */

dback_t *dback_load( char *name, char *load ) ;

/* ---------------------------------------------------------------------- */
/*!
 * Does all the dynamic link loader stuff, that is opens the dynamic library and
 * connects the defined symbols to functions in the library.
 * Possible initialization functions contained in the library is NOT run by this routine. 
 * There exists functions that are necessary for a well functioning persisten datastore
 * if any of these are not present then this routine will fail.
 * \param name The name of the plugin
 * \param load The filename of the library to be loaded
 * \return A pointer to a dback_t struct with all the values filled in.
 */
dback_t *dback_load( char *name, char *load )
{
  dback_t   *dback = 0 ;
  void      *handle ;
  char      *modulename ;

  handle = dlopen( load, RTLD_LAZY ) ;

  if( !handle ) {
    traceLog( "Unable to open %s library: [%s]", load,  dlerror() ) ;
    return 0 ;
  }  

  modulename = ( char * ) Malloc( strlen( name ) + strlen( "_dback" ) + 1 ) ;
  sprintf( modulename, "%s_dback", name ) ;


  dback = (dback_t * ) dlsym( handle, modulename ) ;
  if( dback == 0 || dback->magic != MODULE_MAGIC_COOKIE ) {
    traceLog( "%s: Not a proper dback_struct", name ) ;
    dlclose( handle ) ;
    return 0 ;
  }

  dback->handle = handle ;
  dback->name = Strdup( name ) ;

  return dback ;
}

/* ---------------------------------------------------------------------- */

static octet_t *datum_make( octet_t *rule, octet_t *blob, char *bcname )
{
  octet_t *oct = 0 ;
  int      len = 0 ;
  size_t   size ;
  char    *str ;

  len = rule->len + 1 + DIGITS( rule->len ) ;
  if( blob ) len += blob->len + 1 + DIGITS( blob->len ) ;
  if( bcname ) len += strlen( bcname ) + 1 + DIGITS( strlen( bcname )) ;

  len += 4 ;

  size = len ;

  str = ( char * ) Malloc ( size ) ;

  sexp_printv( str, &size, "olola", rule, ":", blob, ":", bcname ) ;

  oct = (octet_t *) Malloc ( sizeof( octet_t )) ;

  oct->val = str ;
  oct->size = len ;
  oct->len = len - size ;

  return oct ;
}

/* ---------------------------------------------------------------------- */

/*
 The arg has the following format
   <rulelen> ':' rule ':' [<bloblen> ':' blob] ':' [<bcnamelen> ':' bcname]
 created by datum_make()
 */
static spocp_result_t datum_parse( octet_t *arg, octet_t *rule, octet_t *blob, char **bcname ) 
{
  spocp_result_t r ;
  octet_t        oct ;

  /* the rule spec */
  if(( r = get_str( arg, rule )) != SPOCP_SUCCESS ) return r ;

  if( *arg->val != ':' ) return SPOCP_SYNTAXERROR ;

  arg->val++ ;
  arg->len-- ;

  /* the blob spec */
  if( *arg->val == ':' ) { /* no blob */
    blob->len = 0 ; 
  }
  else {
    if(( r = get_str( arg, blob )) != SPOCP_SUCCESS ) return r ;
  }

  if( *arg->val != ':' ) return SPOCP_SYNTAXERROR ;

  arg->val++ ;
  arg->len-- ;

  /* the bcname spec */

  if( arg->len == 0 ) *bcname = 0 ;
  else {
    if(( r = get_str( arg, &oct )) != SPOCP_SUCCESS ) return r ;

    *bcname = oct.val ;
  }

  return SPOCP_SUCCESS ;
}

/* ---------------------------------------------------------------------- */
/*!
 * Runs the initialization function present in the database backend library
 * \param dbc A link to the backend information
 * \return SPOCP_SUCCESS on success otherwise an appropriate error code
 */

spocp_result_t dback_init( dbcmd_t *dbc )
{
  spocp_result_t r = SPOCP_UNAVAILABLE ;

  if( dbc && dbc->dback && dbc && dbc->dback->init ) 
    dbc->dback->init( dbc, dbc->dback->conf, 0, &r ) ;

  return r ;
}

/* ---------------------------------------------------------------------- */
/*!
 * Stores information in the persistent storage
 * \param dbc A link to the backend information
 * \param k The key under which the information should be stored.
 * \param o0, o1, s Pieces of the information to be stored
 * \return SPOCP_SUCCESS on success otherwise an hopefully appropriate error code
 */
spocp_result_t dback_save( dbcmd_t *dbc, char *k, octet_t *o0, octet_t *o1, char *s ) 
{
  octet_t        *datum ;
  spocp_result_t  r ;

  if( dbc == 0 || dbc->dback == 0 ) return SPOCP_SUCCESS ;

  datum = datum_make( o0, o1, s) ;

  dbc->dback->put( dbc, (void *) k, (void *) datum, &r ) ; 

  oct_free( datum ) ;

  return r ;
}

/* ---------------------------------------------------------------------- */
/*!
 * Replaces information stored under a specific key in the persistent storage
 * \param dbc A link to the backend information
 * \param k The key under which the information should be stored.
 * \param o0, o1, s Pieces of the information to be stored
 * \return SPOCP_SUCCESS on success otherwise an hopefully appropriate error code
 */
spocp_result_t dback_replace( dbcmd_t *dbc, char *k, octet_t *o0, octet_t *o1, char *s ) 
{
  octet_t        *datum ;
  spocp_result_t  r ;

  if( dbc == 0 || dbc->dback == 0 ) return SPOCP_SUCCESS ;

  datum = datum_make( o0, o1, s ) ;

  dbc->dback->replace( dbc, (void *) k, (void *) datum, &r ) ; 

  oct_free( datum ) ;

  return r ;
}

/* ---------------------------------------------------------------------- */

/*!
 * Reads stored information from the persistent storage
 * \param dbc Information on the persistent store
 * \param key The key under which the information should be stored.
 * \param o0, o1, s The information from the persistent storage split into its 
 *   original pieces.
 * \return SPOCP_SUCCESS on success otherwise an hopefully appropriate error code
 */
spocp_result_t dback_read( dbcmd_t *dbc, char *key, octet_t *o0, octet_t *o1, char **s ) 
{
  octet_t        *datum = 0 ;
  spocp_result_t  r ;

  if( dbc == 0 || dbc->dback == 0 ) return SPOCP_UNAVAILABLE;

  if(( datum = dbc->dback->get( dbc, (void *) key, 0, &r )) == 0 ) {
    return SPOCP_UNAVAILABLE ; 
  }
  else 
    return datum_parse( datum, o0, o1, s ) ;
}

/* ---------------------------------------------------------------------- */
/*!
 * Deletes a information pieces connected to a specific key from the persistent 
 * storage.
 * \param dbc A link to the backend information
 * \param key The key under which the information was stored.
 * \return An appropriate result code
 */

spocp_result_t dback_delete( dbcmd_t *dbc, char *key )
{
  spocp_result_t r ;

  if( dbc == 0 || dbc->dback == 0 ) return SPOCP_SUCCESS ;

  dbc->dback->delete( dbc, (void *) key, 0, &r ) ;

  return r ;
}

/* ---------------------------------------------------------------------- */
/*!
 * Gets all keys from the persistent storage
 * This is dback_open(), dback_first_key(), dback_next_key(), dback_close rolled into
 * one function.
 * \param dbc A link to the persistent store information
 * \param r A pointer to an int where the result code can be placed.
 * \return A octetarr struct containing all the keys as octet_t structs
 */

octarr_t *dback_all_keys( dbcmd_t *dbc, spocp_result_t *r )
{
  if( dbc && dbc->dback == 0 ) return 0 ;

  return (octarr_t *) dbc->dback->allkeys( dbc->handle, 0, 0, r ) ; 
}

/* ---------------------------------------------------------------------- */
/*!
 * Begins a transaction against the persistent store
 * \param dbc A link to the persistent store information
 * \return A Spocp result code
 */

spocp_result_t dback_begin( dbcmd_t *dbc )
{
  void          *handle ;
  spocp_result_t r = SPOCP_SUCCESS ;

  if( dbc && dbc->dback && dback->begin ) {
    if(( handle = dbc->dback->begin( dbc, 0, 0, &r )) != 0 )  dbc->handle = handle ;
    return r ;
  }
  else return SPOCP_NOT_SUPPORTED ;
}

/* ---------------------------------------------------------------------- */
/*!
 * Ends a transaction
 * \param dbc A link to the persistent store information
 * \return The result code
 */

spocp_result_t dback_end( dbcmd_t *dbc)
{
  spocp_result_t r = SPOCP_NOT_SUPPORTED ;

  if( dbc && dbc->dback && dbc->dback->end ) dbc->dback->end( dbc, 0, 0, &r ) ;

  return r ;
}

/* ---------------------------------------------------------------------- */
/*!
 * Commit the changes to the store
 * \param dbc Information about the persistent store
 * \return The result code
 */

spocp_result_t dback_commit( dbcmd_t *dbc)
{
  spocp_result_t r = SPOCP_NOT_SUPPORTED ;

  if( dbc && dbc->dback && dbc->dback->commit ) dbc->dback->commit( dbc, 0, 0, &r ) ;
  
  return r ;
}

/* ---------------------------------------------------------------------- */
/*!
 * Discards all the changes that has been done so far in this
 * transaction.
 * \param dbc A link to the backend information
 * \return The result code
 */

spocp_result_t dback_rollback( dbcmd_t *dbc)
{
  spocp_result_t r = SPOCP_NOT_SUPPORTED ;

  if( dbc && dbc->dback && dbc->dback->rollback )
    dbc->dback->rollback( dbc, 0, 0, &r ) ;
  
  return r ;
}

/* ---------------------------------------------------------------------- */


