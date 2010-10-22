/*
 *  treelist.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 2/12/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#include "locl.h"

spocp_result_t
treeList(ruleset_t * rs, conn_t * conn, octarr_t **oapp, int recurs)
{
    spocp_result_t  rc = SPOCP_SUCCESS;
    ruleset_t       *rp;
    char            pathname[BUFSIZ];
    work_info_t     wi;
    
    /*
     * check access to operation 
     */
    memset(&wi,0,sizeof(work_info_t));
    wi.conn = conn;
    if ((rc = operation_access(&wi)) != SPOCP_SUCCESS) {
        LOG(SPOCP_INFO) {
            if (rs->name)
                traceLog(LOG_INFO,
                         "List access denied to rule set \"%s\"",
                         rs->name);
            else
                traceLog(LOG_INFO,
                         "List access denied to the root rule set");
        }
    } else {
        if (0)
            traceLog(LOG_DEBUG, "Allowed to list");
        
        if ((rc = ruleset_pathname(rs, pathname, BUFSIZ)) != SPOCP_SUCCESS)
            return rc;
        
        if (rs->db) {
            rc = dbapi_rules_list(rs->db, 0, wi.oparg, oapp, pathname);
        }
        
        if (0)
            traceLog(LOG_DEBUG,"Done head");
        
        if (recurs && rs->down) {
            for (rp = rs->down; rp->left; rp = rp->left);
            
            for (; rp; rp = rp->right) {
                rc = treeList(rp, conn, oapp, recurs);
            }
        }
    }
    
    return rc;
}
