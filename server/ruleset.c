
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
ruleset_free(ruleset_t * rs)
{
	if (rs) {
		if (rs->name)
			free(rs->name);

		if (rs->down)
			ruleset_free(rs->down);
		if (rs->left)
			ruleset_free(rs->left);
		if (rs->right)
			ruleset_free(rs->right);

		if (rs->db) 
			db_free( rs->db );

		free(rs);
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

ruleset_t      *
one_level(octet_t * name, ruleset_t * rs)
{
	int             c;

	if (rs == 0)
		return 0;

	do {
		if (0)
			traceLog(LOG_DEBUG,"one_level: [%s](%d),[%s]", name->val,
				 name->len, rs->name);
		c = oct2strcmp(name, rs->name);

		if (c == 0) {
			break;
		} else if (c < 0) {
			rs = rs->left;
		} else {
			rs = rs->right;
		}
	} while (c && rs);

	return rs;
}

/*
 * ---------------------------------------------------------------------- 
 */

/*
 * returns 1 if a ruleset is found with the given name 
 */
int
ruleset_find(octet_t * name, ruleset_t ** rs)
{
	octet_t         loc;
	ruleset_t      *r, *nr = 0;
	octarr_t       *oa = 0;
	int             i, pathlen;

	if (*rs == 0)
		return 0;

	/*
	 * the root 
	 */
	if (name == 0 || name->len == 0
	    || (*name->val == '/' && name->len == 1)) {
		for (r = *rs; r->up; r = r->up);
		*rs = r;
		return 1;
	}
	/*
	 * server part 
	 */
	else if (name->len >= 2 && *name->val == '/'
		 && *(name->val + 1) == '/') {
		for (r = *rs; r->up; r = r->up);

		loc.val = "//";
		loc.len = 2;
		if ((nr = one_level(&loc, r)) == 0) {
			*rs = r;
			return 0;
		}

		if (nr && name->len == 2) {
			*rs = nr;
			name->val += 2;
			name->len -= 2;
			return 1;
		}

		loc.val = name->val + 2;
		loc.len = name->len - 2;
		loc.size = 0;
	}
	/*
	 * absolute path 
	 */
	else if (*name->val == '/') {
		for (nr = *rs; nr->up; nr = nr->up);
		octln(&loc, name);
		loc.val++;
		loc.len--;
		r = nr;
	} else {
		return 0;	/* don't do relative */
	}

	oa = path_split(&loc, &pathlen);
	for (i = 0; oa && i < oa->n; i++) {
		r = nr->down;
		if (r == 0) {
			*rs = r;
			return 0;
		}

		if ((nr = one_level(oa->arr[i], r)) == 0)
			break;
	}

	if (nr == 0) {
		*rs = r;
		return 0;
	} else
		*rs = nr;

	name->val = loc.val + pathlen;
	name->len = loc.len - pathlen;

	return 1;
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

	do {
		if (0)
			traceLog(LOG_DEBUG,"add_level: [%s](%d),[%s]", name->val,
				 name->len, rs->name);
		c = oct2strcmp(name, rs->name);

		if (c < 0) {
			if (!rs->left) {
				rs->left = new;
				c = 0;
			}
			rs = rs->left;
		} else {
			if (!rs->right) {
				rs->right = new;
				c = 0;
			}
			rs = rs->right;
		}
	} while (c && rs);

	return rs;
}

/*
 * ---------------------------------------------------------------------- 
 */

ruleset_t      *
ruleset_create(octet_t * name, ruleset_t ** root)
{
	ruleset_t      *r = 0, *nr;
	octet_t         loc;
	octarr_t       *oa = 0;
	int             i, pathlen = 0;

	if (*root == 0) {
		*root = r = ruleset_new(0);

		if (name == 0 || name->len == 0
		    || (name->len == 1 && *name->val == '/'))
			return r;
	} else
		r = *root;


	if( name == 0 || name->len == 0 ) return r ;

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

	return r;
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

	/*
	 * check access to operation 
	 */
	if ((rc = operation_access(conn)) != SPOCP_SUCCESS) {
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
			rc = dbapi_rules_list(rs->db, 0, conn->oparg, oa,
					      pathname);
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


