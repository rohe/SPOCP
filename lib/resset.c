
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

#include <struct.h>
#include <func.h>
#include <spocp.h>
#include <macros.h>
#include <wrappers.h>

/* #define AVLUS 1 */

/*
 *----------------------------------------------------------------------
 */

resset_t *
resset_new( spocp_index_t *t, octarr_t *blob )
{
	resset_t *rs;

	rs = (resset_t *) Malloc( sizeof(resset_t));
	rs->si = index_cp(t);
	rs->blob = blob ;
	rs->next = 0;

#ifdef AVLUS
	traceLog(LOG_DEBUG,"====----____");
	resset_print(rs);
	traceLog(LOG_DEBUG,"____----====");
#endif

	return rs;
}

void
resset_free( resset_t *rs)
{
	if( rs) {
#ifdef AVLUS 
		traceLog(LOG_DEBUG,"resset_free %p", rs);
#endif
		index_delete( rs->si );
		if (rs->blob ) {
#ifdef AVLUS 
			traceLog(LOG_DEBUG,"octarr_free %p", rs->blob);
#endif
			octarr_free( rs->blob );
		}
		if (rs->next) {
			resset_free(rs->next);
		}
		Free( rs );
	}
}

resset_t *
resset_add( resset_t *rs, spocp_index_t *t, octarr_t *blob)
{
	resset_t *head, *tail;

	tail = resset_new( t, blob );
#ifdef AVLUS
	traceLog(LOG_DEBUG,"resset_add tail %p", tail);
#endif

	if (rs)
		head = rs;
	else 
		return tail;

	while( rs->next )
		rs = rs->next;

	rs->next = tail;
	return head;
}

resset_t *
resset_join( resset_t *a, resset_t *b)
{
	resset_t *head;

#ifdef AVLUS
	traceLog(LOG_DEBUG,"resset_join %p %p", a, b);
#endif
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

resset_t *
resset_compact( resset_t *r)
{
	resset_t *h, *t;

	if (r == NULL)
		return r;

	for ( h = r->next ; h != NULL ; h = t ) {
		t = h->next;
		index_extend( r->si, h->si);
		resset_free( h );
	}

	r->next = 0;

	return r;
}

resset_t *
resset_and( resset_t *a, resset_t *b)
{
	spocp_index_t	*si;

	if( a==0 || b == 0)
		return NULL;

	resset_compact( a );
	resset_compact( b );

	si = index_and(a->si, b->si);
	if (si) {
		index_delete(a->si);
		a->si = si;
	}
	else {
		index_delete( a->si );
		a->si = 0;
	}

	resset_free( b );

	return a;
}

void 
resset_print( resset_t *rs)
{
	if (rs == 0)
		return;

	if (rs->si)
		index_print(rs->si);
	else 
		traceLog(LOG_DEBUG,"No ruleindex");

	if (rs->blob)
		octarr_print(LOG_DEBUG, rs->blob);
	else 
		traceLog(LOG_DEBUG,"No blob");

	if (rs->next)
		resset_print(rs->next);
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

