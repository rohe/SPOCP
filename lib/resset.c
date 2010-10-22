
/***************************************************************************
 *                           resset.c  -  description
 *                            -------------------
 *    begin                : Thu Jan 12 2005
 *    copyright            : (C) 2005 by Umeå University, Sweden
 *    email                : roland.hedberg@adm.umu.se
 *
 *    COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
 *    of this package for details.
 *
 ****************************************************************************/

#include <config.h>
#include <sys/types.h>

#include <stdlib.h>
#include <string.h>

#include <octet.h>
#include <resset.h>
#include <spocp.h>
#include <macros.h>
#include <wrappers.h>
#include <log.h>

/* #define AVLUS 1 */

/*
 *----------------------------------------------------------------------
 */

resset_t *
resset_new( ruleinst_t *ri, octarr_t *blob )
{
	resset_t *rs;

	rs = (resset_t *) Malloc( sizeof(resset_t));
	rs->ri = ri;
	rs->blob = blob ;
	rs->next = 0;

	return rs;
}

void
resset_free( resset_t *rs)
{
	if( rs) {
		if (rs->blob ) {
			octarr_free( rs->blob );
		}
		if (rs->next) {
			resset_free(rs->next);
		}
		Free( rs );
	}
}

/*!
 * \brief Adds a resset_t struct to a list of such.
 * \param rs The, possibly empty, list to which the new resset_t struct should
 *  be added
 * \param ri A pointer to a ruleinst_t struct, representing a rule that are
 *  part of this result set.
 * \param blob A octarr_t struct containing blobs that are bound to the 
 *  rules in this result set
 * \return The head of the list.
 */
resset_t *
resset_add( resset_t *rs, ruleinst_t *ri, octarr_t *blob)
{
	resset_t *head, *tail;

	tail = resset_new( ri, blob );

	if (rs)
		head = rs;
	else 
		return tail;

	while( rs->next )
		rs = rs->next;

	rs->next = tail;
	return head;
}

/*!
 * \brief Joins two lists of resset_t list
 * \param a A pointer to a list of resset_t struct's.
 * \param a A pointer to another list of resset_t struct's.
 * \return A pointer to the head of the combined list.
 */

resset_t *
resset_extend( resset_t *a, resset_t *b)
{
	resset_t *head;

	if (a) {
		head = a;
		while( a->next)
			a = a->next;
		a->next = b;
		return head;
	}
	else
		return b;
}

void 
resset_print( resset_t *rs)
{
	if (rs == 0)
		return;

	if (rs->ri)
		ruleinst_print(rs->ri, "/");
	else 
		traceLog(LOG_DEBUG,"No rules??");

	if (rs->blob)
		octarr_print(LOG_DEBUG, rs->blob);
	else 
		traceLog(LOG_DEBUG,"No blob");

	if (rs->next)
		resset_print(rs->next);
}

int 
resset_len(resset_t *rsp)
{
    int i;
    
    if (rsp== 0) {
        return 0;
    }
    
    for (i=1; rsp->next; rsp=rsp->next, i++) ;
    return i;
}

/* ---------------------------------------------------------------------- */

qresult_t *
qresult_new( spocp_result_t rc, resset_t *rs )
{
	qresult_t *res;

	res = (qresult_t *)Malloc(sizeof(qresult_t));
	res->rc = rc;
	res->part = rs;
	res->next = 0;

	return res;
}

qresult_t *
qresult_add( qresult_t *res, spocp_result_t rc, resset_t *rs)
{
	qresult_t *head ;

	if (res == NULL)
		return qresult_new( rc, rs );
	else
		head = res;

	while( res->next )
		res = res->next;

	res->next = qresult_new( rc, rs );

	return head;
}

qresult_t *
qresult_join( qresult_t *a, qresult_t *b)
{
	if (a){
		while( a->next )
			a = a->next;

		a->next = b;
	}
	else
		return b;

	return a;
}

void 
qresult_free( qresult_t *res )
{
	if (res){
		resset_free( res->part );
		if (res->next)
			qresult_free(res->next);
		Free(res);
	}
}

