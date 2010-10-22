
/***************************************************************************
                             ll.c  -  description
                             -------------------

    begin                : Fri Feb 6 2003
    copyright            : (C) 2009,2010 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <stdlib.h>
#include <syslog.h>

#include <varr.h>
#include <ll.h>
#include <result.h>
#include <log.h>
#include <wrappers.h>

ll_t           *
ll_new(cmpfn * cf, ffunc * ff, dfunc * df, pfunc * pf)
{
	ll_t           *lp;

	lp = (ll_t *) Calloc(1, sizeof(ll_t));

	/*
	 * lp->head = lp->tail = 0 ; 
	 */

	lp->cf = cf;
	lp->ff = ff;
	lp->df = df;
	lp->pf = pf;

	return lp;
}

void
ll_free(ll_t * lp)
{
	node_t         *np, *pp;

	if (lp) {
		for (np = lp->head; np; np = pp) {
			if (lp->ff)
				lp->ff(np->payload);
			pp = np->next;
			Free(np);
		}
		Free(lp);
	}
}

node_t         *
node_new(void *vp)
{
	node_t         *np;

	np = (node_t *) Calloc(1, sizeof(node_t));
	np->payload = vp;
	np->next = np->prev = 0;

	return np;
}

/* The linked list push/pop works according to the FIFO-principal */
ll_t           *
ll_push(ll_t * lp, void *vp, int nodup)
{
	node_t         *np;

	if (lp == NULL)
		return 0;

	if (lp->head == NULL) {	/* empty list */
		lp->tail = lp->head = node_new(vp);
		lp->n = 1;
	} else {
		if (nodup) {
			/*
			 * should I let anyone know I don't want to add this ? 
			 */
			if ((np = ll_find(lp, vp)) != 0) {
				traceLog(LOG_INFO,
				    "Attempting to add already present rule");
				return lp;
			}
		}
		np = node_new(vp);
		lp->tail->next = np;
		np->prev = lp->tail;
		lp->tail = np;
		lp->n++;
	}

	return lp;
}

void           *
ll_pop(ll_t * lp)
{
	void           *vp;
	node_t         *np;

	if (lp == 0 || lp->n == 0)
		return NULL;	/* empty list */
    else if (lp->n == 1) { /* The last one */
		np = lp->head;
        lp->head = lp->tail = 0;
        lp->n = 0;
		vp = np->payload;
        Free(np);
        return vp;
    }
	else {
		np = lp->head;
		lp->head = np->next;
		lp->head->prev = 0;
		vp = np->payload;
        Free(np);
		lp->n--;

		return vp;
	}
}

/*
 * ------------------------------------------------------------- 
 */

node_t  *
ll_first(ll_t * lp)
{
	return lp->head;
}

node_t  *
ll_next(ll_t * lp, node_t * np)
{
	if (np == lp->tail)
		return 0;
	else
		return np->next;
}

/*
 * ------------------------------------------------------------- 
 */

node_t  *
ll_find(ll_t * lp, void *pattern)
{
	node_t          *np;
    spocp_result_t  rc;
    
	/*
	 * no compare function, then nothing match 
	 */

	if (lp->cf == 0)
		return 0;

	for (np = lp->head; np; np = np->next) {
		if (lp->cf(np->payload, pattern, &rc) == 0)
            if (rc == SPOCP_SUCCESS)
                return np;
	}

	return 0;
}

void
ll_rm_link(ll_t * lp, node_t * np)
{
	node_t  *tmp;

    for (tmp = lp->head; tmp; tmp = tmp->next)
        if (tmp == np)
            break;

    if (tmp == 0) /* not part of this list */
        return;
        
	if (np == lp->head && np == lp->tail) {
		lp->head = lp->tail = 0;
	} else if (np == lp->head) {
		tmp = np->next;
		lp->head = tmp;
		tmp->prev = 0;
	} else if (np == lp->tail) {
		lp->tail = np->prev;
		np->prev->next = 0;
	} else {
		np->prev->next = np->next;
		np->next->prev = np->prev;
	}
    
    lp->n -= 1;
}

ll_t           *
ll_dup(ll_t * old)
{
	ll_t           *new;
	node_t         *on, *pn, *nn;
	void           *pl;

	if (old == 0)
		return 0;

	new = ll_new(old->cf, old->ff, old->df, old->pf);

	if (old->df)
		pl = old->df(old->head->payload);
	else
		pl = old->head->payload;

	new->head = pn = node_new(pl);

	new->n = 1;

	for (on = old->head->next; on != 0; on = on->next) {
		if (old->df)
			pl = old->df(on->payload);
		else
			pl = on->payload;

		nn = node_new(pl);
		pn->next = nn;
		pn = nn;
		new->n++;
	}

	new->tail = pn;

	return new;
}

/*
 * ---------------------------------------------------------- 
 */

int
ll_print(ll_t * ll)
{
	node_t         *np;

	if (ll->pf == 0)
		return -1;

	for (np = ll->head; np; np = np->next)
		ll->pf(np->payload);

	return 0;
}
