/*! \file lib/aci.c \author Roland Hedberg <roland@catalogix.se> 
 */
#include <string.h>

#include <spocp.h>
#include <func.h>
#include <struct.h>
#include <db0.h>
#include <wrappers.h>
#include <macros.h>

/*! A successfull matching of a query against the ruledatabase returns a
 * pointer to a junction where one of the branches is a array of pointer to
 * rule instances. Presently this array will never contain more than one
 * pointer, since the software does not allow several identical rules to be
 * added. The possible static return information is not included when
 * uniqueness is checked. So two 'rules' with the same rightsdefinition but
 * different static return information can not be added. \param ap A pointer
 * to the junction where the endoflist array are \return on success a pointer
 * to the first element in the endoflist array otherwise 0 
 */
ruleinst_t     *
allowing_rule(junc_t * ap)
{
	junc_t         *nap;
	branch_t       *dbp, *ndb;

	if ((dbp = ARRFIND(ap, SPOC_ENDOFLIST))) {
		nap = dbp->val.list;

		while ((ndb = ARRFIND(nap, SPOC_ENDOFLIST))) {
			dbp = ndb;
			nap = dbp->val.list;
		}

		if ((dbp = ARRFIND(dbp->val.list, SPOC_ENDOFRULE))) {
			if (dbp->val.id->n)
				return dbp->val.id->arr[0];
		}
	} else if ((dbp = ARRFIND(ap, SPOC_ENDOFRULE))) {
		if (dbp->val.id->n)
			return dbp->val.id->arr[0];
	}

	return 0;
}

/*! The function which is the starting point for access control, that is
 * matching the query * against a specific ruleset. \param ap A pointer to
 * the start of the ruletree \param comp A set of command parameters \return
 * SPOCP_SUCCESS on success otherwise an appropriate error code 
 */
spocp_result_t
allowed(junc_t * ap, comparam_t * comp)
{
	spocp_result_t  res = SPOCP_DENIED;

	if ((ap = element_match_r(ap, comp->head, comp)))
		res = SPOCP_SUCCESS;
	else if (comp->rc != SPOCP_SUCCESS)
		res = comp->rc;

	return res;
}
