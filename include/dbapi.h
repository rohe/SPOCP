void           dbapi_db_del( db_t *db, dbcmd_t *dbc ) ;
void          *dbapi_db_dup( db_t *db, spocp_result_t *r ) ;
spocp_result_t dbapi_allowed( db_t *db, octet_t *sexp, octarr_t **on ) ;
spocp_result_t dbapi_rule_rm( db_t *db, dbcmd_t *dbc, octet_t *op ) ;
spocp_result_t dbapi_rule_add( db_t **dpp, plugin_t *p, dbcmd_t *dbc, octarr_t *oa ) ;
spocp_result_t dbapi_rules_list(
  db_t *db, dbcmd_t *dbc, octarr_t *pattern, octarr_t *oa, char *rs ) ;
