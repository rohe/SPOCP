/***************************************************************************

                spocp.c  -  contains the public interface to spocp

                             -------------------

    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <stdlib.h>
#include <config.h>
#include <string.h>

#include <macros.h>
#include <struct.h>
#include <func.h>
#include <spocp.h>
#include <wrappers.h>
#include <proto.h>
#include <dbapi.h>

/*
   returns
 */

spocp_result_t spocp_allowed( void *vp, octet_t *sexp, octarr_t **on )
{
  db_t       *db = ( db_t *) vp ;

  return dbapi_allowed( db, sexp, on ) ;
}

void spocp_del_database( void *vp )
{
  db_t  *db = (db_t *) vp ;

  dbapi_db_del( db, NULL ) ;
}

spocp_result_t spocp_del_rule( void *vp, octet_t *op )
{
  db_t           *db = ( db_t *) vp ;

  return dbapi_rule_rm( db, NULL, op ) ;
}

spocp_result_t spocp_list_rules( void *vp, octarr_t *pattern, octarr_t *oa, char *rs ) 
{
  db_t *db = ( db_t *) vp ;

  return dbapi_rules_list( db, NULL, pattern, oa, rs ) ;
}

spocp_result_t spocp_add_rule( void **vp, octarr_t *oa )
{
  db_t          **db = ( db_t **) vp  ;

  return dbapi_rule_add( db, NULL, NULL, oa ) ;
}

void *spocp_dup( void *vp, spocp_result_t *r ) 
{
  db_t *db = (db_t *) vp ;


  return dbapi_db_dup( db, r ) ;
}
