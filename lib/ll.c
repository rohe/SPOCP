
/***************************************************************************
                             ll.c  -  description
                             -------------------

    begin                : Fri Feb 6 2003
    copyright            : (C) 2003 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <stdlib.h>

#include <struct.h>
#include <wrappers.h>
#include <func.h>

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
			free(np);
		}
		free(lp);
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
				traceLog
				    ("Attempting to add already present rule");
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
	else {
		np = lp->head;
		lp->head = np->next;
		lp->head->prev = 0;
		vp = np->payload;
		free(np);
		lp->n--;

		return vp;
	}
}

/*
 * ------------------------------------------------------------- 
 */

node_t         *
ll_first(ll_t * lp)
{
	return lp->head;
}

node_t         *
ll_next(ll_t * lp, node_t * np)
{
	if (np == lp->tail)
		return 0;
	else
		return np->next;
}

void           *
ll_first_p(ll_t * lp)
{
	return lp->head;
}

void           *
ll_next_p(ll_t * lp, void *prev)
{
	node_t         *np;

	for (np = lp->head; np; np = np->next) {
		if (np->payload == prev)
			return np->next;
	}

	return 0;
}

/*
 * ------------------------------------------------------------- 
 */

void           *
ll_find(ll_t * lp, void *pattern)
{
	node_t         *np;

	/*
	 * no compare function, then nothing match 
	 */

	if (lp->cf == 0)
		return 0;

	for (np = lp->head; np; np = np->next) {
		if (lp->cf(np->payload, pattern) == 0)
			return np->payload;
	}

	return 0;
}

void
ll_rm_link(ll_t * lp, node_t * np)
{
	node_t         *tmp;

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
}

/*
 * if all != 0, remove all instances that match otherwise just the first one 
 */

int
ll_rm_item(ll_t * lp, void *pattern, int all)
{
	int             n = 0;
	node_t         *np, *next;

	/*
	 * no compare function == no matches 
	 */
	if (lp->cf == 0)
		return 0;

	for (np = lp->head; np;) {
		if (lp->cf(np->payload, pattern) == 0) {

			next = np->next;

			ll_rm_link(lp, np);

			lp->ff(np->payload);
			free(np);

			lp->n--;
			n++;

			if (!all)
				break;

			np = next;
		} else
			np = np->next;
	}

	return n;
}

void
ll_rm_payload(ll_t * lp, void *pl)
{
	node_t         *np;

	for (np = lp->head; np;) {
		if (np->payload == pl) {
			ll_rm_link(lp, np);
			break;
		}
	}
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
		pl = old->df(old->head->payload, 0);
	else
		pl = old->head->payload;

	new->head = pn = node_new(pl);

	new->n = 1;

	for (on = old->head->next; on != 0; on = on->next) {
		if (old->df)
			pl = old->df(on->payload, 0);
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

ll_t           *
parr2ll(parr_t * pp)
{
	ll_t           *ll = 0;
	int             i;

	for (i = 0; i < pp->n; i++)
		ll = ll_push(ll, pp->vect[i], 0);

	return ll;
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
