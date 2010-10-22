
/***************************************************************************
                          slist.c  -  skip-list 
                             -------------------
    begin                : Sat Oct 24 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/


#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#include <element.h>
#include <slist.h>
#include <varr.h>
#include <boundary.h>
#include <branch.h>

#include <log.h>
#include <wrappers.h>
#include <macros.h>

/* #define AVLUS 1 */

#ifdef AVLUS
static void slnode_print( slnode_t *s );
#endif

/*
 * An implementation of a skip-list 
 */
/*
 * In skip lists, extra links are used to skip nodes in a list while searching
 * 
 * [ ]----------->[ ]----------->[ ] 
 * [ ]------>[ ]->[ ]->[ ]------>[ ] 
 * [ ]->[ ]->[ ]->[ ]->[ ]->[ ]->[ ]
 * 
 * The number of nodes skipped is variable, though at each level at least as
 * many as the largest one of the immediate lower level 
 */

/************************************************************************/

slnode_t       *
sl_new(boundary_t *value, int n, slnode_t *tail, int width)
{
	slnode_t       *sln;
	int             i;

	sln = (slnode_t *) Calloc(1, sizeof(slnode_t));

	/*
	 * set the number of levels for this node 
	 */
	if (n) {
		sln->sz = n;
		sln->next = (slnode_t **) Calloc(n, sizeof(slnode_t *));
        if (tail) {
            for( i = 0; i < n; i++)
                sln->next[i] = tail;
        }
		sln->width = (int *) Calloc(n, sizeof(int));
        if (width){
            for (i = 0; i < n; i++) 
                sln->width[i] = width;
        }
	} else {
		sln->next = 0;
        sln->width = 0;
		sln->sz = 0;
	}

	sln->value = value;	/* the data */
	sln->refc = 0;      /* reference counter to know if I should still 
                         * keep it around. If a request to free it is
                         * issued and refc == 0 then it is removed */
	sln->varr = 0;		/* joins the boundaries of a range */

	return sln;
}

/************************************************************************/

/* have to coordinate the freeing of junc_t structs since they 
 * appears on both side
 */
void
sl_free_node(slnode_t * sp, slnode_t *tail)
{
    int         i;
    junc_t      *jp;
    slnode_t    **sln;

	if (sp) {
		if (sp->item)
			boundary_free(sp->item);

		if (sp->next) {
            for (i=0, sln=sp->next; i<sp->sz; i++, sln++) {
                if (*sln && (*sln != tail))
                    sl_free_node(*sln, tail);
            }
			Free(sp->next);
        }
        if (sp->varr) {
            while ((jp = varr_junc_pop(sp->varr))) {
                junc_free(jp);
            }

            varr_free(sp->varr);
        }
        
		Free(sp);
	}
}

/************************************************************************
 Initializing the list, starts of with only a head and a tail, representing
 the absolute minumum and maximum values in a range 
 ************************************************************************/

slist_t        *
sl_init(int max)
{
	slist_t        *slp;

	slp = (slist_t *) Malloc(sizeof(slist_t));

	slp->size = 0;          /* size of the list */
	slp->maxlev = max;	/* max number of levels */
	slp->type = 0;          /* type of data: numeric, alpha, ... */

	slp->tail = sl_new(0, 0, 0, 0);
	slp->head = sl_new(0, max + 1, slp->tail, 1);

	return slp;
}

/************************************************************************/

void
sl_free_slist(slist_t * slp)
{
	slnode_t    *tail;

	if (slp == 0)
		return;

    /* printf("[sl_free_slist] head:%p\n", slp->head); */
    tail = slp->tail;
    sl_free_node(slp->head, tail);
    sl_free_node(tail, 0);
    
}

/************************************************************************/

/*
 * Find the number of links 
 */
/*
 * the probability of getting a j-link node should be 1/(2**j) 
 */
int
sl_rand(slist_t * slp)
{
	int     j, r;    
	int     randmax = RAND_MAX, m;

	if (slp == 0)
		return 0;

    srand(1);
    r = rand();

    m = randmax/2;
	for (j = 1; j ; j++) {
        /*printf("m:%d r:%d\n",m,r);*/
		if (r < m || j == slp->lgnmax)
			break;
        r -= m;
        m = m/2;
	}

    if (j > slp->lgn) {
        slp->lgn = j;
    }
    
	return j;
}

/************************************************************************/

/* find where to insert the new value in the sorted list.
 * lowest values at the start of the list.
 */
void
sl_rec_insert(slnode_t * old, slnode_t * new, int n)
{
	int             r;

	if ((r = boundary_xcmp(new->item, old->next[n]->item)) < 0) {
		if (n < new->sz) {
			new->next[n] = old->next[n];
			old->next[n] = new;
		}

		if (n == 0)
			return;

		sl_rec_insert(old, new, n - 1);

		return;
	} else
		sl_rec_insert(old->next[n], new, n);
}

/************************************************************************/

slnode_t       *
sl_rec_delete(slnode_t * node, boundary_t * item, int n)
{
	slnode_t       *next = node->next[n], *ret = 0;
	int             r;

	if ((r = boundary_xcmp(next->item, item)) >= 0) {
		if (r == 0) {
			if (!ret) {
				next->refc--;
				ret = next;
			}

			if (next->refc == 0)
				node->next[n] = next->next[n];
		}

		if (n == 0)
			return ret;

		return sl_rec_delete(node, item, n - 1);
	}

	return sl_rec_delete(next, item, n);
}

/************************************************************************/

slnode_t       *
sl_delete(slist_t * slp, boundary_t * item)
{
	return sl_rec_delete(slp->head, item, slp->lgn);
}

/************************************************************************ 
 This is the actually search routine, starting at the top level and 
 working downwards.
 ************************************************************************/

slnode_t       *
sl_rec_find(slnode_t * node, boundary_t * item, int n, int *flag)
{
    char *bstr ;
    bstr = boundary_prints( item ) ; 
    /* printf("boundary: %s\n",bstr); */
    Free(bstr);
    
#ifdef AVLUS
	DEBUG( SPOCP_DMATCH) {
		traceLog(LOG_DEBUG, "sl_rec_find" );
		slnode_print( node);
	}
#endif

    /* printf("node->sz\n"); */
	if (node->sz == 0)
		return 0;	/* end of list */

    /* printf("boundary_xcmp(%p, %p)\n", item, node->item); */
	if (boundary_xcmp(item, node->item) == 0) {
		*flag = 0;
		return node;	/* The simple case */
	}

    /* printf("boundary_xcmp(item, node->next[n]->item)\n"); */
	if (boundary_xcmp(item, node->next[n]->item) < 0) {
		if (n == 0) {	/* reached the bottom */
#ifdef AVLUS
			DEBUG( SPOCP_DMATCH )
				traceLog(LOG_DEBUG, "Reached bottom");
#endif

			*flag = 1;
			return node->next[0];	/* So what's returned is the
						 * next bigger */
		}

		return sl_rec_find(node, item, n - 1, flag);
	} else
		return sl_rec_find(node->next[n], item, n, flag);

}

/************************************************************************/

slnode_t       *
sl_find(slist_t * slp, boundary_t * item)
{
	slnode_t       *node;
	int             flag = 0;

	node = sl_rec_find(slp->head, item, slp->lgn, &flag);

	if (flag)
		return 0;
	else
		return node;
}

/************************************************************************/

#ifdef AVLUS
static void
slnode_print( slnode_t *s )
{
	char *str ;

	if ( s->item ) {
		str = boundary_prints( s->item );
		traceLog(LOG_DEBUG,"%s sz=%d", str, s->sz );
	}
	else {
		traceLog(LOG_DEBUG,"top or bottom sz=%d", s->sz );
	}

}
#endif


void
sl_print(slist_t * slp)
{
	slnode_t       *sn;
	int             i;
	char 		*s;

	if (slp == 0)
		return;

	for (sn = slp->head, i = 0; sn->next ; sn = sn->next[0], i++) {
		s = boundary_prints(sn->item);
		traceLog(LOG_DEBUG,"%p, %d: %s", sn, i,s );
		Free( s );
	}
	s = boundary_prints(sn->item);
	traceLog(LOG_DEBUG,"%p, %d: %s", sn, i,s );
	Free( s );
}

/************************************************************************/

varr_t         *
sl_match(slist_t * slp, boundary_t * item)
{
	slnode_t       *node, *nc;
	int             flag = 0;
	varr_t         *lp = 0, *up = 0, *res;

	if (slp == 0) {
		traceLog(LOG_DEBUG,"Empty range ?");
		return 0;
	}

#ifdef AVLUS
	DEBUG(SPOCP_DMATCH) sl_print(slp);
#endif

	node = sl_rec_find(slp->head, item, slp->lgn, &flag);

#ifdef AVLUS
	DEBUG( SPOCP_DMATCH) {
		traceLog(LOG_DEBUG,"sl_rec_find returned flag:%d, %p",
			flag,node);
		if (node != NULL)
			slnode_print( node );
	}
#endif
	/*
	 * found a exakt match 
	 */
	if (flag == 0) {
		/* collect all below */
		for (nc = slp->head; nc != node->next[0]; nc = nc->next[0])
			lp = varr_or(lp, nc->varr, 1);

		if (node->item->type & EQ)
			nc = node;

		for (;; nc = nc->next[0]) {
			up = varr_or(up, nc->varr, 1);
			if (nc->item == 0)
				break;
		}
	} else { /* what I have is a pointer to a larger value */
		if (node == slp->head) {
			lp = varr_or(lp, node->varr, 1 );
			nc = slp->head->next[0];
		}
		else {
			for (nc = slp->head; nc != node; nc = nc->next[0])
				lp = varr_or(lp, nc->varr, 1);
		}

		for (;; nc = nc->next[0]) {
			up = varr_or(up, nc->varr, 1);
			if (nc->item == 0)
				break;
		}
	}

	/* sieve out all the ones that are both below and above */
#ifdef AVLUS
	DEBUG(SPOCP_DMATCH) {
		varr_print(lp, NULL);
		varr_print(up, NULL);
	}
#endif

	res = varr_and(lp, up);
	varr_free(lp);
	varr_free(up);

#ifdef AVLUS
	DEBUG(SPOCP_DMATCH) { 
		traceLog(LOG_DEBUG,"RES:");
		varr_print(res, NULL);
	}
#endif

	return res;
}

/************************************************************************/

slnode_t       *
sl_insert(slist_t * slp, boundary_t * item)
{
	slnode_t       *node;

	/*
	 * is the value already in there ? 
	 */
	if ((node = sl_find(slp, item)) == 0) {

		/*
		 * If not create a new node and slip it in 
		 */
		node = sl_new(item, sl_rand(slp), slp->tail);

		sl_rec_insert(slp->head, node, slp->lgn);
	}

	node->refc++;

	return node;
}

/************************************************************************/

varr_t         *
sl_range_match(slist_t * slp, range_t * rp)
{
	slnode_t       *low, *high, *nc;
	int             lflag = 0, hflag = 0;
	varr_t         *lp = 0, *up = 0, *res;

	if ((rp->lower.type & 0xF0) == 0) {	/* no value */
		low = slp->head;
		/*
		 * lflag = 0 ; implicit 
		 */
	} else
		low = sl_rec_find(slp->head, &rp->lower, slp->lgn, &lflag);

	if ((rp->upper.type & 0xF0) == 0) {	/* no value */
		high = slp->tail;
		/*
		 * hflag = 0 ; implicit 
		 */
	} else
		high = sl_rec_find(slp->head, &rp->upper, slp->lgn, &hflag);


	/*
	 * get every range the includes the present range 
	 */
	/*
	 * head --> low high ---->tail 
	 */

	if (lflag == 0) {
		for (nc = slp->head; nc != low->next[0]; nc = nc->next[0])
			lp = varr_or(lp, nc->varr, 1);
	} else {
		for (nc = slp->head; nc != low; nc = nc->next[0])
			lp = varr_or(lp, nc->varr, 1);
	}

	for (nc = high;; nc = nc->next[0]) {
		up = varr_or(up, nc->varr, 1);
		if (nc->item == 0)
			break;
	}

	res = varr_and(lp, up);
	varr_free(lp);
	varr_free(up);

	return res;
}

/************************************************************************/

void         *
sl_add_range(slist_t * slp, range_t * rp)
{
	slnode_t       *low, *high;
	junc_t         *jp;

    /* printf("[[sl_add_range]] %p\n", rp); */
    
#ifdef AVLUS
	DEBUG(SPOCP_DMATCH) {
		char *l,*u ;

		l = boundary_prints(&rp->lower);
		u = boundary_prints(&rp->upper);
		traceLog(LOG_DEBUG,"upper: %s, lower: %s", u,l );
		Free(u);
		Free(l);
	}
#endif
    /* printf("--\n"); */
    
    /* printf("[sl_add_range] lower type: %d\n",rp->lower.type); */
	if ((rp->lower.type & 0xF0) == 0) {	/* no value */
		low = slp->head;
		low->refc++;
	} else
		low = sl_insert(slp, &rp->lower);

    /* printf("[sl_add_range] upper type: %d\n",rp->upper.type & 0xF0); */
	if ((rp->upper.type & 0xF0) == 0) {
		high = slp->tail;
		high->refc++;
	} else
		high = sl_insert(slp, &rp->upper);

    /* printf("===\n"); */
	/*
	 * If there is no common junction add a new one 
	 */
	if ((jp = varr_junc_common(low->varr, high->varr)) == 0) {
		jp = junc_new();
        jp->refc = 1;
		low->varr = varr_junc_add(low->varr, jp);
		high->varr = varr_junc_add(high->varr, jp);
	}

	return jp;
}

/************************************************************************/

void         *
sl_range_rm(slist_t * slp, range_t * rp, int *rc)
{
	junc_t         *jp = 0;
	slnode_t       *low, *upp;
	int             i;

	if ((rp->lower.type & 0xF0) == 0) {	/* no value */
		low = slp->head;
		low->refc--;
	} else
		low = sl_delete(slp, &rp->lower);


	if ((rp->upper.type & 0xF0) == 0) {
		upp = slp->tail;
		upp->refc--;
	} else
		upp = sl_delete(slp, &rp->upper);

	/*
	 * there should never be more than one in common 
	 */
	jp = (junc_t *) varr_first_common(low->varr, upp->varr);
	if (jp) {
		jp = varr_junc_rm(low->varr, jp);
		jp = varr_junc_rm(upp->varr, jp);

	}

	if (low->varr->n == 0 && low != slp->head)
		sl_free_node(low, slp->tail);

	if (upp->varr->n == 0 && upp != slp->tail)
		sl_free_node(upp, slp->tail);

	/*
	 * if head is pointing to tail and both are down to 0 in ref count I
	 * could even remove the list, but why should I ? 
	 */

	for (i = 0; i < slp->lgnmax; i++)
		if (slp->head->next[i] != slp->tail)
			break;

	if (i == slp->lgnmax && slp->head->refc == 0)
		*rc = 1;
	else
		*rc = 0;

	return jp;
}

varr_t         *
get_all_range_followers(branch_t * bp, varr_t * ja)
{
	slnode_t       *node;
	slist_t       **slp = bp->val.range;
	int             i;

	for (i = 0; i < DATATYPES; i++) {
		if (slp[i] == 0)
			continue;

		node = slp[i]->head;
		if (node == 0)
			continue;

		if (node->varr)
			ja = varr_or(ja, node->varr, 1);

		do {
			node = node->next[0];
			if (node->varr)
				ja = varr_or(ja, node->varr, 1);
		} while (node != slp[i]->tail);
	}

	return ja;
}

varr_t         *
sl_range_lte_match(slist_t * slp, range_t * rp, varr_t * pa)
{
	slnode_t       *low, *high, *nc, *end;
	int             lflag = 0, hflag = 0;
	varr_t         *res = 0;

	if ((rp->lower.type & 0xF0) == 0) {	/* no value */
		low = slp->head;
		/*
		 * lflag = 0 ; implicit 
		 */
	} else
		low = sl_rec_find(slp->head, &rp->lower, slp->lgn, &lflag);

	if ((rp->upper.type & 0xF0) == 0) {	/* no value */
		high = slp->tail;
		/*
		 * hflag = 0 ; implicit 
		 */
	} else
		high = sl_rec_find(slp->head, &rp->upper, slp->lgn, &hflag);


	/*
	 * get every range that is included in the given range 
	 */

	nc = low;

	if (hflag)
		end = high;
	else {
		if (high->next)
			end = high->next[0];	/* If there is one */
		else
			end = 0;	/* have to break on other info */
	}

	for (nc = low; nc != end; nc = nc->next[0]) {
		res = varr_or(res, nc->varr, 1);
		if (nc->next == 0)
			break;	/* reached the end node */
	}

	if (res)
		pa = varr_or(pa, res, 1);

	return pa;
}

slnode_t       *
sln_dup(slnode_t * old)
{
	slnode_t       *new;

	if (old == 0)
		return 0;

	new = (slnode_t *) Calloc(1, sizeof(slnode_t));

	if (old->sz) {
		new->next = (slnode_t **) Calloc(old->sz, sizeof(slnode_t *));
		new->sz = old->sz;

		/*
		 * for( i = 0 ; i < old->sz ; i++ ) sln->next[i] = 0 ; 
		 */
	} else {
		new->next = 0;
		new->sz = 0;
	}

	new->sz = old->sz;
	new->refc = old->refc;
	new->item = boundary_dup(old->item);

	return new;
}

slist_t        *
sl_dup(slist_t * old, ruleinfo_t * ri)
{
	slist_t        *new = 0;
	slnode_t       *slp, *nc = 0, *pc = 0, *oc, *bc;
	int             i, j;
	junc_t         *jp;
	void           *vp;

	if (old == 0)
		return 0;

	new = (slist_t *) Malloc(sizeof(slist_t));

	new->lgn = old->lgn;	/* number of levels at this node */
	new->lgnmax = old->lgnmax;	/* max number of levels */
	new->type = old->type;	/* type of data: numeric, alpha, ... */
	new->n = old->n;

	/*
	 * first all the nodes up until the tail 
	 */
	for (oc = old->head; oc->next != 0; oc = oc->next[0]) {
		if (nc) {
			nc->next[0] = sln_dup(oc);
			nc = nc->next[0];
		} else {
			nc = sln_dup(oc);
			new->head = nc;
		}
	}

	/*
	 * and the tail 
	 */
	nc->next[0] = sln_dup(oc);
	new->tail = nc->next[0];

	/*
	 * then connect them on all levels 
	 */

	/*
	 * first the default, the tail 
	 */
	for (nc = new->head; nc->next; nc = nc->next[0]) {
		for (i = 1; i < nc->sz; i++)
			nc->next[i] = new->tail;
	}

	for (i = 1; i < old->lgnmax; i++) {
		nc = new->head;
		oc = old->head;

		while (oc != old->tail) {
			pc = oc->next[i];
			for (slp = oc->next[0], j = 1; slp != pc;
			     slp = slp->next[0])
				j++;
			oc = pc;
			for (bc = nc; j > 0; j--, bc = bc->next[0]);
			nc->next[i] = bc;
			nc = bc;
		}
	}

	/*
	 * and then the junctions, this is time consuming 
	 */

	for (oc = old->head, nc = new->head; oc->next;
	     nc = nc->next[0], oc = oc->next[0]) {
		for (bc = nc->next[0], pc = oc->next[0]; pc->next;
		     pc = pc->next[0], bc = bc->next[0]) {
			/*
			 * there should never be more than one common 
			 */
			if ((vp = varr_first_common(oc->varr, pc->varr)) != 0) {

				jp = junc_dup((junc_t *) vp, ri);

				nc->varr = varr_junc_add(nc->varr, jp);

				bc->varr = varr_junc_add(bc->varr, jp);
			}
		}
		/*
		 * there should never be more than one common 
		 */
		if ((vp = varr_first_common(oc->varr, pc->varr)) != 0) {

			jp = junc_dup((junc_t *) vp, ri);

			nc->varr = varr_junc_add(nc->varr, jp);

			bc->varr = varr_junc_add(bc->varr, jp);
		}
	}

	return new;
}
