#ifndef __DBAPI_H
#define __DBAPI_H

#include <db0.h>

/*
 * =============================================================================== 
 */

spocp_result_t	parse_canonsexp( octet_t *sexp, element_t **target);

/*
 * =============================================================================== 
 */

spocp_result_t  spocp_add_rule(void **vp, octarr_t * oa);
spocp_result_t  spocp_allowed(void *vp, octet_t * sexp, octarr_t ** on);
spocp_result_t  spocp_del_rule(void *vp, octet_t * ids);
spocp_result_t  spocp_list_rules(void *, octarr_t *, octarr_t *, char *);
spocp_result_t  spocp_open_log(char *file, int level);
void            spocp_del_database(void *vp);
void           *spocp_get_rules(void *vp, char *file, int *rc);
void           *spocp_dup(void *vp, spocp_result_t * r);

/*
 * =============================================================================== 
 */

void            dbapi_db_del(db_t * db, dbcmd_t * dbc);
void           *dbapi_db_dup(db_t * db, spocp_result_t * r);
spocp_result_t  dbapi_allowed(db_t * db, octet_t * sexp, octarr_t ** on);
spocp_result_t  dbapi_rule_rm(db_t * db, dbcmd_t * dbc, octet_t * op);
spocp_result_t  dbapi_rule_add(db_t ** dpp, plugin_t * p, dbcmd_t * dbc,
			       octarr_t * oa);
spocp_result_t  dbapi_rules_list(db_t * db, dbcmd_t * dbc, octarr_t * pattern,
				 octarr_t * oa, char *rs);

#endif

