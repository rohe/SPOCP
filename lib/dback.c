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
#include <wrappers.h>
#include <func.h>
#include <macros.h>
#include <proto.h>

static char *necessary_function[] = { "get", "put", "delete", "allkeys",  0 } ;
static char *optional_function[] = { "init", "firstkey", "nextkey", "open", "close", 0 } ;

static dback_t *dback_new( void ) 
{
  return ( dback_t * ) Calloc (1, sizeof( dback_t )) ;
}

/* ---------------------------------------------------------------------- */
/*!
 * Does all the dynamic link loader stuff, that is opens the dynamic library and
 * connects the defined symbols to functions in the library.
 * Possible initialization functions contained in the library is NOT run by this routine. 
 * There exists functions that are necessary for a well functioning persisten datastore
 * if any of these are not present then this routine will fail.
 * \param pl A link to the set of plugins defined in the configuration file. The database
 *   plugin is handle in the same way as the backend plugins in this regard.
 * \return A pointer to a dback_t struct with all the values filled in.
 */
dback_t *init_dback( plugin_t *pl )
{
  dback_t   *dback = 0 ;
  dbackfn   *dbf ;
  octarr_t  *oa ;
  char      *error ;
  int        i, j ;
  
  oa = pconf_get_keys_by_plugin( pl, "dback" ) ;
  if( !oa ) {
    traceLog( "No dback definition" ) ;
    return 0 ;
  }
   
  for( j = 0 ; necessary_function[j] ; j++ ) 
    for( i = 0 ; i < oa->n ; i++ ) {
      if( strcmp( necessary_function[j], oa->arr[i]->val ) == 0 ) break ;

    if( i == oa->n ) {
      traceLog( "Necessary dback function \"%s\" missing", necessary_function[j] ) ;
      octarr_free( oa ) ;
      return 0 ; 
    }
  }

  octarr_free( oa ) ;
  oa = 0 ;

  oa = pconf_get_keyval_by_plugin( pl, "dback", "filename" ) ;

  if( !oa ) {
    traceLog( "No link to the db backend library") ;
    return 0 ;
  }

  dback = dback_new() ;

  /* should only be one, disregard the other */
  dback->handle = dlopen( oa->arr[0]->val, RTLD_LAZY ) ;
  if( !dback->handle ) {
    traceLog( "Unable to open %s library: [%s]", oa->arr[0]->val,  dlerror() ) ;
    free( dback ) ;
    return 0 ;
  }  

  octarr_free( oa ) ;
  oa = 0 ;

  /* ------------------------------------------------------------ *
     The necessary functions 
   * ------------------------------------------------------------ */

  for( i = 0 ; necessary_function[i] ; i++ ) {
    oa = pconf_get_keyval_by_plugin( pl, "dback", necessary_function[i] ) ;

    if( oa->n > 1 ) { /* */
      traceLog( "Should only be one %s db backend function defined", necessary_function[i] ) ;
    }

    dbf = ( dbackfn * ) dlsym( dback->handle, oa->arr[0]->val ) ;
    if(( error = dlerror()) != NULL ) {
      traceLog("Couldn't link the %s db backend function", oa->arr[0]->val ) ;
      free( dback ) ;
      return 0 ;
    }

    octarr_free( oa ) ;
    oa = 0 ;

    switch( i ) {
      case 0:
        dback->get = dbf ;
        break ;

      case 1:
        dback->put = dbf ;
        break ;

      case 2:
        dback->delete = dbf ;
        break ;

      case 3:
        dback->allkeys = dbf ;
        break ;
    }
  }

  /* ------------------------------------------------------------ *
     The optional functions 
   * ------------------------------------------------------------ */

  for( i = 0 ; optional_function[i] ; i++ ) {
    oa = pconf_get_keyval_by_plugin( pl, "dback", optional_function[i] ) ;

    if( !oa ) continue ;

    if( oa->n > 1 ) { /* */
      traceLog( "Should only be one %st dback function defined", optional_function[i] ) ;
    }

    dbf = ( dbackfn * ) dlsym( dback->handle, oa->arr[0]->val ) ;
    if(( error = dlerror()) != NULL ) {
      traceLog("Couldn't link the %s dback function", oa->arr[0]->val ) ;
      continue ;
    }

    octarr_free( oa ) ;
    oa = 0 ;

    switch( i ) {
      case 0:
        dback->init = dbf ;
        break ;
    }
  }

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
 * \param dback A link to the backend information
 * \param cfg A pointer to a function that can be used to read information collected from the
 *        configuration file
 * \param conf A pointer to configuration information for usage by the function mentioned above.
 * \return SPOCP_SUCCESS on success otherwise an appropriate error code
 */

spocp_result_t dback_init( dback_t *dback, void *cfg, void *conf )
{
  spocp_result_t r = SPOCP_UNAVAILABLE ;

  if( dback && dback->init ) dback->init( 0, cfg, conf, &r ) ;

  return r ;
}

/* ---------------------------------------------------------------------- */
/*!
 * Stores information in the persistent storage
 * \param dback A link to the backend information
 * \param handle The connection handle cast as a (void *)
 * \param key The key under which the information should be stored.
 * \param oct0, oct1, str Pieces of the information to be stored
 * \return SPOCP_SUCCESS on success otherwise an hopefully appropriate error code
 */
spocp_result_t
 dback_save( dback_t *dback, void *handle, char *key, octet_t *oct0, octet_t *oct1, char *str ) 
{
  octet_t        *datum ;
  spocp_result_t  r ;

  if( dback == 0 ) return SPOCP_SUCCESS ;

  datum = datum_make( oct0, oct1, str) ;

  dback->put( handle, (void *) key, (void *) datum, &r ) ; 

  oct_free( datum ) ;

  return r ;
}

/* ---------------------------------------------------------------------- */
/*!
 * Replaces information stored under a specific key in the persistent storage
 * \param dback A link to the backend information
 * \param handle The connection handle cast as a (void *)
 * \param key The key under which the information should be stored.
 * \param oct0, oct1, s Pieces of the information to be stored
 * \return SPOCP_SUCCESS on success otherwise an hopefully appropriate error code
 */
spocp_result_t
 dback_replace( dback_t *dback, void *handle, char *key, octet_t *oct0, octet_t *oct1, char *s ) 
{
  octet_t        *datum ;
  spocp_result_t  r ;

  if( dback == 0 ) return SPOCP_SUCCESS ;

  datum = datum_make( oct0, oct1, s ) ;

  dback->replace( handle, (void *) key, (void *) datum, &r ) ; 

  oct_free( datum ) ;

  return r ;
}

/* ---------------------------------------------------------------------- */

/*!
 * Reads stored information from the persistent storage
 * \param dback A link to the backend information
 * \param handle The connection handle cast as a (void *)
 * \param key The key under which the information should be stored.
 * \param oct0, oct1, str The information from the persistent storage split into its 
 *   original pieces.
 * \return SPOCP_SUCCESS on success otherwise an hopefully appropriate error code
 */
spocp_result_t
 dback_read( dback_t *dback, void *handle, char *key, octet_t *oct0, octet_t *oct1, char **str ) 
{
  octet_t        *datum = 0 ;
  spocp_result_t  r ;

  if( dback == 0 ) return SPOCP_UNAVAILABLE;

  if(( datum = dback->get( handle, (void *) key, 0, &r )) == 0 ) {
    return SPOCP_UNAVAILABLE ; 
  }
  else 
    return datum_parse( datum, oct0, oct1, str ) ;
}

/* ---------------------------------------------------------------------- */
/*!
 * Deletes a information pieces connected to a specific key from the persistent 
 * storage.
 * \param dback A link to the backend information
 * \param handle The connection handle cast as a (void *)
 * \param key The key under which the information was stored.
 * \return An appropriate result code
 */

spocp_result_t dback_delete( dback_t *dback, void *handle, char *key )
{
  spocp_result_t r ;

  if( dback == 0 ) return SPOCP_SUCCESS ;

  dback->delete( handle, (void *) key, 0, &r ) ;

  return r ;
}

/* ---------------------------------------------------------------------- */

/*!
 * Opens a connection to the persisten storage. Such a connection is usable
 * When you need to keep a connection open while doing other computations.
 * \param dback A link to the backend information
 * \param r     A pointer to a int where the resultcode is placed
 * \return A void pointer to a handle the database backend has return as a
 *  connection token.
 */
void *dback_open( dback_t *dback, spocp_result_t *r )
{
  return dback->open( 0, 0, 0, r ) ;
}

/* ---------------------------------------------------------------------- */
/*!
 * Closes the connection to the persistent store.
 * \param dback A link to the backend information
 * \param handle The connection handle cast as a (void *)
 * \return The result code
 */
spocp_result_t dback_close( dback_t *dback, void *handle )
{
  spocp_result_t r ;

  dback->close( handle, 0, 0, &r ) ;

  return r ;
}

/* ---------------------------------------------------------------------- */

/*!
 * Gets the key to the first (in some sense) information pieces in the 
 * persistent store. One can not make any assumptions that it is the 
 * oldest, newest or anything like that. The only valid assumption is that
 * if dback_first_key() is followed by dback_next_key()'s until dback_next_key()
 * returns NULL, and no keys has been added or deleted during the time this 
 * has taken. Then all keys has been returned.
 * \param dback A link to the backend information
 * \param handle A connection handle to the storage facility.
 * \param r A pointer to an int where the result code can be placed.
 * \return The key expressed as a octet_t struct.
 */
octet_t *dback_first_key( dback_t *dback, void *handle, spocp_result_t *r )
{
  return (octet_t *) dback->firstkey( handle, 0, 0, r ) ;
}

/* ---------------------------------------------------------------------- */

/*!
 * Gets the next ( according to some definition ) key from the persistent store.
 * \param dback A link to the backend information
 * \param handle A connection handle to the storage facility
 * \param key The previous key.
 * \param r A pointer to an int where the result code can be placed.
 * \return The next key expressed as a octet_t struct.
 */
octet_t *dback_next_key( dback_t *dback, void *handle, octet_t *key, spocp_result_t *r )
{
  return (octet_t *) dback->nextkey( handle, (void *) key, 0, r ) ; 
}

/* ---------------------------------------------------------------------- */
/*!
 * Gets all keys from the persistent storage
 * This is dback_open(), dback_first_key(), dback_next_key(), dback_close rolled into
 * one function.
 * \param dback A link to the backend information
 * \param handle A connection handle
 * \param r A pointer to an int where the result code can be placed.
 * \return A octetarr struct containing all the keys as octet_t structs
 */

octarr_t *dback_all_keys( dback_t *dback, void *handle, spocp_result_t *r )
{
  if( dback == 0 ) return 0 ;

  return (octarr_t *) dback->allkeys( handle, 0, 0, r ) ; 
}

/* ---------------------------------------------------------------------- */
/*!
 * Begins a transaction against the persistent store
 * \param dback A link to the backend information
 * \param handle A connection handle
 * \param r A pointer to an int where the result code can be placed.
 * \return A transaction handle cast, to be used when commands are to be run
 *   within this transaction, as a (void *)
 */

void *dback_begin( dback_t *dback, void *handle, spocp_result_t *r )
{
  if( dback->begin ) return dback->begin( handle, 0, 0, r ) ;
  else {
    *r = SPOCP_NOT_SUPPORTED ;
    return NULL ;
  }
}

/* ---------------------------------------------------------------------- */
/*!
 * Ends a transaction
 * \param dback A link to the backend information
 * \param transhandle A handle to the transaction
 * \return The result code
 */

spocp_result_t dback_end( dback_t *dback, void *transhandle )
{
  spocp_result_t r = SPOCP_NOT_SUPPORTED ;

  if( dback->end ) dback->end( transhandle, 0, 0, &r ) ;

  return r ;
}

/* ---------------------------------------------------------------------- */
/*!
 * Commit the changes to the store
 * \param dback A link to the backend information
 * \param transhandle A handle to the transaction
 * \return The result code
 */

spocp_result_t dback_commit( dback_t *dback, void *transhandle )
{
  spocp_result_t r = SPOCP_NOT_SUPPORTED ;

  if( dback->commit ) dback->commit( transhandle, 0, 0, &r ) ;
  
  return r ;
}

/* ---------------------------------------------------------------------- */
/*!
 * Discards all the changes that has been done so far in this
 * transaction.
 * \param dback A link to the backend information
 * \param transhandle A handle to the transaction
 * \return The result code
 */

spocp_result_t dback_rollback( dback_t *dback, void *transhandle )
{
  spocp_result_t r = SPOCP_NOT_SUPPORTED ;

  if( dback->rollback ) dback->rollback( transhandle, 0, 0, &r ) ;
  
  return r ;
}

/* ---------------------------------------------------------------------- */


