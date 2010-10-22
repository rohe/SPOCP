
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

#if defined(HAVE_LIBODBC) && defined(HAVE_SQLEXT_H)

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <rvapi.h>
#include <log.h>

#include <sqlext.h>

#ifndef SQLLEN
typedef long SQLLEN;
#endif

befunc          sql_test;
/*
 * type = "sql" typespecific = sqlserver ";" vset sqlserver = [ uid [ ":" pwd
 * ] "@" ] dsn uid = utf8string pwd = utf8string dsn = utf8string vset =
 * sqlset *( ";" sqlset ) ; If more than one is specified, all must be true
 * sqlset = [ modifier ] sqlstring [ constraint ] modifier = "-" ; The
 * modifier - negates the result. constraint = "{" val *( "," val ) "}" ; If a 
 * constraint for success is dependent on the amount of rows returned or the
 * value of a specific field. val = var_term conj term var_term = "%" ref conj 
 * = "&" / "!" ref = "n" / d *( d ) ; "n" stands for amount of rows, digits
 * stands for rownumber, counting from 0 d = %x30-39 term = var_term /
 * const_term const_term = '"' string '"'
 * 
 * Standard behaviour is represented by the vset-string: -SQL{%n&0} Which
 * means returns true if not the amount of rows is 0. 
 */

spocp_result_t
sql_test(cmd_param_t * cpp, octet_t * blob)
{
	octarr_t      *argv;
	octarr_t      *argv_ident;
	octarr_t      *argv_uid = 0;
	/*
	 * octet_t **sql_constraint; 
	 */
	int             i;
	/*
	 * int n_constraint; 
	 */
	char           *uid = "";
	char           *pwd = "";
	char           *dsn = "";
	char           *sql = "";
	int             negate = FALSE;
	SQLHENV         hEnv = 0;
	SQLHDBC         hDbc = 0;
	SQLHSTMT        hStmt;
	SQLINTEGER      ret;
	SQLLEN          nRows = 0;

	/*
	 * Split into sqlserver and vset parts 
	 */
	argv = oct_split(cpp->arg, ';', 0, 0, 0);
	/*
	 * Get the dsn and ident parts 
	 */
	argv_ident = oct_split(argv->arr[0], '@', 0, 0, 0);

	if (argv_ident->n == 0) {
		dsn = oct2strdup(argv->arr[0], 0);
	} else {
		dsn = oct2strdup(argv->arr[1], 0);
		/*
		 * Get uid and pwd parts 
		 */
		argv_uid = oct_split(argv_ident->arr[0], ':', 0, 0, 0);

		if (argv_uid->n == 0) {
			uid = oct2strdup(argv_ident->arr[0], 0);
		} else {
			uid = oct2strdup(argv_uid->arr[0], 0);
			pwd = oct2strdup(argv_uid->arr[1], 0);
		}
	}

	DEBUG(SPOCP_DBCOND) {
		traceLog(LOG_DEBUG,"Uid: %s DSN: %s", uid, dsn);
	}

	/*
	 * Allocate ODBC environment 
	 */
	if (SQLAllocEnv(&hEnv) != SQL_SUCCESS) {
		DEBUG(SPOCP_DBCOND) {
			traceLog(LOG_DEBUG,"Could not SQLAllocEnv.");
		}
		octarr_free(argv);
		octarr_free(argv_ident);
		octarr_free(argv_uid);
		if (*dsn != 0) free(dsn);
		if (*uid != 0) free(uid);
		if (*pwd != 0) free(pwd);
		return SPOCP_OPERATIONSERROR;
	}

	/*
	 * Allocate ODBC connect structure 
	 */
	if (SQLAllocConnect(hEnv, &hDbc) != SQL_SUCCESS) {
		DEBUG(SPOCP_DBCOND) {
			traceLog(LOG_DEBUG,"Could not SQLAllocConnect.");
		}
		octarr_free(argv);
		octarr_free(argv_ident);
		octarr_free(argv_uid);
		if (*dsn != 0) free(dsn);
		if (*uid != 0) free(uid);
		if (*pwd != 0) free(pwd);
		SQLFreeEnv(hEnv);
		return SPOCP_OPERATIONSERROR;
	}

	/*
	 * Connect 
	 */
	if (!SQL_SUCCEEDED
	    (SQLConnect
	     (hDbc, (SQLCHAR *) dsn, SQL_NTS, (SQLCHAR *) uid, SQL_NTS,
	      (SQLCHAR *) pwd, SQL_NTS))) {
		DEBUG(SPOCP_DBCOND) {
			traceLog(LOG_DEBUG,"Could not SQLConnect.");
		}
		octarr_free(argv);
		octarr_free(argv_ident);
		octarr_free(argv_uid);
		if (*dsn != 0) free(dsn);
		if (*uid != 0) free(uid);
		if (*pwd != 0) free(pwd);
		SQLFreeConnect(hDbc);
		SQLFreeEnv(hEnv);
		return SPOCP_OPERATIONSERROR;
	}

	/*
	 * For each sql 
	 */
	for (i = 1; i <= argv->n; i++) {
		negate = FALSE;

		sql = oct2strdup( argv->arr[i], 0);

		if (*sql == '-') {
			negate = TRUE;
			sql++;
		}

		DEBUG(SPOCP_DBCOND) {
			traceLog(LOG_DEBUG,"SQL: %s Negate: %d", sql, negate);
		}

		/*
		 * Allocate ODBC statement 
		 */
		if (SQLAllocStmt(hDbc, &hStmt) != SQL_SUCCESS) {
			DEBUG(SPOCP_DBCOND) {
				traceLog(LOG_DEBUG,"Could not SQLAllocStmt.");
			}
			octarr_free(argv);
			octarr_free(argv_ident);
			octarr_free(argv_uid);
			if (*dsn != 0) free(dsn);
			if (*uid != 0) free(uid);
			if (*pwd != 0) free(pwd);
			free(sql);
			SQLFreeConnect(hDbc);
			SQLFreeEnv(hEnv);
			return SPOCP_OPERATIONSERROR;
		}

		/*
		 * Prepare statement and SQL 
		 */
		if (SQLPrepare(hStmt, (SQLCHAR *) sql, SQL_NTS) != SQL_SUCCESS) {
			DEBUG(SPOCP_DBCOND) {
				traceLog(LOG_DEBUG,"Could not SQLPrepare.");
			}
			octarr_free(argv);
			octarr_free(argv_ident);
			octarr_free(argv_uid);
			if (*dsn != 0) free(dsn);
			if (*uid != 0) free(uid);
			if (*pwd != 0) free(pwd);
			free(sql);
			SQLFreeStmt(hStmt, SQL_DROP);
			SQLFreeConnect(hDbc);
			SQLFreeEnv(hEnv);
			return SPOCP_OPERATIONSERROR;
		}

		ret = SQLExecute(hStmt);

		if (ret == SQL_NO_DATA) {
			/*
			 * N = 0 
			 */
			DEBUG(SPOCP_DBCOND) {
				traceLog(LOG_DEBUG,"SQLExecute returned SQL_NO_DATA.");
			}
			if (negate == FALSE) {
				octarr_free(argv);
				octarr_free(argv_ident);
				octarr_free(argv_uid);
				if (*dsn != 0) free(dsn);
				if (*uid != 0) free(uid);
				if (*pwd != 0) free(pwd);
				free(sql);
				return FALSE;
			}
		} else if (ret == SQL_SUCCESS_WITH_INFO) {
			/*
			 * N = 0 ?
			 */
			DEBUG(SPOCP_DBCOND) {
				traceLog(LOG_DEBUG,
				    "SQLExecute returned SQL_SUCCESS_WITH_INFO.");
			}
		} else if (ret != SQL_SUCCESS) {
			DEBUG(SPOCP_DBCOND) {
				traceLog(LOG_DEBUG,"Could not SQLExecute.");
			}
			SQLFreeStmt(hStmt, SQL_DROP);
			SQLFreeConnect(hDbc);
			SQLFreeEnv(hEnv);
			octarr_free(argv);
			octarr_free(argv_ident);
			octarr_free(argv_uid);
			if (*dsn != 0) free(dsn);
			if (*uid != 0) free(uid);
			if (*pwd != 0) free(pwd);
			free(sql);
			return SPOCP_OPERATIONSERROR;
		}

		SQLRowCount(hStmt, &nRows);

		DEBUG(SPOCP_DBCOND) {
			traceLog(LOG_DEBUG,"Got %d rows", nRows);
		}

		free(sql);

		if (nRows > 0 && negate == TRUE) {
			return SPOCP_DENIED;
		}
		if (nRows == 0 && negate == FALSE) {
			return SPOCP_DENIED;
		}
	}


	SQLFreeStmt(hStmt, SQL_DROP);

	SQLDisconnect(hDbc);
	SQLFreeConnect(hDbc);
	SQLFreeEnv(hEnv);

	octarr_free(argv);
	octarr_free(argv_ident);
	octarr_free(argv_uid);
	if (*dsn != 0) free(dsn);
	if (*uid != 0) free(uid);
	if (*pwd != 0) free(pwd);

	return SPOCP_SUCCESS;
}

plugin_t        sql_module = {
	SPOCP20_PLUGIN_STUFF,
	sql_test,
	NULL,
	NULL,
	NULL
};
#else
int             spocp_no_odbc;
#endif 
