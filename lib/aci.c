/*! \file lib/aci.c \author Roland Hedberg <roland@catalogix.se> 
 */
#include <config.h>

#include <string.h>

#include <spocp.h>
#include <resset.h>
#include <branch.h>
#include <element.h>
#include <db0.h>
#include <wrappers.h>
#include <macros.h>
#include <match.h>

/*! A successfull matching of a query against the rule database returns a
 * pointer to a junction where one of the branches is a array of pointers to
 * rule instances. Presently this array will never contain more than one
 * pointer, this since the software does not allow several identical rules 
 * to be added. The possible static return information is not included when
 * uniqueness is checked. So two 'rules' with the same rights definition but
 * different static return information can not be added. 
 * \param jp A pointer to the junction where the endoflist array are 
 * \return on success a pointer to the first element in the endoflist 
 *  array otherwise NULL 
 */
 
/*@null@*/ ruleinst_t     *
allowing_rule(junc_t *jp)
{
    junc_t         *njp;
    branch_t       *dbp, *ndb;

    if ((dbp = ARRFIND(jp, SPOC_ENDOFLIST))) {
        njp = dbp->val.list;

        while ((ndb = ARRFIND(njp, SPOC_ENDOFLIST))) {
            dbp = ndb;
            njp = dbp->val.list;
        }

        if ((dbp = ARRFIND(dbp->val.list, SPOC_ENDOFRULE))) {
            if (dbp->val.ri)
                return dbp->val.ri;
        }
    } else if ((dbp = ARRFIND(jp, SPOC_ENDOFRULE))) {
        if (dbp->val.ri)
            return dbp->val.ri;
    }

    return NULL;
}

/*! The function which is the starting point for access control, that is
 * matching the query * against a specific ruleset.
 * \param jp A pointer to the start of the ruletree
 * \param comp A set of command parameters
 * \return SPOCP_SUCCESS on success otherwise an appropriate error code 
 */
spocp_result_t
allowed(junc_t * jp, comparam_t *comp, resset_t **rspp)
{
    resset_t    *rs;

    comp->rc = SPOCP_DENIED;

    if ((rs = element_match_r(jp, comp->head, comp))) {
#ifdef AVLUS
        /*DEBUG(SPOCP_DSRV)*/ {
            traceLog(LOG_DEBUG,"Allowed() got Result Set");
            resset_print(rs);
            traceLog(LOG_DEBUG,"---------------------------");
        }
#endif
        comp->rc = SPOCP_SUCCESS ;
        *rspp = rs ;
    }

    return comp->rc;
}
