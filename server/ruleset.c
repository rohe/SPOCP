
/***************************************************************************
                                ruleset.c 
                             -------------------

    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/


#include "locl.h"
RCSID("$Id$");

ruleset_t      *one_level(octet_t * name, ruleset_t * rs);
octarr_t       *path_split(octet_t * oct, int *pathlen);

/*
#define AVLUS 1
*/

/*
 * ---------------------------------------------------------------------- 
 */

octarr_t       *
path_split(octet_t * oct, int *pathlen)
{
	char           *sp, *ep;
	octarr_t       *res;
	octet_t        *o, tmp;

	octln(&tmp, oct);

	while (*tmp.val == '/')
		tmp.val++, tmp.len--;

	for (sp = tmp.val; *sp == '/' || DIRCHAR(*sp); sp++);

	tmp.len = sp - tmp.val;

	*pathlen = sp - oct->val;

	if (tmp.len == 0)
		return 0;

	o = octdup(&tmp);

	ep = o->val + o->len;
	res = octarr_new(4);

	for (sp = o->val; sp < ep; sp++) {
		if (*sp == '/') {
			o->len = sp - o->val;
			octarr_add(res, o);

			while (*(sp + 1) == '/')
				sp++;
			if (sp + 1 == ep) {
				o = 0;
				break;
			}

			o = oct_new(0, 0);
			o->val = sp + 1;
		}
	}

	if (o) {
		o->len = sp - o->val;
		octarr_add(res, o);
	}

	return res;
}

/*
 * ---------------------------------------------------------------------- 
 */

ruleset_t      *
ruleset_new(octet_t * name)
{
	ruleset_t      *rs;

	rs = (ruleset_t *) Calloc(1, sizeof(ruleset_t));

	if (name == 0 || name->len == 0) {
		rs->name = (char *) Malloc(1);
		*rs->name = '\0';
	} else
		rs->name = oct2strdup(name, 0);

	LOG(SPOCP_DEBUG) traceLog(LOG_DEBUG,"New ruleset: %s", rs->name);

	return rs;
}

/*
 * ---------------------------------------------------------------------- 
 */

void
ruleset_tree( ruleset_t *rs, int level )
{
	if (rs) {
		traceLog(LOG_INFO, "(%d) Name: %s", level, rs->name);
		if (rs->left)
			ruleset_tree( rs->left, level );
		if (rs->right)
			ruleset_tree( rs->right, level );
		if (rs->down)
			ruleset_tree( rs->down, level+1 );
	}
}

void
ruleset_free(ruleset_t * rs)
{
	if (rs) {
		if (rs->name)
			Free(rs->name);

		if (rs->down)
			ruleset_free(rs->down);
		if (rs->left)
			ruleset_free(rs->left);
		if (rs->right)
			ruleset_free(rs->right);

		if (rs->db) 
			db_free( rs->db );

		Free(rs);
	}
}

/*
 * ---------------------------------------------------------------------- 
 */

/*
 * static int next_part( octet_t *a, octet_t *b ) { if( a->len == 0 ) return 0 
 * ;
 * 
 * octln( b, a ) ;
 * 
 * for( ; b->len && DIRCHAR(*b->val) && *b->val != '/' ; b->val++, b->len-- )
 * ;
 * 
 * if( b->len && ( !DIRCHAR( *b->val ) && *b->val != '/' )) return 0 ;
 * 
 * a->len -= b->len ;
 * 
 * return 1 ; } 
 */
/*
 * ---------------------------------------------------------------------- 
 */

/*
 * Search through all the rulesets on this level to see if this one
 * already exists
 */
ruleset_t      *
one_level(octet_t * name, ruleset_t * rs)
{
	int             c;

	if (rs == 0)
		return 0;

#ifdef AVLUS
	traceLog(LOG_DEBUG,"one_level: [%s](%d),[%s]", name->val,
		 name->len, rs->name);
#endif
	c = oct2strcmp(name, rs->name);

	if (c < 0) {
		while( c < 0 ) {
			if (!rs->left)
				break;
			rs = rs->left;
			c = oct2strcmp(name, rs->name);
		}
	}
	else {
		while( c > 0 ) {
			if (!rs->right)
				break;
			rs = rs->right;
			c = oct2strcmp(name, rs->name);
		}
	}

	if (c == 0)
		return rs;
	else
		return 0;
}

/*
 * ---------------------------------------------------------------------- 
 */

/*
 * returns pointer to the ruleset if it exists otherwise NULL
 */
ruleset_t *
ruleset_find(octet_t * name, ruleset_t * rs)
{
	octet_t         loc;
	ruleset_t      *r, *nr = 0;
	octarr_t       *oa = 0;
	int             i, pathlen;

	/*
	{
		char *tmp;
		tmp = oct2strdup( name, '%' );
		traceLog(LOG_INFO, "Find ruleset %s", tmp );
		Free(tmp);
	}
	*/

	if (rs == 0) {
		traceLog(LOG_INFO,"ruleset_find() no starting point");
		return 0;
	}

	/*
	 * the root 
	 */
	if (name == 0 || name->len == 0
	    || (*name->val == '/' && name->len == 1)) {
		for (r = rs; r->up; r = r->up);
		return r;
	}
	/*
	 * server part 
	 */
	else if (name->len >= 2 && *name->val == '/'
		 && *(name->val + 1) == '/') {
		for (r = rs; r->up; r = r->up);

		loc.val = "//";
		loc.len = 2;
		if ((nr = one_level(&loc, r)) == 0) {
			return NULL;
		}

		if (nr && name->len == 2) {
			name->val += 2;
			name->len -= 2;
			return nr;
		}

		loc.val = name->val + 2;
		loc.len = name->len - 2;
		loc.size = 0;
	}
	/*
	 * absolute path 
	 */
	else if (*name->val == '/') {
		/*traceLog(LOG_INFO,"Absolute path");*/
		for (nr = rs; nr->up; nr = nr->up);
		octln(&loc, name);
		loc.val++;
		loc.len--;
	} else 
		return NULL;	/* don't do relative */

	oa = path_split(&loc, &pathlen);
		/*
	if (oa)
		traceLog(LOG_INFO,"%d levels in path", oa->n );
		*/

	for (i = 0; oa && i < oa->n; i++) {
		r = nr->down;
		if (r == 0) {
			octarr_free(oa);
			return NULL;
		}

		if ((nr = one_level(oa->arr[i], r)) == 0)
			break;
	}
	octarr_free(oa);

	if (nr == 0) 
		return NULL;

	name->val = loc.val + pathlen;
	name->len = loc.len - pathlen;

	/* traceLog(LOG_INFO,"Found \"%s\"", nr->name);*/ 

	return nr;
}

/*
 * ---------------------------------------------------------------------- 
 */

static ruleset_t *
add_to_level(octet_t * name, ruleset_t * rs)
{
	int             c;
	ruleset_t      *new;

	new = ruleset_new(name);

	if (rs == 0)
		return new;

#ifdef AVLUS	
	traceLog(LOG_DEBUG,"add_level: [%s](%d),[%s]", name->val,
				 name->len, rs->name);
#endif
	c = oct2strcmp(name, rs->name);

	if (c < 0) {
		while( c < 0 ) {
			/* Far left */
#ifdef AVLUS
			traceLog(LOG_DEBUG,"Left of %s", rs->name);
#endif

			if (!rs->left) {
				rs->left = new;
				new->right = rs;
				rs = new;
				c = 0;
			}
			else {
				rs = rs->left;
				c = oct2strcmp(name, rs->name);
			}
		}
		
		if ( c > 0 ) {
			new->right = rs->right;
			new->left = rs;
			new->right->left = new;
			rs->right = new;	
			rs = new;
		}
	} else if( c > 0) {
		while( c > 0 ) {
#ifdef AVLUS
			traceLog(LOG_DEBUG,"Right of %s", rs->name);
#endif
			/* Far right */
			if (!rs->right) {
				rs->right = new;
				new->left = rs;
				rs = new;
				c = 0;
			}
			else {
				rs = rs->right;
				c = oct2strcmp(name, rs->name);
			}
		}
		
		if ( c < 0 ) {
			new->left = rs->left;
			new->right = rs;
			new->left->right = new;
			rs->left = new;	
			rs = new;
		}
	} 

	return rs;
}

/*
 * ---------------------------------------------------------------------- 
 */

ruleset_t      *
ruleset_create(octet_t * name, ruleset_t *root)
{
	ruleset_t      *r, *nr;
	octet_t         loc;
	octarr_t       *oa = 0;
	int             i, pathlen = 0;

	/*
	{
		char *tmp;

		tmp = oct2strdup( name, '%' );
		traceLog(LOG_INFO,"Create ruleset: %s", tmp);
		Free(tmp);
	}
	*/

#ifdef AVLUS
	oct_print(LOG_INFO,"Ruleset create", name);
#endif

	if (root == 0) {
		root = ruleset_new(0);

		if (name == 0 || name->len == 0
		    || (name->len == 1 && *name->val == '/'))
			return root;
	} /* else
	       traceLog(LOG_INFO, "Got some kind of tree");	
	       */


	if( name == 0 || name->len == 0 ) return root ;

	r = root;

	/*
	 * special case 
	 */
	if (name->len >= 2 && *name->val == '/' && *(name->val + 1) == '/') {
		oct_assign(&loc, "//");

		if ((nr = one_level(&loc, r)) == 0) {
			nr = ruleset_new(&loc);
			r->left = nr;
			nr->up = r;
			r = nr;
		} else
			r = nr;

		octln(&loc, name);
		loc.val += 2;
		loc.len -= 2;
	} else if (*name->val == '/') {
		octln(&loc, name);
		loc.val++;
		loc.len--;
	} else
		octln(&loc, name);

	oa = path_split(&loc, &pathlen);

	for (i = 0; oa && i < oa->n; i++) {

		if ((nr = one_level(oa->arr[i], r->down)) == 0) {
			if (r->down)
				nr = add_to_level(oa->arr[i], r->down);
			else
				r->down = nr = ruleset_new(oa->arr[i]);
			nr->up = r;
			r = nr;
		} else
			r = nr;
	}

	name->val = loc.val + pathlen;
	name->len = loc.len - pathlen;

	return root;
}

/*
 * ---------------------------------------------------------------------- 
 */

/*
 * should be of the format [ "/" *( string "/" ) ] s-exp string = 1*(
 * %x30-39 / %x61-7A / %x41-5A / %x2E ) 
 */

spocp_result_t
ruleset_name(octet_t * orig, octet_t * rsn)
{
	char           *cp;
	size_t          l;

	cp = orig->val;

	if (*cp != '/') {
		rsn->val = 0;
		rsn->len = 0;
	} else {
		/*
		 * get the name space info 
		 */
		for (l = 0; (DIRCHAR(*cp) || *cp == '/') && l != orig->len;
		     cp++, l++);

		if (l == orig->len)
			return SPOCP_SYNTAXERROR;	/* very fishy */

		rsn->val = orig->val;
		rsn->len = l;

		orig->val = cp;
		orig->len -= l;
	}

	return SPOCP_SUCCESS;
}

spocp_result_t
pathname_get(ruleset_t * rs, char *buf, int buflen)
{
	int             len = 0;
	ruleset_t      *rp;

	for (rp = rs; rp; rp = rp->up) {
		len++;
		len += strlen(rp->name);
	}
	len++;

	if (len > buflen)
		return SPOCP_OPERATIONSERROR;

	*buf = 0;

	for (rp = rs; rp; rp = rp->up) {
		/* Flawfinder: ignore */
		strcat(buf, "/");
		/* Flawfinder: ignore */
		strcat(buf, rp->name);
	}

	return SPOCP_SUCCESS;
}

spocp_result_t
treeList(ruleset_t * rs, conn_t * conn, octarr_t * oa, int recurs)
{
	spocp_result_t  rc = SPOCP_SUCCESS;
	ruleset_t      *rp;
	char            pathname[BUFSIZ];
	work_info_t	wi;

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

		if ((rc = pathname_get(rs, pathname, BUFSIZ)) != SPOCP_SUCCESS)
			return rc;

		if (rs->db) {
			rc = dbapi_rules_list(rs->db, 0, wi.oparg, oa, pathname);
		}

		if (0)
			traceLog(LOG_DEBUG,"Done head");

		if (recurs && rs->down) {
			for (rp = rs->down; rp->left; rp = rp->left);

			for (; rp; rp = rp->right) {
				rc = treeList(rp, conn, oa, recurs);
			}
		}
	}

	return rc;
}


