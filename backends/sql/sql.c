/***************************************************************************
                          be_sql.c  -  description
                             -------------------
    begin                : Mon Sep 8 2003
    copyright            : (C) 2003 by Karolinska Institutet, Sweden
    email                : ola.gustafsson@itc.ki.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#ifdef HAVE_LIBODBC

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <spocp.h>

#include <sqlext.h>

typedef long SQLLEN ;

befunc sql_test ;
/*
  type         = "sql"
  typespecific = sqlserver ";" vset
  sqlserver    = [ uid [ ":" pwd ] "@" ] dsn
  uid          = utf8string
  pwd          = utf8string
  dsn          = utf8string
  vset         = sqlset *( ";" sqlset )                        ; If more than one is specified, all must be true
  sqlset       = [ modifier ] sqlstring [ constraint ]
  modifier     = "-"                                           ; The modifier - negates the result.
  constraint   = "{" val *( "," val ) "}"                      ; If a constraint for success is dependent on the amount of rows returned or the value of a specific field.
  val          = var_term conj term
  var_term     = "%" ref
  conj         = "&" / "!"
  ref          = "n" / d *( d )                                ; "n" stands for amount of rows, digits stands for rownumber, counting from 0
  d            = %x30-39
  term         = var_term / const_term
  const_term   = '"' string '"'

  Standard behaviour is represented by the vset-string: 
        -SQL{%n&0}
  Which means returns true if not the amount of rows is 0.
 */

spocp_result_t sql_test( 
  element_t *qp, element_t *rp, element_t *xp, octet_t *arg, pdyn_t *bcp, octet_t *blob )
{
  octet_t  **argv;
  octet_t  **argv_ident;
  octet_t  **argv_uid = 0;
  /* octet_t  **sql_constraint; */
  int n,n_ident,n_uid,i ;
  /* int n_constraint; */
  char *uid = "";
  char *pwd = "";
  char *dsn = "";
  char *sql = "";
  int negate = FALSE;
  SQLHENV hEnv = 0;
  SQLHDBC hDbc = 0;
  SQLHSTMT hStmt;
  SQLINTEGER ret;
  SQLLEN  nRows	= 0;

  /* Split into sqlserver and vset parts */
  argv = oct_split( arg, ';', 0, 0, 0, &n );
  argv[0]->val[argv[0]->len] = 0;
  /* Get the dsn and ident parts */
  argv_ident = oct_split( argv[0], '@', 0, 0, 0, &n_ident );

  if( n_ident == 0 ) {
    dsn = argv[0]->val;
  } else {
    argv_ident[0]->val[argv_ident[0]->len] = 0;
    argv_ident[1]->val[argv_ident[1]->len] = 0;
    dsn = argv_ident[1]->val;
    /* Get uid and pwd parts */
    argv_uid = oct_split( argv_ident[0], ':', 0, 0, 0, &n_uid );
    if( n == 0 ) {
      uid = argv_ident[0]->val;
    } else {
      argv_uid[0]->val[argv_uid[0]->len] = 0;
      argv_uid[1]->val[argv_uid[1]->len] = 0;
      uid = argv_uid[0]->val;
      pwd = argv_uid[1]->val;
    }
  }

  DEBUG( SPOCP_DBCOND ){
    traceLog("Uid: %s DSN: %s",uid,dsn);
  }

  /* Allocate ODBC environment */
  if(SQLAllocEnv( &hEnv ) != SQL_SUCCESS) {
    DEBUG( SPOCP_DBCOND ){
      traceLog("Could not SQLAllocEnv.");
    }
    oct_freearr( argv ) ;
    oct_freearr( argv_ident ) ;
    oct_freearr( argv_uid ) ;
    return SPOCP_OPERATIONSERROR ;
  }

  /* Allocate ODBC connect structure */
  if(SQLAllocConnect( hEnv, &hDbc ) != SQL_SUCCESS) {
    DEBUG( SPOCP_DBCOND ){
      traceLog("Could not SQLAllocConnect.");
    }
    oct_freearr( argv ) ;
    oct_freearr( argv_ident ) ;
    oct_freearr( argv_uid ) ;
    SQLFreeEnv( hEnv );
    return SPOCP_OPERATIONSERROR ;
  }
  
  /* Connect */
  if(!SQL_SUCCEEDED( SQLConnect( hDbc, (SQLCHAR*)dsn, SQL_NTS, (SQLCHAR*)uid, SQL_NTS, (SQLCHAR*)pwd, SQL_NTS ))) {
    DEBUG( SPOCP_DBCOND ){
      traceLog("Could not SQLConnect.");
    }
    oct_freearr( argv ) ;
    oct_freearr( argv_ident ) ;
    oct_freearr( argv_uid ) ;
    SQLFreeConnect( hDbc );
    SQLFreeEnv( hEnv );
    return SPOCP_OPERATIONSERROR ;
  }

  /* For each sql */
  for(i = 1;i<=n;i++) {
    sql = "";
    negate = FALSE;
    argv[i]->val[argv[i]->len] = 0;

    sql = argv[i]->val;

    if(*sql == '-') {
      negate = TRUE;
      sql++;
    }

    DEBUG( SPOCP_DBCOND ){
      traceLog("SQL: %s Negate: %d",sql,negate);
    }

    /* Allocate ODBC statement */
    if(SQLAllocStmt( hDbc, &hStmt ) != SQL_SUCCESS) {
      DEBUG( SPOCP_DBCOND ){
	traceLog("Could not SQLAllocStmt.");
      }
      oct_freearr( argv ) ;
      oct_freearr( argv_ident ) ;
      oct_freearr( argv_uid ) ;
      SQLFreeConnect( hDbc );
      SQLFreeEnv( hEnv );
      return SPOCP_OPERATIONSERROR ;
    }

    /* Prepare statement and SQL */
    if(SQLPrepare( hStmt, (SQLCHAR*)sql, SQL_NTS ) != SQL_SUCCESS ) {
      DEBUG( SPOCP_DBCOND ){
	traceLog("Could not SQLPrepare.");
      }
      oct_freearr( argv ) ;
      oct_freearr( argv_ident ) ;
      oct_freearr( argv_uid ) ;
      SQLFreeStmt( hStmt, SQL_DROP );
      SQLFreeConnect( hDbc );
      SQLFreeEnv( hEnv );
      return SPOCP_OPERATIONSERROR ;
    }
    
    ret =  SQLExecute( hStmt );
    
    if(ret == SQL_NO_DATA ) {
      /* N = 0 */
      DEBUG( SPOCP_DBCOND ){
	traceLog("SQLExecute returned SQL_NO_DATA.");
      }
      if(negate == FALSE) {
	oct_freearr( argv ) ;
	oct_freearr( argv_ident ) ;
	oct_freearr( argv_uid ) ;
	return FALSE;
      }
    } else if( ret == SQL_SUCCESS_WITH_INFO ) {
      /* N = 0 ?*/
      DEBUG( SPOCP_DBCOND ){
	traceLog("SQLExecute returned SQL_SUCCESS_WITH_INFO.");
      }
    } else if( ret != SQL_SUCCESS ) {
      DEBUG( SPOCP_DBCOND ){
	traceLog("Could not SQLExecute.");
      }
      SQLFreeStmt( hStmt, SQL_DROP );
      SQLFreeConnect( hDbc );
      SQLFreeEnv( hEnv );
      oct_freearr( argv ) ;
      oct_freearr( argv_ident ) ;
      oct_freearr( argv_uid ) ;
      return SPOCP_OPERATIONSERROR ;
    }

    SQLRowCount( hStmt, &nRows );

    DEBUG( SPOCP_DBCOND ){
      traceLog("Got %d rows",nRows);
    }
   
    if(nRows > 0 && negate == TRUE) {
      return SPOCP_DENIED;
    }
    if(nRows == 0 && negate == FALSE) {
      return SPOCP_DENIED;
    }
  }


  SQLFreeStmt( hStmt, SQL_DROP );
 
  SQLDisconnect( hDbc );
  SQLFreeConnect( hDbc );
  SQLFreeEnv( hEnv );

  oct_freearr( argv ) ;
  oct_freearr( argv_ident ) ;
  oct_freearr( argv_uid ) ;

  return SPOCP_SUCCESS ;
}

#else
int spocp_no_odbc ;
#endif
