
/***************************************************************************
                          db0.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <string.h>

#include <macros.h>
#include <db0.h>
#include <struct.h>
#include <plugin.h>
#include <func.h>
#include <wrappers.h>
#include <spocp.h>
#include <sha1.h>
#include <rbtree.h>
#include <dback.h>

static junc_t *element_add(plugin_t * pl, junc_t * dvp, element_t * ep,
			    ruleinst_t * ri, int n);
junc_t         *rm_next(junc_t * ap, branch_t * bp);
char           *set_item_list(junc_t * dv);
junc_t         *atom_add(branch_t * bp, atom_t * ap);
junc_t         *extref_add(branch_t * bp, atom_t * ap);
junc_t         *list_end(junc_t * arr);
junc_t         *list_add(plugin_t * pl, branch_t * bp, list_t * lp,
			 ruleinst_t * ri);
junc_t         *range_add(branch_t * bp, range_t * rp);
junc_t         *prefix_add(branch_t * bp, atom_t * ap);
junc_t         *rule_close(junc_t * ap, ruleinst_t * ri);

static ruleinst_t *ruleinst_new(octet_t * rule, octet_t * blob, char *bcname);

ruleinst_t     *ruleinst_dup(ruleinst_t * ri);
void            ruleinst_free(ruleinst_t * ri);

/*
 * ------------------------------------------------------------ 
 */

char            item[NTYPES + 1];

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

junc_t         *
junc_new()
{
	junc_t         *ap;

	ap = (junc_t *) Calloc(1, sizeof(junc_t));

	return ap;
}

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

junc_t         *
branch_add(junc_t * ap, branch_t * bp)
{
	if (ap == 0)
		ap = junc_new();

	ap->item[bp->type] = bp;
	bp->parent = ap;

	return ap;
}

/************************************************************
*                                                   &P_match_uid        *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

junc_t         *
junc_dup(junc_t * jp, ruleinfo_t * ri)
{
	junc_t         *new;
	branch_t       *nbp, *obp;
	int             i, j;

	if (jp == 0)
		return 0;

	new = junc_new();

	for (i = 0; i < NTYPES; i++) {
		if (jp->item[i]) {
			nbp = new->item[i] =
			    (branch_t *) Calloc(1, sizeof(branch_t));
			obp = jp->item[i];

			nbp->type = obp->type;
			nbp->count = obp->count;
			nbp->parent = new;

			switch (i) {
			case SPOC_ATOM:
				nbp->val.atom = phash_dup(obp->val.atom, ri);
				break;

			case SPOC_LIST:
				nbp->val.list = junc_dup(obp->val.list, ri);
				break;

			case SPOC_PREFIX:
				nbp->val.prefix = ssn_dup(obp->val.prefix, ri);
				break;

			case SPOC_SUFFIX:
				nbp->val.suffix = ssn_dup(obp->val.suffix, ri);
				break;

			case SPOC_RANGE:
				for (j = 0; j < DATATYPES; j++) {
					nbp->val.range[j] =
					    sl_dup(obp->val.range[j], ri);
				}
				break;

			case SPOC_ENDOFLIST:
				nbp->val.list = junc_dup(obp->val.list, ri);
				break;

			case SPOC_ENDOFRULE:
				nbp->val.id = index_dup(obp->val.id, ri);
				break;

				/*
				 * case SPOC_REPEAT : break ; 
				 */

			}
		}
	}

	return new;
}

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

char           *
set_item_list(junc_t * dv)
{
	int             i;

	for (i = 0; i < NTYPES; i++) {
		if (dv && dv->item[i])
			item[i] = '1';
		else
			item[i] = '0';
	}

	item[NTYPES] = '\0';

	return item;
}

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

junc_t         *
atom_add(branch_t * bp, atom_t * ap)
{
	buck_t         *bucket = 0;

	if (bp->val.atom == 0)
		bp->val.atom = phash_new(3, 50);

	bucket = phash_insert(bp->val.atom, ap, ap->hash);
	bucket->refc++;

	DEBUG(SPOCP_DSTORE) traceLog("Atom \"%s\" [%d]", ap->val.val,
				     bucket->refc);

	if (bucket->next == 0)
		bucket->next = junc_new();

	return bucket->next;
}


/************************************************************/

static junc_t  *
any_add(branch_t * bp)
{
	if (bp->val.next == 0)
		bp->val.next = junc_new();

	return bp->val.next;
}

/************************************************************/

/*
 * static char *P_print_junc( item_t i ) { junc_print( 0, (junc_t *) i ) ;
 * 
 * return 0 ; }
 * 
 * static void P_free_junc( item_t i ) { junc_free(( junc_t *) i ) ; }
 * 
 * static item_t P_junc_dup( item_t i, item_t j ) { return junc_dup( ( junc_t
 * * ) i , ( ruleinfo_t *) j ) ; } 
 */


/************************************************************
*                                                           *
************************************************************/

varr_t         *
varr_ruleinst_add(varr_t * va, ruleinst_t * ju)
{
	return varr_add(va, (void *) ju);
}

ruleinst_t     *
varr_ruleinst_pop(varr_t * va)
{
	return (ruleinst_t *) varr_pop(va);
}

ruleinst_t     *
varr_ruleinst_nth(varr_t * va, int n)
{
	return (ruleinst_t *) varr_nth(va, n);
}

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

junc_t         *
list_end(junc_t * arr)
{
	branch_t       *db;

	db = ARRFIND(arr, SPOC_ENDOFLIST);

	if (db == 0) {
		db = (branch_t *) Calloc(1, sizeof(branch_t));
		db->type = SPOC_ENDOFLIST;
		db->count = 1;
		db->val.list = junc_new();
		branch_add(arr, db);
	} else {
		db->count++;
	}

	DEBUG(SPOCP_DSTORE) traceLog("List end [%d]", db->count);

	arr = db->val.list;

	return arr;
}

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

junc_t         *
list_add(plugin_t * pl, branch_t * bp, list_t * lp, ruleinst_t * ri)
{
	element_t      *elp;
	junc_t         *jp;

	if (bp->val.list == 0) {
		jp = bp->val.list = junc_new();
	} else
		jp = bp->val.list;

	elp = lp->head;

	/*
	 * fails, means I should clean up 
	 */
	if ((jp = element_add(pl, jp, elp, ri, 1)) == 0) {
		traceLog("List add failed");
		return 0;
	}

	return jp;
}

/************************************************************
*                                                           *
************************************************************/

static void
list_clean(junc_t * jp, list_t * lp)
{
	element_t      *ep = lp->head, *next;
	buck_t         *buck;
	branch_t       *bp;

	for (; ep; ep = next) {
		next = ep->next;
		if (jp == 0 || (bp = ARRFIND(jp, ep->type)) == 0)
			return;

		switch (ep->type) {
		case SPOC_ATOM:
			buck =
			    phash_search(bp->val.atom, ep->e.atom,
					 ep->e.atom->hash);
			jp = buck->next;
			buck->refc--;
			bp->count--;
			if (buck->refc == 0) {
				buck->val.val = 0;
				buck->hash = 0;
				junc_free(buck->next);
				buck->next = 0;
				next = 0;
			}
			break;

		case SPOC_LIST:
			bp->count--;
			if (bp->count == 0) {
				junc_free(bp->val.list);
				bp->val.list = 0;
			} else
				list_clean(bp->val.list, ep->e.list);
			break;
		}
	}
}

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

junc_t         *
range_add(branch_t * bp, range_t * rp)
{
	int             i = rp->lower.type & 0x07;
	junc_t         *jp;

	if (bp->val.range[i] == 0) {
		bp->val.range[i] = sl_init(4);
	}

	jp = sl_range_add(bp->val.range[i], rp);

	return jp;
}

/************************************************************
*            adds a prefix to a prefix branch               *
************************************************************/
/*
 * 
 * Arguments: bp pointer to a branch ap pointer to a atom
 * 
 * Returns: pointer to next node in the tree 
 */

junc_t         *
prefix_add(branch_t * bp, atom_t * ap)
{
	/*
	 * assumes that the string to add is '\0' terminated 
	 */
	return ssn_insert(&bp->val.prefix, ap->val.val, FORWARD);
}

/************************************************************
*            adds a suffix to a suffix branch               *
************************************************************/
/*
 * 
 * Arguments: bp pointer to a branch ap pointer to a atom
 * 
 * Returns: pointer to next node in the tree 
 */

static junc_t  *
suffix_add(branch_t * bp, atom_t * ap)
{
	/*
	 * assumes that the string to add is '\0' terminated 
	 */
	return ssn_insert(&bp->val.suffix, ap->val.val, BACKWARD);
}

/************************************************************
*      Is this the last element in a S-expression ?         *
************************************************************/
/*
 * assumes list as most basic unit
 * 
 * Arguments: ep element
 * 
 * Returns: 1 if it is the last element otherwise 0
 * 
 * static int last_element( element_t *ep ) { while( ep->next == 0 &&
 * ep->memberof != 0 ) ep = ep->memberof ;
 * 
 * if( ep->next == 0 && ep->memberof == 0 ) return 1 ; else return 0 ; } 
 */

/************************************************************
*           Adds end-of-rule marker to the tree             *
************************************************************/
/*
 * 
 * Arguments: ap node in the tree id ID for this rule
 * 
 * Returns: pointer to next node in the tree 
 */

static junc_t  *
rule_end(junc_t * ap, ruleinst_t * ri)
{
	branch_t       *bp;

	if ((bp = ARRFIND(ap, SPOC_ENDOFLIST)) == 0) {
		DEBUG(SPOCP_DSTORE) traceLog("New rule end");

		bp = (branch_t *) Calloc(1, sizeof(branch_t));
		bp->type = SPOC_ENDOFRULE;
		bp->count = 1;
		bp->val.id = index_add(bp->val.id, (void *) ri);

		ap = branch_add(ap, bp);
	} else {		/* If rules contains references this can
				 * happen, otherwise not */
		bp->count++;
		DEBUG(SPOCP_DSTORE) traceLog("Old rule end: count [%d]",
					     bp->count);
		index_add(bp->val.id, (void *) ri);
	}

	DEBUG(SPOCP_DSTORE) traceLog("done rule %s", ri->uid);

	return ap;
}

/************************************************************
*           Adds end-of-list marker to the tree             *
************************************************************/
/*
 * 
 * Arguments: ap node in the tree ep end of list element id ID for this rule
 * 
 * Returns: pointer to next node in the tree
 * 
 * Should mark in some way that end of rule has been reached 
 */

static junc_t  *
list_close(junc_t * ap, element_t * ep, ruleinst_t * ri, int *eor)
{
	element_t      *parent;

	do {
		ap = list_end(ap);

		parent = ep->memberof;

		DEBUG(SPOCP_DSTORE) {
			traceLog("end_list that started with \"%s\"",
				 parent->e.list->head->e.atom->val.val);
		}

		ep = parent;
	} while (ep->type == SPOC_LIST && ep->next == 0 && ep->memberof != 0);

	if (parent->memberof == 0) {
		rule_end(ap, ri);
		*eor = 1;
	} else
		*eor = 0;

	return ap;
}

static junc_t  *
add_next(plugin_t * plugin, junc_t * jp, element_t * ep, ruleinst_t * ri)
{
	int             eor = 0;

	if (ep->next)
		jp = element_add(plugin, jp, ep->next, ri, 1);
	else if (ep->memberof) {
		/*
		 * if( ep->memberof->type == SPOC_SET ) ; 
		 */
		if (ep->type != SPOC_LIST)	/* a list never closes itself */
			jp = list_close(jp, ep, ri, &eor);
	}

	return jp;
}

/************************************************************
*           Add a s-expression element                      *
************************************************************/
/*
 * The representation of the S-expression representing rules are as a tree. A
 * every node in the tree there are a couple of different types of branches
 * that can appear. This routine chooses which type it should be and then
 * adds this element to that branch.
 * 
 * Arguments: ap node in the tree ep element to store rt pointer to the
 * ruleinstance
 * 
 * Returns: pointer to the next node in the tree 
 */


junc_t         *
element_add(plugin_t * pl, junc_t * jp, element_t * ep, ruleinst_t * rt,
	    int next)
{
	branch_t       *bp = 0;
	junc_t         *ap = 0;
	int             n;
	varr_t         *va, *dsva;
	element_t      *elem;
	dset_t         *ds;
	void           *v;

	bp = ARRFIND(jp, ep->type);

	/*
	 * DEBUG( SPOCP_DSTORE ) traceLog("Items: %s", set_item_list( jp ) ) ; 
	 */

	if (bp == 0) {
		bp = (branch_t *) Calloc(1, sizeof(branch_t));
		bp->type = ep->type;
		bp->count = 1;
		ap = branch_add(jp, bp);
	} else {
		bp->count++;
	}

	DEBUG(SPOCP_DSTORE) traceLog("Branch [%d] [%d]", bp->type, bp->count);

	switch (ep->type) {
	case SPOC_ATOM:
		if ((ap = atom_add(bp, ep->e.atom)) && next)
			ap = add_next(pl, ap, ep, rt);
		break;

	case SPOC_PREFIX:
		if ((ap = prefix_add(bp, ep->e.atom)) && next)
			ap = add_next(pl, ap, ep, rt);
		break;

	case SPOC_SUFFIX:
		if ((ap = suffix_add(bp, ep->e.atom)) && next)
			ap = add_next(pl, ap, ep, rt);
		break;

	case SPOC_RANGE:
		if ((ap = range_add(bp, ep->e.range)) && next)
			ap = add_next(pl, ap, ep, rt);
		break;

	case SPOC_LIST:
		if ((ap = list_add(pl, bp, ep->e.list, rt)) && next)
			ap = add_next(pl, ap, ep, rt);
		break;

	case SPOC_ANY:
		if ((ap = any_add(bp)) && next)
			ap = add_next(pl, ap, ep, rt);
		break;

	case SPOC_SET:
		va = ep->e.set;
		n = varr_len(va);
		if (bp->val.set == 0)
			ds = bp->val.set =
			    (dset_t *) Calloc(1, sizeof(dset_t));
		else {
			for (ds = bp->val.set; ds->next; ds = ds->next);
			ds->next = (dset_t *) Calloc(1, sizeof(dset_t));
			ds = ds->next;
		}

		for (v = varr_first(va), dsva = ds->va; v;
		     v = varr_next(va, v)) {
			elem = (element_t *) v;
			if ((ap = element_add(pl, jp, elem, rt, 0)) == 0)
				break;
			dsva = varr_add(dsva, ap);
			ap = add_next(pl, ap, ep, rt);
		}
		ds->va = dsva;

		break;
	}

	if (ap == 0 && bp != 0) {
		bp->count--;

		if (bp->count == 0) {
			DEBUG(SPOCP_DSTORE) traceLog("Freeing type %d branch",
						     bp->type);
			branch_free(bp);
			jp->item[ep->type] = 0;
		}
	}

	return ap;
}

/************************************************************
                 RULE INFO functions 
 ************************************************************/

/*
 * --------------- ruleinst ----------------------------- 
 */

static ruleinst_t *
ruleinst_new(octet_t * rule, octet_t * blob, char *bcname)
{
	struct sha1_context ctx;
	ruleinst_t     *rip;
	unsigned char   sha1sum[21], *ucp;
	int             j;

	if (rule == 0 || rule->len == 0)
		return 0;

	rip = (ruleinst_t *) Calloc(1, sizeof(ruleinst_t));

	rip->rule = octdup(rule);

	if (blob && blob->len)
		rip->blob = octdup(blob);
	else
		rip->blob = 0;

	sha1_starts(&ctx);

	sha1_update(&ctx, (uint8 *) rule->val, rule->len);
	if (bcname)
		sha1_update(&ctx, (uint8 *) bcname, strlen(bcname));

	sha1_finish(&ctx, (unsigned char *) sha1sum);

	for (j = 0, ucp = (unsigned char *) rip->uid; j < 20; j++, ucp += 2)
		sprintf((char *) ucp, "%02x", sha1sum[j]);

	DEBUG(SPOCP_DSTORE) traceLog("New rule (%s)", rip->uid);

	rip->ep = 0;
	rip->alias = 0;

	return rip;
}

ruleinst_t     *
ruleinst_dup(ruleinst_t * ri)
{
	ruleinst_t     *new;

	if (ri == 0)
		return 0;

	if (ri->bcond)
		new = ruleinst_new(ri->rule, ri->blob, ri->bcond->name);
	else
		new = ruleinst_new(ri->rule, ri->blob, 0);

	new = (ruleinst_t *) Calloc(1, sizeof(ruleinst_t));

	strcpy(new->uid, ri->uid);
	new->rule = octdup(ri->rule);
	/*
	 * if( ri->ep ) new->ep = element_dup( ri->ep ) ; 
	 */
	if (ri->alias)
		new->alias = ll_dup(ri->alias);

	return new;
}

static item_t
P_ruleinst_dup(item_t i, item_t j)
{
	return (void *) ruleinst_dup((ruleinst_t *) i);
}

void
ruleinst_free(ruleinst_t * rt)
{
	if (rt) {
		if (rt->rule)
			oct_free(rt->rule);
		if (rt->blob)
			oct_free(rt->blob);
		if (rt->alias)
			ll_free(rt->alias);
		/*
		 * if( rt->ep ) element_rm( rt->ep ) ; 
		 */

		free(rt);
	}
}

/*
 * static int uid_match( ruleinst_t *rt, char *uid ) { * traceLog( "%s <> %s", 
 * rt->uid, uid ) ; *
 * 
 * return strcmp( rt->uid, uid ) ; }
 * 
 * static int P_match_uid( void *vp, void *pattern ) { return uid_match(
 * (ruleinst_t *) vp, (char *) pattern ) ; } 
 */

/*
 * --------- ruleinst functions ----------------------------- 
 */

static void
P_ruleinst_free(void *vp)
{
	ruleinst_free((ruleinst_t *) vp);
}

static int
P_ruleinst_cmp(void *a, void *b)
{
	ruleinst_t     *ra, *rb;

	if (a == 0 && b == 0)
		return 0;
	if (a == 0 || b == 0)
		return 1;

	ra = (ruleinst_t *) a;
	rb = (ruleinst_t *) b;

	return strcmp(ra->uid, rb->uid);
}

/*
 * PLACEHOLDER 
 */
static char    *
P_ruleinst_print(void *vp)
{
	return 0;
}

static Key
P_ruleinst_key(item_t i)
{
	ruleinst_t     *ri = (ruleinst_t *) i;

	return ri->uid;
}


/*
 * --------- ruleinfo ----------------------------- 
 */

ruleinfo_t     *
ruleinfo_new(void)
{
	return (ruleinfo_t *) Calloc(1, sizeof(ruleinfo_t));
}

ruleinfo_t     *
ruleinfo_dup(ruleinfo_t * ri)
{
	ruleinfo_t     *new;

	if (ri == 0)
		return 0;

	new = (ruleinfo_t *) Calloc(1, sizeof(ruleinfo_t));
	new->rules = rbt_dup(ri->rules, 1);

	return new;
}

void
ruleinfo_free(ruleinfo_t * ri)
{
	if (ri) {
		rbt_free(ri->rules);
		free(ri);
	}
}

ruleinst_t     *
ruleinst_find_by_uid(rbt_t * rules, char *uid)
{
	return (ruleinst_t *) rbt_search(rules, uid);
}

/*
 * ---------------------------------------- 
 */

/*
 * void rm_raci( ruleinst_t *rip, raci_t *ap ) { }
 * 
 */

int
free_rule(ruleinfo_t * ri, char *uid)
{
	return rbt_remove(ri->rules, uid);
}

void
free_all_rules(ruleinfo_t * ri)
{
	ruleinfo_free(ri);
}

static ruleinst_t *
save_rule(db_t * db, dbcmd_t * dbc, octet_t * rule, octet_t * blob,
	  char *bcondname)
{
	ruleinfo_t     *ri;
	ruleinst_t     *rt;

	if (db->ri == 0)
		db->ri = ri = ruleinfo_new();
	else
		ri = db->ri;

	rt = ruleinst_new(rule, blob, bcondname);

	if (ri->rules == 0)
		ri->rules =
		    rbt_init(&P_ruleinst_cmp, &P_ruleinst_free,
			     &P_ruleinst_key, &P_ruleinst_dup,
			     &P_ruleinst_print);
	else {
		if (ruleinst_find_by_uid(ri->rules, rt->uid) != 0) {
			ruleinst_free(rt);
			return 0;
		}
	}

	if (db->dback)
		dback_save(dbc, rt->uid, rule, blob, bcondname);

	rbt_insert(ri->rules, (item_t) rt);

	return rt;
}

int
nrules(ruleinfo_t * ri)
{
	if (ri == 0)
		return 0;
	else
		return rbt_items(ri->rules);
}

int
rules(db_t * db)
{
	if (db == 0 || db->ri == 0 || db->ri->rules == 0
	    || db->ri->rules->head == 0)
		return 0;
	else
		return 1;
}

ruleinst_t     *
get_rule(ruleinfo_t * ri, char *uid)
{
	if (ri == 0 || uid == 0)
		return 0;

	return ruleinst_find_by_uid(ri->rules, uid);
}

/*
 * creates an output string that looks like this
 * 
 * path ruleid rule [ blob ]
 * 
 */
octet_t	*
ruleinst_print(ruleinst_t * r, char *rs)
{
	octet_t        *oct;
	int             l, lr;
	char            flen[1024];


	if (rs) {
		lr = strlen(rs);
		snprintf(flen, 1024, "%d:%s", lr, rs);
	} else {
		strcpy(flen, "1:/");
		lr = 1;
	}

	l = r->rule->len + DIGITS(r->rule->len) + lr + DIGITS(lr) + 5 + 40 + 1;

	if (r->blob && r->blob->len) {
		l += r->blob->len + 1 + DIGITS(r->blob->len);
	}

	oct = oct_new(l, 0);

	octcat(oct, flen, strlen(flen));

	octcat(oct, "40:", 3);

	octcat(oct, r->uid, 40);

	snprintf(flen, 1024, "%d:", r->rule->len);
	octcat(oct, flen, strlen(flen));

	octcat(oct, r->rule->val, r->rule->len);

	if (r->blob && r->blob->len) {
		snprintf(flen, 1024, "%d:", r->blob->len);
		octcat(oct, flen, strlen(flen));
		octcat(oct, r->blob->val, r->blob->len);
	}

	return oct;
}

spocp_result_t
get_all_rules(db_t * db, octarr_t * oa, char *rs)
{
	int             i, n;
	ruleinst_t     *r;
	varr_t         *pa = 0;
	octet_t        *oct;
	spocp_result_t  rc = SPOCP_SUCCESS;

	n = nrules(db->ri);

	if (n == 0)
		return rc;

        /* resize if too small */
	if ((oa && (oa->size - oa->n) < n))
		octarr_mr(oa, n);

	pa = rbt2varr(db->ri->rules);

	for (i = 0; (r = (ruleinst_t *) varr_nth(pa, i)); i++) {

		/*
		if (1) {
			char	*str;
			traceLog("...") ;
			str = oct2strdup( r->rule, '%' );
			traceLog("Rule[%d]: %s", i, str );
			free(str);
		}
		*/

		if ((oct = ruleinst_print(r, rs)) == 0) {
			rc = SPOCP_OPERATIONSERROR;
			octarr_free(oa);
			break;
		}

		/*
		if (1) {
			char	*str;
			str = oct2strdup( oct, '%' );
			traceLog("Rule[%d] => %s", i, str );
			free(str);
		}
		*/

		oa = octarr_add(oa, oct);

		/*
		if (1)
			traceLog("Added to octarr") ;
		*/
	}

	/*
	 * dont't remove the items since I've only used pointers 
	 */
	varr_free(pa);

	return rc;
}

octet_t        *
get_blob(ruleinst_t * ri)
{

	if (ri == 0)
		return 0;
	else
		return ri->blob;
}

db_t           *
db_new()
{
	db_t           *db;

	db = (db_t *) Calloc(1, sizeof(db_t));
	db->jp = junc_new();
	db->ri = ruleinfo_new();

	return db;
}

/************************************************************
*    Add a rightdescription to the rules database           *
************************************************************/
/*
 * 
 * Arguments: db Pointer to the incore representation of the rules database ep 
 * Pointer to a parsed S-expression rt Pointer to the rule instance
 * 
 * Returns: TRUE if OK 
 */

static spocp_result_t
store_right(db_t * db, element_t * ep, ruleinst_t * rt)
{
	int             r;

	if (db->jp == 0)
		db->jp = junc_new();

	if (element_add(db->plugins, db->jp, ep, rt, 1) == 0)
		r = SPOCP_OPERATIONSERROR;
	else
		r = SPOCP_SUCCESS;

	return r;
}

/************************************************************
*    Store a rights description in the rules database           *
************************************************************/
/*
 * 
 * Arguments: db Pointer to the incore representation of the rules database
 * 
 * Returns: TRUE if OK 
 */

spocp_result_t
add_right(db_t ** db, dbcmd_t * dbc, octarr_t * oa, ruleinst_t ** ri,
	  bcdef_t * bcd)
{
	element_t      *ep;
	octet_t         rule, blob, oct;
	ruleinst_t     *rt;
	spocp_result_t  rc = SPOCP_SUCCESS;

	/*
	 * LOG( SPOCP_WARNING ) traceLog("Adding new rule: \"%s\"", rp->val) ; 
	 */

	rule.len = rule.size = 0;
	rule.val = 0;

	blob.len = blob.size = 0;
	blob.val = 0;

	octln(&oct, oa->arr[0]);
	octln(&rule, oa->arr[0]);

	if (oa->n > 1) {
		octln(&blob, oa->arr[1]);
	} else
		blob.len = 0;

	if ((rc = element_get(&oct, &ep)) == SPOCP_SUCCESS) {

		/*
		 * stuff left ?? 
		 */
		/*
		 * just ignore it 
		 */
		if (oct.len) {
			rule.len -= oct.len;
		}

		if ((rt =
		     save_rule(*db, dbc, &rule, &blob,
			       bcd ? bcd->name : NULL)) == 0) {
			element_free(ep);
			return SPOCP_EXISTS;
		}

		/*
		 * right == rule 
		 */
		if ((rc = store_right(*db, ep, rt)) != SPOCP_SUCCESS) {
			element_free(ep);

			/*
			 * remove ruleinstance 
			 */
			free_rule((*db)->ri, rt->uid);

			LOG(SPOCP_WARNING) traceLog("Error while adding rule");
		}

		if (bcd) {
			rt->bcond = bcd;
			bcd->rules = varr_add(bcd->rules, (void *) rt);
		}

		*ri = rt;
	}

	return rc;
}

/*
 * static int ruleinst_print( ruleinst_t *ri ) { char *str ;
 * 
 * if( ri == 0 ) return 0 ; traceLog( "uid: %s", ri->uid ) ;
 * 
 * str = oct2strdup( ri->rule, '%' ) ; traceLog( "rule: %s", str ) ;
 * 
 * if( ri->blob ) { str = oct2strdup( ri->blob, '%' ) ; traceLog( "rule: %s",
 * str ) ; }
 * 
 * if( ri->alias ) ll_print( ri->alias ) ;
 * 
 * return 0 ; } 
 */

int
ruleinfo_print(ruleinfo_t * r)
{
	if (r == 0)
		return 0;

	rbt_print(r->rules);

	return 0;
}
