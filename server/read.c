#include "locl.h"
/*
 * RCSID("$Id$"); 
 */

typedef struct _ptree {
	int             list;
	octet_t         val;
	struct _ptree  *next;
	struct _ptree  *part;
} ptree_t;


char           *strndup(const char *old, size_t sz);

/*
 * ----------------------------------------------------------------------------- 
 */

static void
ptree_free(ptree_t * ptp)
{
	if (ptp) {
		if (ptp->part)
			ptree_free(ptp->part);
		if (ptp->next)
			ptree_free(ptp->next);
		if (ptp->val.size)
			free(ptp->val.val);
		free(ptp);
	}
}
/*
 * ----------------------------------------------------------------------------- 
 */

static ptree_t *
parse_sexp(octet_t * sexp)
{
	ptree_t        *ptp, *ntp = 0, *ptr;

	if (*sexp->val == '(') {
		ptp = (ptree_t *) calloc(1, sizeof(ptree_t));
		ptp->list = 1;
		sexp->val++;
		sexp->len--;
		while (sexp->len && *sexp->val != ')') {
			if ((ptr = parse_sexp(sexp)) == 0) {
				ptree_free(ptp);
				return 0;
			}

			if (ptp->part == 0)
				ntp = ptp->part = ptr;
			else {
				ntp->next = ptr;
				ntp = ntp->next;
			}
		}
		if (*sexp->val == ')') {
			sexp->val++;
			sexp->len--;
		} else {	/* error */
			ptree_free(ptp);
			return 0;
		}
	} else {
		ptp = (ptree_t *) calloc(1, sizeof(ptree_t));
		if (get_str(sexp, &ptp->val) != SPOCP_SUCCESS) {
			ptree_free(ptp);
			return 0;
		}
	}

	return ptp;
}

static int
length_sexp(ptree_t * ptp)
{
	int             len = 0;
	ptree_t        *pt;

	if (ptp->list) {
		len = 2;
		for (pt = ptp->part; pt; pt = pt->next)
			len += length_sexp(pt);
	} else {
		len = DIGITS(ptp->val.len);
		len++;
		len += ptp->val.len;
	}

	return len;
}

static void
recreate_sexp(octet_t * o, ptree_t * ptp)
{
	ptree_t        *pt;
	int             len = 0;

	if (ptp->list) {
		*o->val++ = '(';
		o->len++;
		for (pt = ptp->part; pt; pt = pt->next)
			recreate_sexp(o, pt);
		*o->val++ = ')';
		o->len++;
	} else {
		sprintf(o->val, "%d:", ptp->val.len);
		len = DIGITS(ptp->val.len);
		len++;
		o->len += len;
		o->val += len;
		strncpy(o->val, ptp->val.val, ptp->val.len);
		o->len += ptp->val.len;
		o->val += ptp->val.len;
	}
}

/*
 * ----------------------------------------------------------------------------- 
 * The format of the rule file
 *
 */

int
read_rules(srv_t * srv, char *file, dbcmd_t * dbc)
{
	FILE           *fp;
	char           *sp, *tmp;
	int             n = 0, f = 0, r;
	octet_t         oct, *op;
	octarr_t       *oa = 0;
	ruleset_t      *rs = 0, *trs;
	spocp_result_t  rc = SPOCP_SUCCESS;
	spocp_charbuf_t	*buf;
	spocp_chunk_t	*chunk = 0, *ck;
	spocp_ruledef_t	rdef;
	struct stat	statbuf;

	if ((fp = fopen(file, "r")) == 0) {
		LOG(SPOCP_EMERG) traceLog(LOG_ERR,"couldn't open rule file \"%s\"",
					  file);
		op = oct_new( 256, NULL);
		sp = getcwd(op->val, op->size);
		traceLog(LOG_ERR,"I'm in \"%s\"", sp);
		oct_free(op);
		return -1;
	}

	stat( file, &statbuf);

	srv->mtime = statbuf.st_mtime;
 
	/*
	 * The default ruleset should already be set 
	 */

	if (srv->root == 0) {
		srv->root = rs = ruleset_new(0);
	} else
		rs = srv->root;

	if (rs->db == 0)
		rs->db = db_new();

	buf = charbuf_new( fp, BUFSIZ );

	if (get_more(buf) == 0) return 0;

	/*
	 * have to escape CR since fgets stops reading when it hits a newline
	 * NUL also has to be escaped since I have problem otherwise finding
	 * the length of the 'string'. '\' hex hex is probably going to be the 
	 * choice 
	 */
	while (rc == SPOCP_SUCCESS && ( chunk = get_object( buf, 0 )) != 0 ) {
		if (oct2strcmp(chunk->val, ";include ") == 0) {	/* include
								 * file */
			ck = chunk->next;
			tmp = oct2strdup( ck->val, 0 ) ;
			LOG(SPOCP_DEBUG) traceLog(LOG_DEBUG,"include directive \"%s\"",
						  tmp);
			if ((rc = read_rules(srv, tmp, dbc)) < 0) {
				traceLog(LOG_ERR,"Include problem");
			}
		}
		else if (*chunk->val->val == '/' || *chunk->val->val == '(') {
			trs = rs;
			if (*chunk->val->val == '/') {
				if (ruleset_find( chunk->val, &trs) == 0) {
					ruleset_create(chunk->val, &rs);
					LOG(SPOCP_DEBUG)
					    traceLog(LOG_DEBUG,"ruleset name: \"%s\"",
					    rs->name);
					trs = rs;
					ruleset_find(chunk->val, &trs);
					trs->db = db_new();
				}

				ck = chunk->next;
			}
			else {
				ck = chunk;
			}

			ruledef_return( &rdef, ck ) ;
			if( rdef.rule ) {
				op = chunk2sexp( rdef.rule ) ;
				oa = octarr_add(oa, op) ;
			}
			
			if( rdef.bcond) {
				op = chunk2sexp( rdef.bcond ) ;
				oa = octarr_add(oa, op) ;
			}
			
			if( rdef.blob ) {
				if (!rdef.bcond)
					oa = octarr_add(oa, str2oct("NULL", 0)) ;
				oa = octarr_add(oa, octdup(rdef.blob->val)) ;
			}
			
			r = dbapi_rule_add(&(trs->db), srv->plugin, dbc, oa, NULL);
			if (r == SPOCP_SUCCESS)
				n++;
			else {
				LOG(SPOCP_WARNING)
				    traceLog(LOG_WARNING,"Failed to add rule: \"%s\"",
					     oct.val);
				f++;
			}

			octarr_free(oa);
			oa = 0;
			
		}
       		else {
			octet_t *key, *val, *to = 0;

			key = chunk->val ;
			if( *chunk->next->next->val->val == '(' ) {
				ruledef_return( &rdef, chunk->next->next ) ;
				if( rdef.rule) {
					val = to = chunk2sexp( rdef.rule ) ;
				}
				else {
					LOG( SPOCP_WARNING )
						traceLog(LOG_WARNING,
						    "Error in bconddef");
					f++;
					chunk_free( chunk ) ;
					continue ;
				}
			}
			else 
				val = chunk->next->next->val ;

			if (bcdef_add(rs->db, srv->plugin, dbc, key, val) == 0 )
				rc = SPOCP_LOCAL_ERROR;

			if (to) oct_free( to );
		}

		chunk_free( chunk ) ;
	}

	fclose( fp );
	charbuf_free( buf );

	if (rc != SPOCP_SUCCESS)
		return -1;
	else {
		LOG(SPOCP_INFO) traceLog(LOG_INFO,"Stored %d rules, failed %d", n, f);

		return f;
	}
}

/***************************************************************

 Structure of the rule file:

 line     = ( comment | global | ruledef | include | bconddef ) 
 comment  = "#" text CR
 rule     = "(" S_exp ")" [1*SP bcond [ 1*SP blob]] CR
 global   = key "=" value
 bconddef = key ":=" value
 include  = ";include" filename

 ***************************************************************/

static octet_t *
bcref_create(char *bcname)
{
	octet_t        *ref;
	size_t          size, l = strlen(bcname);;

	size = l + DIGITS(l) + 1;
	size += 8;

	ref = oct_new(size, 0);
	sexp_printv(ref->val, &size, "(aa)", "ref", bcname);
	ref->len = ref->size - size;

	return ref;
}

/*
 * ---------------------------------------------------------------------- 
 */

int
dback_read_rules(dbcmd_t * dbc, srv_t * srv, spocp_result_t * rc)
{
	octarr_t       *oa = 0, *roa = 0;
	spocp_result_t  r;
	int             i, f = 0, n = 0;
	octet_t         dat0, dat1, *bcond, name;
	char           *bcname = 0, *tmp;
	ruleset_t      *rs;

	oa = dback_all_keys(dbc, &r);

	memset(&dat0, 0, sizeof(octet_t));
	memset(&dat1, 0, sizeof(octet_t));

	if (r == SPOCP_SUCCESS && oa && oa->n) {

		if (srv->root == 0)
			srv->root = ruleset_new(0);

		rs = srv->root;

		if (rs->db == 0)
			rs->db = db_new();

		/*
		 * three passes, the first for simple bconds 
		 */
		for (i = 0; i < oa->n; i++) {
			if (strncmp(oa->arr[i]->val, "BCOND:", 6) != 0)
				continue;

			if (bcspec_is(oa->arr[i]) == TRUE) {
				tmp =
				    strndup(oa->arr[i]->val + 6,
					     oa->arr[i]->len - 6);

				r = dback_read(dbc, tmp, &dat0, &dat1,
					       &bcname);

				if (dat0.len) {
					oct_assign(&name, tmp);
					bcdef_add(rs->db, srv->plugin, dbc,
						  &name, &dat0);
					octclr(&dat0);
				}
				free(tmp);
			}
		}
		/*
		 * the second for compound bconds 
		 */
		for (i = 0; i < oa->n; i++) {
			if (strncmp(oa->arr[i]->val, "BCOND:", 6) != 0)
				continue;

			if (bcspec_is(oa->arr[i]) == FALSE) {
				tmp =
				    strndup(oa->arr[i]->val + 6,
					     oa->arr[i]->len - 6);

				r = dback_read(dbc, tmp, &dat0, &dat1,
					       &bcname);

				if (dat0.len) {
					oct_assign(&name, tmp);
					bcdef_add(rs->db, srv->plugin, dbc,
						  &name, &dat0);
					octclr(&dat0);
				}

				free(tmp);
			}
		}
		/*
		 * and the third for rules 
		 */
		for (i = 0; i < oa->n; i++) {
			if (strncmp(oa->arr[i]->val, "BCOND:", 6) == 0)
				continue;
			tmp = oct2strdup(oa->arr[i], 0);
			r = dback_read(dbc, tmp, &dat0, &dat1, &bcname);
			free(tmp);

			roa = octarr_add(roa, octdup(&dat0));
			octclr(&dat0);

			if (bcname) {
				bcond = bcref_create(bcname);
				octarr_add(roa, bcond);
				bcname = 0;
			}

			if (dat1.len) {
				octarr_add(roa, octdup(&dat1));
				octclr(&dat1);
			}

			if ((r =
			     spocp_add_rule((void **) &(rs->db),
					    roa)) == SPOCP_SUCCESS)
				n++;
			else {
				LOG(SPOCP_WARNING)
				    traceLog(LOG_WARNING,"Failed to add rule: \"%s\"",
					     dat0.val);
				f++;
			}
			octarr_free(roa);

			roa = 0;
		}

		octarr_free(oa);
	}

	return n;
}
