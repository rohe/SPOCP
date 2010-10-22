/*
 *  eval.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/26/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#include <check.h>
#include <element.h>
#include <index.h>
#include <octet.h>
#include <log.h>
#include <bcondfunc.h>

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief Checks whether a boundary condition is true or false. If the
 *  backend that does the evaluation return true it might be accompanied by a
 *  dynamic blob.
 * \param ep A pointer to a internal representation of the parsed query 
 *  S-expression.
 * \param ri A set of rules that are '<=' the query.
 * \param oa An octarr struct. This is where the dynamic blob will be placed 
 *  if the backend wants to return one.
 * \return A result code 
 */

spocp_result_t
bcond_check(element_t * ep, ruleinst_t *ri, octarr_t ** oa, checked_t **cr)
{
    spocp_result_t  r = SPOCP_DENIED;
    checked_t       *c;
    octarr_t        *loa=0;
    octet_t         *blob=0;
    
    /*
     * find the top 
     */
    while (ep->memberof)
        ep = ep->memberof;
        
    /* have I done this rule already */
    if ((c = checked_rule( ri, cr ))) {
        if (c->rc == SPOCP_SUCCESS )
            if (c->blob)
                *oa = octarr_add(*oa, octdup(c->blob));
        
        return c->rc;
    }
    
    if (ri->bcond == 0) {
        if (ri->blob)
            blob = octdup(ri->blob);
        r = SPOCP_SUCCESS;
    }
    else {
        r = bcexp_eval(ep, ri->ep, ri->bcond->exp, &loa);
        if (loa) {
            blob = loa->arr[0];
            *oa = octarr_add(*oa, blob);
            loa->n = 0;
            octarr_free(loa);
        }
        traceLog(LOG_INFO, "boundary condition \"%s\" returned %d\n",
                 ri->bcond->name, r);
    }
    
    if (r == SPOCP_SUCCESS) {
        add_checked( ri, r, octdup(blob), cr );
        if (blob)
            *oa = octarr_add(*oa, blob);
        
        LOG(SPOCP_INFO) {
            oct_print(LOG_INFO,"Matched rule",ri->rule);
        }
    }
    
    return r;
}

