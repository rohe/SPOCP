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
 * \param pl A link to the set of plugins defined in the configuration file. The database
 *   plugin is handle in the same way as the backend plugins in this regard.
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
 * \param dback A link to the backend information
 * \param cfg A pointer to a function that can be used to read information collected from the
 *        configuration file
 * \param conf A pointer to configuration information for usage by the function mentioned above.
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
 * \param dback A link to the backend information
 * \param handle The connection handle cast as a (void *)
 * \param key The key under which the information should be stored.
 * \param oct0, oct1, str Pieces of the information to be stored
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
 * \param dback A link to the backend information
 * \param handle The connection handle cast as a (void *)
 * \param key The key under which the information should be stored.
 * \param oct0, oct1, s Pieces of the information to be stored
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
 * \param dback A link to the backend information
 * \param dbc The connection handle cast as a (void *)
 * \param key The key under which the information should be stored.
 * \param oct0, oct1, str The information from the persistent storage split into its 
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
 * \param dback A link to the backend information
 * \param handle The connection handle cast as a (void *)
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
 * Opens a connection to the persisten storage. Such a connection is usable
 * When you need to keep a connection open while doing other computations.
 * \param dback A link to the backend information
 * \param r     A pointer to a int where the resultcode is placed
 * \return A void pointer to a handle the database backend has return as a
 *  connection token.
 *
void *dback_open( dback_t *dback, spocp_result_t *r )
{
  return dback->open( 0, 0, 0, r ) ;
}

 * ---------------------------------------------------------------------- * 
 *!
 * Closes the connection to the persistent store.
 * \param dback A link to the backend information
 * \param handle The connection handle cast as a (void *)
 * \return The result code
 * 
spocp_result_t dback_close( dback_t *dback, void *handle )
{
  spocp_result_t r ;

  dback->close( handle, 0, 0, &r ) ;

  return r ;
}

 * ---------------------------------------------------------------------- *

 *!
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
 * 
octet_t *dback_first_key( dback_t *dback, void *handle, spocp_result_t *r )
{
  return (octet_t *) dback->firstkey( handle, 0, 0, r ) ;
}

 * ---------------------------------------------------------------------- * 

 *!
 * Gets the next ( according to some definition ) key from the persistent store.
 * \param dback A link to the backend information
 * \param handle A connection handle to the storage facility
 * \param key The previous key.
 * \param r A pointer to an int where the result code can be placed.
 * \return The next key expressed as a octet_t struct.
 * 
octet_t *dback_next_key( dback_t *dback, void *handle, octet_t *key, spocp_result_t *r )
{
  return (octet_t *) dback->nextkey( handle, (void *) key, 0, r ) ; 
}

 * ---------------------------------------------------------------------- */
/*!
 * Gets all keys from the persistent storage
 * This is dback_open(), dback_first_key(), dback_next_key(), dback_close rolled into
 * one function.
 * \param dback A link to the backend information
 * \param handle A connection handle
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
 * \param dback A link to the backend information
 * \param handle A connection handle
 * \param r A pointer to an int where the result code can be placed.
 * \return A transaction handle cast, to be used when commands are to be run
 *   within this transaction, as a (void *)
 */

spocp_result_t dback_begin( dback_t *dback, dbcmd_t *dbc )
{
  void          *handle ;
  spocp_result_t r = SPOCP_SUCCESS ;

  if( dback->begin ) {
    if(( handle = dback->begin( dbc, 0, 0, &r )) != 0 )  dbc->handle = handle ;
    return r ;
  }
  else return SPOCP_NOT_SUPPORTED ;
}

/* ---------------------------------------------------------------------- */
/*!
 * Ends a transaction
 * \param dback A link to the backend information
 * \param transhandle A handle to the transaction
 * \return The result code
 */

spocp_result_t dback_end( dback_t *dback, dbcmd_t *dbc)
{
  spocp_result_t r = SPOCP_NOT_SUPPORTED ;

  if( dback->end ) dback->end( dbc, 0, 0, &r ) ;

  return r ;
}

/* ---------------------------------------------------------------------- */
/*!
 * Commit the changes to the store
 * \param dback A link to the backend information
 * \param transhandle A handle to the transaction
 * \return The result code
 */

spocp_result_t dback_commit( dback_t *dback, dbcmd_t *dbc)
{
  spocp_result_t r = SPOCP_NOT_SUPPORTED ;

  if( dback->commit ) dback->commit( dbc, 0, 0, &r ) ;
  
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

spocp_result_t dback_rollback( dback_t *dback, dbcmd_t *dbc)
{
  spocp_result_t r = SPOCP_NOT_SUPPORTED ;

  if( dback->rollback ) dback->rollback( dbc, 0, 0, &r ) ;
  
  return r ;
}

/* ---------------------------------------------------------------------- */


