
/***************************************************************************
                             input.c  

   -  routines to parse canonical S-expression into the incore representation

                             -------------------

    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#define _XOPEN_SOURCE

#include <struct.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <time.h>

#include <config.h>

#include <macros.h>
#include <wrappers.h>
#include <func.h>
#include <spocp.h>
#include <proto.h>


spocp_result_t  get_str(octet_t * so, octet_t * ro);
spocp_result_t  get_and(octet_t * oct, element_t * ep);

atom_t         *get_atom(octet_t * op, spocp_result_t * rc);
atom_t         *atom_new(octet_t * op);

spocp_result_t  do_prefix(octet_t * oct, element_t * ep);
spocp_result_t  do_suffix(octet_t * oct, element_t * ep);
spocp_result_t  do_or(octet_t * oct, element_t * ep);
spocp_result_t  do_range(octet_t * oct, element_t * ep);
spocp_result_t  do_list(octet_t * tag, octet_t * list, element_t * ep,
			char *base);

spocp_result_t  is_valid_range(range_t * rp);
spocp_result_t  is_atom(range_t * rp);

spocp_result_t  set_limit(boundary_t * bp, octet_t * op);
boundary_t     *set_delimiter(range_t * range, octet_t oct);

/*
 * =============================================================== 
 */

element_t      *
element_new()
{
	element_t      *ep;

	ep = (element_t *) Calloc(1, sizeof(element_t));

	return ep;
}

atom_t         *
atom_new(octet_t * op)
{
	atom_t         *ap;

	ap = (atom_t *) Malloc(sizeof(atom_t));

	if (op) {
		ap->val.val = Strndup(op->val, op->len);
		ap->val.len = op->len;
		ap->val.size = op->len;

		ap->hash = lhash((unsigned char *) op->val, op->len, 0);
	} else {
		ap->val.val = 0;
		ap->val.len = 0;
		ap->val.size = 0;
		ap->hash = 0;
	}

	return ap;
}

atom_t         *
get_atom(octet_t * op, spocp_result_t * rc)
{
	octet_t         oct;

	if ((*rc = get_str(op, &oct)) != SPOCP_SUCCESS) {
		return 0;
	}

	return atom_new(&oct);
}

spocp_result_t
do_prefix(octet_t * oct, element_t * ep)
{
	spocp_result_t  rc = SPOCP_SUCCESS;

	DEBUG(SPOCP_DPARSE) traceLog("Parsing 'prefix' expression");

	ep->type = SPOC_PREFIX;

	if ((ep->e.atom = get_atom(oct, &rc)) == 0)
		return rc;

	/*
	 * a check for external references, which is not allowed in ranges 
	 */

	if (*oct->val != ')') {
		LOG(SPOCP_ERR) traceLog("Missing ending ')'");
		return SPOCP_SYNTAXERROR;
	} else {
		oct->val++;
		oct->len--;
	}

	return SPOCP_SUCCESS;
}

spocp_result_t
do_suffix(octet_t * oct, element_t * ep)
{
	spocp_result_t  rc = 0;

	DEBUG(SPOCP_DPARSE) traceLog("Parsing 'suffix' expression");

	ep->type = SPOC_SUFFIX;

	if ((ep->e.atom = get_atom(oct, &rc)) == 0)
		return rc;

	if (*oct->val != ')') {
		LOG(SPOCP_ERR) traceLog("Missing ending ')'");
		return SPOCP_SYNTAXERROR;
	} else {
		oct->val++;
		oct->len--;
	}

	return SPOCP_SUCCESS;
}

/*
 * ----------------------------------- 
 */

static list_t  *
list_new(void)
{
	return (list_t *) Calloc(1, sizeof(list_t));
}

/*
 * ----------------------------------- 
 */

void
set_memberof(varr_t * va, element_t * group)
{
	void           *vp;
	element_t      *ep;

	for (vp = varr_first(va); vp; vp = varr_next(va, vp)) {
		ep = (element_t *) vp;
		ep->memberof = group;
	}
}


/*
 * ----------------------------------- 
 */

static spocp_result_t
get_set(octet_t * oct, element_t * ep)
{
	element_t      *nep = 0;
	varr_t         *pa, *sa = 0;
	spocp_result_t  rc;

	/*
	 * until end of star expression is found 
	 */
	while (oct->len && *oct->val != ')') {

		if ((rc = element_get(oct, &nep)) != SPOCP_SUCCESS) {
			return rc;
		}

		/*
		 * what if nep is of the same type as ep, deflate 
		 */
		if (nep->type == ep->type) {
			pa = nep->e.set;

			sa = varr_or(sa, pa, 0);
		} else {
			sa = varr_add(sa, (void *) nep);
		}
	}

	DEBUG(SPOCP_DPARSE) traceLog("Got end of set");

	/*
	 * only one item in the and expr, so it's not really a set. It's just
	 * a element and should therefore be treated as such 
	 */
	if (sa->n == 1) {
		ep = (element_t *) varr_pop(sa);
		varr_free(sa);
	} else {
		ep->e.set = sa;
	}

	set_memberof(sa, ep);

	if (*oct->val == ')') {
		oct->val++;
		oct->len--;
	}

	return SPOCP_SUCCESS;
}

/*
 * -------------------------------------------------------------------------- 
 */

static spocp_result_t
do_set(octet_t * oct, element_t * ep)
{
	DEBUG(SPOCP_DPARSE) traceLog("Parsing 'set' expression");

	ep->type = SPOC_SET;

	return get_set(oct, ep);
}

/*
 * -------------------------------------------------------------------------- 
 */

boundary_t     *
set_delimiter(range_t * range, octet_t oct)
{
	boundary_t     *bp = 0;

	if (oct.len != 2)
		return 0;

	switch (oct.val[0]) {
	case 'g':
		switch (oct.val[1]) {
		case 'e':
			range->lower.type |= GLE;
			bp = &range->lower;
			break;

		case 't':
			range->lower.type |= GT;
			bp = &range->lower;
			break;
		}
		break;

	case 'l':
		switch (oct.val[1]) {
		case 'e':
			range->upper.type |= GLE;
			bp = &range->upper;
			break;

		case 't':
			range->upper.type |= LT;
			bp = &range->upper;
			break;
		}
		break;
	}

	return bp;
}

/*
 * -------------------------------------------------------------------------- 
 */

void
to_gmt(octet_t * s, octet_t * t)
{
	char           *sp, time[20];
	struct tm       tm;
	time_t          tid;

	if (s->len == 19 || s->len == 20) {
		t->val = Strndup(s->val, 19);
		t->len = 19;
	} else {		/* offset */
		strncpy(time, s->val, 19);
		time[19] = '\0';
		strptime(time, "%Y-%m-%dT%H:%M:%S", &tm);
		tid = mktime(&tm);

		sp = s->val + 19;
		if (*sp == '+') {
			sp++;
			tid += (*sp++ - '0') * 36000;
			tid += (*sp++ - '0') * 3600;
			if (s->len > 22) {
				s++;
				tid += (*sp++ - '0') * 600;
				tid += (*sp++ - '0') * 60;
			}
		} else {
			sp++;
			tid -= (*sp++ - '0') * 36000;
			tid -= (*sp++ - '0') * 3600;
			if (s->len > 22) {
				s++;
				tid -= (*sp++ - '0') * 600;
				tid -= (*sp++ - '0') * 60;
			}
		}

		localtime_r(&tid, &tm);

		t->val = (char *) Malloc(20 * sizeof(char));
		t->val[19] = 0;

		strftime(t->val, 20, "%Y-%m-%dT%H:%M:%S", &tm);
		t->len = 19;
	}
}

void
hms2int(octet_t * op, long *num)
{
	char           *cp;

	cp = op->val;

	*num += (*cp++ - '0') * 36000;
	*num += (*cp++ - '0') * 3600;

	cp++;

	*num += (*cp++ - '0') * 600;
	*num += (*cp++ - '0') * 60;

	cp++;

	*num += (*cp++ - '0') * 10;
	*num += (*cp++ - '0');
}

/*
 * -------------------------------------------------------------------------- 
 */

spocp_result_t
set_limit(boundary_t * bp, octet_t * op)
{
	int             r = SPOCP_SYNTAXERROR;

	switch (bp->type & 0x07) {
	case SPOC_NUMERIC:
		r = is_numeric(op, &bp->v.num);
		break;

	case SPOC_ALPHA:
		bp->v.val.val = Strndup(op->val, op->len);
		bp->v.val.len = op->len;
		r = SPOCP_SUCCESS;
		break;

	case SPOC_DATE:
		if ((r = is_date(op)) == SPOCP_SUCCESS)
			to_gmt(op, &bp->v.val);
		break;

	case SPOC_TIME:
		if ((r = is_time(op)) == SPOCP_SUCCESS)
			hms2int(op, &bp->v.num);
		break;

	case SPOC_IPV4:
		r = is_ipv4(op, &bp->v.v4);
		break;

	case SPOC_IPV6:
		r = is_ipv6(op, &bp->v.v6);
		break;
	}

	return r;
}

int
ipv4cmp(struct in_addr *ia1, struct in_addr *ia2)
{
	return ia2->s_addr - ia1->s_addr;
}

int
ipv6cmp(struct in6_addr *ia1, struct in6_addr *ia2)
{
	uint32_t       *lw = (uint32_t *) ia1;
	uint32_t       *uw = (uint32_t *) ia2;
	int             r, i;

	for (i = 0; i < 4; i++, lw++, uw++) {
		r = *uw - *lw;
		if (r)
			return r;
	}

	return 0;
}

/*
 * what's invalid ? 1) the upper boundary being lower than the lower 2) not
 * allowing the only possible value to be within the boundaries equivalent to
 * ( x > 5 && x < 5 )
 * 
 * Returns: FALSE if invalid TRUE if valid 
 */
spocp_result_t
is_valid_range(range_t * rp)
{
	spocp_result_t  r = SPOCP_SUCCESS;
	int             c = 0;

	if (rp->lower.type & 0xF0 && rp->upper.type & 0xF0) {

		switch (rp->lower.type & 0x07) {
		case SPOC_NUMERIC:
		case SPOC_TIME:
			c = rp->upper.v.num - rp->lower.v.num;
			break;

		case SPOC_IPV4:
			c = ipv4cmp(&rp->lower.v.v4, &rp->upper.v.v4);
			break;

		case SPOC_IPV6:
			c = ipv6cmp(&rp->lower.v.v6, &rp->upper.v.v6);
			break;

		case SPOC_ALPHA:
		case SPOC_DATE:
			c = octcmp(&rp->upper.v.val, &rp->lower.v.val);
			break;

		default:
			return SPOCP_UNKNOWN_TYPE;
		}

		if (c < 0) {
			LOG(SPOCP_ERR) traceLog("Upper limit less then lower");
			r = SPOCP_SYNTAXERROR;
		} else if (c == 0 && !(rp->upper.type & GLE)
			   && !(rp->lower.type & GLE)) {
			LOG(SPOCP_ERR)
			    traceLog
			    ("Upper limit equal to lower when it shouldn't");
			r = SPOCP_SYNTAXERROR;
		}
	}

	return r;
}

/*
 * checks whether the range in fact is an atom 
 */
spocp_result_t
is_atom(range_t * rp)
{
	uint32_t       *u, *l;
	spocp_result_t  sr = SPOCP_SYNTAXERROR;

	switch (rp->lower.type & 0x07) {
	case SPOC_NUMERIC:
	case SPOC_TIME:
		if (rp->lower.v.num == 0);
		else if (rp->lower.v.num == rp->upper.v.num)
			sr = SPOCP_SUCCESS;
		break;

	case SPOC_IPV4:
		l = (uint32_t *) & rp->lower.v.v4;
		u = (uint32_t *) & rp->upper.v.v4;
		if (*l == *u)
			sr = SPOCP_SUCCESS;
		break;

	case SPOC_IPV6:
		l = (uint32_t *) & rp->lower.v.v6;
		u = (uint32_t *) & rp->upper.v.v6;
		if (l[0] == u[0] && l[1] == u[1] && l[2] == u[2]
		    && l[3] == u[3])
			sr = SPOCP_SUCCESS;
		break;

	case SPOC_ALPHA:
	case SPOC_DATE:
		if (rp->lower.v.val.len == 0 && rp->upper.v.val.len == 0)
			sr = SPOCP_SUCCESS;

		if (rp->lower.v.val.len == 0 || rp->upper.v.val.len == 0)
			sr = SPOCP_SUCCESS;
		else if (octcmp(&rp->lower.v.val, &rp->upper.v.val) == 0)
			sr = SPOCP_SUCCESS;
		break;
	}

	return sr;
}

spocp_result_t
do_range(octet_t * op, element_t * ep)
{
	octet_t         oct;
	range_t        *rp = 0;
	boundary_t     *bp;
	int             r = SPOCP_SUCCESS;

	DEBUG(SPOCP_DPARSE) traceLog("Parsing range");

	/*
	 * next part should be type specifier 
	 */

	if ((r = get_str(op, &oct)) != SPOCP_SUCCESS)
		goto done;

	ep->type = SPOC_RANGE;
	DEBUG(SPOCP_DPARSE) traceLog("new_range");
	rp = (range_t *) Calloc(1, sizeof(range_t));

	ep->e.range = rp;

	switch (oct.len) {
	case 4:
		if (strncasecmp(oct.val, "date", 4) == 0)
			rp->lower.type = SPOC_DATE;
		else if (strncasecmp(oct.val, "ipv4", 4) == 0)
			rp->lower.type = SPOC_IPV4;
		else if (strncasecmp(oct.val, "ipv6", 4) == 0)
			rp->lower.type = SPOC_IPV6;
		else if (strncasecmp(oct.val, "time", 4) == 0)
			rp->lower.type = SPOC_TIME;
		else {
			LOG(SPOCP_ERR) traceLog("Unknown range type");
			r = SPOCP_SYNTAXERROR;
		}
		break;

	case 5:
		if (strncasecmp(oct.val, "alpha", 5) == 0)
			rp->lower.type = SPOC_ALPHA;
		else {
			LOG(SPOCP_ERR) traceLog("Unknown range type");
			r = SPOCP_SYNTAXERROR;
		}
		break;

	case 7:
		if (strncasecmp(oct.val, "numeric", 7) == 0)
			rp->lower.type = SPOC_NUMERIC;
		else {
			LOG(SPOCP_ERR) traceLog("Unknown range type");
			r = SPOCP_SYNTAXERROR;
		}
		break;

	default:
		LOG(SPOCP_ERR) traceLog("Unknown range type");
		r = SPOCP_SYNTAXERROR;
		break;
	}

	if (r == SPOCP_SYNTAXERROR)
		goto cleanup;

	rp->upper.type = rp->lower.type;

	if (*op->val == ')') {	/* no limits defined */
		rp->lower.v.num = 0;
		rp->upper.v.num = 0;
		op->val++;
		op->len--;
		goto done;
	}

	/*
	 * next part should be limiter description 'lt'/'le'/'gt'/'ge' 
	 */

	if ((r = get_str(op, &oct)) != SPOCP_SUCCESS)
		goto cleanup;

	if ((bp = set_delimiter(rp, oct)) == 0) {
		LOG(SPOCP_ERR) traceLog("Error in delimiter specification");
		r = SPOCP_SYNTAXERROR;
		goto cleanup;
	}

	/*
	 * and then some value 
	 */
	/*
	 * it can't be a external reference 
	 */

	if ((r = get_str(op, &oct)) != SPOCP_SUCCESS)
		goto cleanup;

	if ((r = set_limit(bp, &oct)) != SPOCP_SUCCESS)
		goto cleanup;

	if (r == SPOCP_SUCCESS && *op->val == ')') {	/* no more limit
							 * defined */
		op->val++;
		op->len--;
		goto done;
	}

	/*
	 * next part should be limiter description 'lt'/'le'/'gt'/'ge'
	 * can't/shouldn't (?) be the same as above 
	 */

	if ((r = get_str(op, &oct)) != SPOCP_SUCCESS)
		goto cleanup;

	if (r == SPOCP_SUCCESS && (bp = set_delimiter(rp, oct)) == 0) {
		LOG(SPOCP_ERR) traceLog("Error in delimiter specification");
		r = SPOCP_SYNTAXERROR;
		goto cleanup;
	}

	/*
	 * and then some value 
	 */

	if ((r = get_str(op, &oct)) != SPOCP_SUCCESS)
		goto cleanup;

	if ((r = set_limit(bp, &oct)) != SPOCP_SUCCESS)
		goto cleanup;

	if (r == SPOCP_SUCCESS && *op->val != ')') {
		LOG(SPOCP_ERR) traceLog("Missing closing ')'");
		r = SPOCP_SYNTAXERROR;
		goto cleanup;
	}

	if ((r = is_valid_range(rp)) != SPOCP_SUCCESS)
		goto cleanup;

	if (is_atom(rp) == SPOCP_SUCCESS) {
		ep->type = SPOC_ATOM;
		ep->e.atom = atom_new(&oct);
		range_free(rp);
	}

      cleanup:
	if (r != SPOCP_SUCCESS) {
		range_free(rp);
		ep->e.range = 0;
	} else {
		/*
		 * boundary_print( &rp->lower ) ; boundary_print( &rp->upper ) 
		 * ; 
		 */
		op->val++;
		op->len--;
	}

      done:
	return r;
}

spocp_result_t
do_list(octet_t * to, octet_t * lo, element_t * ep, char *base)
{
	spocp_result_t  r = SPOCP_SUCCESS;
	element_t      *nep = 0, *pep;

	DEBUG(SPOCP_DPARSE) traceLog("List tag");

	ep->type = SPOC_LIST;
	ep->e.list = list_new();

	pep = ep->e.list->head = element_new();

	/*
	 * first element has to be a atom 
	 */
	pep->memberof = ep;
	pep->type = SPOC_ATOM;
	pep->e.atom = atom_new(to);

	/*
	 * get rest of the element 
	 */

	while (lo->len) {
		nep = 0;

		r = element_get(lo, &nep);

		if (r != SPOCP_SUCCESS) {
			list_free(ep->e.list);
			ep->e.list = 0;
			return r;
		}

		if (nep == 0)
			break;	/* end of rule */

		pep->next = nep;
		nep->memberof = ep;
		pep = nep;
	}

	/*
	 * sanity check 
	 */
	if (nep != 0)
		return SPOCP_SYNTAXERROR;

	DEBUG(SPOCP_DPARSE) {
		char           *tmp;
		tmp = oct2strdup(to, '\\');
		traceLog("Input end of list with tag [%s]", tmp);
		free(tmp);
	}

	return SPOCP_SUCCESS;
}

/*
 * parse a string until I've got one full query 
 */
/*
 * S-expression format is (3:tag7:element) the so called canonical
 * representation, and not (tag element)
 * 
 * which among other things means that spaces does not occur between elements 
 */

spocp_result_t
element_get(octet_t * op, element_t ** epp)
{
	char           *base;
	octet_t         oct;
	spocp_result_t  r = SPOCP_SUCCESS;
	element_t      *ep = 0;

	if (op->len == 0)
		return SPOCP_SYNTAXERROR;

	/*
	 * Why ? Is this still needed ? 
	 */
	if (*op->val == ')') {
		op->val++;
		op->len--;
		*epp = 0;
		return SPOCP_SUCCESS;
	}

	*epp = ep = element_new();

	if (*op->val == '(') {	/* start of list or set */
		base = op->val;

		op->val++;
		op->len--;

		if ((r = get_str(op, &oct)) == SPOCP_SUCCESS) {

			if (oct.len == 1 && oct.val[0] == '*') {	/* not 
									 * a
									 * simple 
									 * list, 
									 * star 
									 * expression 
									 */

				if (*op->val == ')') {
					ep->type = SPOC_ANY;	/* that's all
								 * that is
								 * needed */
					op->val++;
					op->len--;
					r = SPOCP_SUCCESS;
				} else if ((r = get_str(op, &oct)) ==
					   SPOCP_SUCCESS) {

					/*
					 * unless proven otherwise 
					 */
					r = SPOCP_SYNTAXERROR;

					switch (oct.len) {
					case 6:
						if (strncmp
						    (oct.val, "prefix",
						     6) == 0)
							r = do_prefix(op, ep);
						else if (strncmp
							 (oct.val, "suffix",
							  6) == 0)
							r = do_suffix(op, ep);
						break;

					case 5:
						if (strncmp
						    (oct.val, "range", 5) == 0)
							r = do_range(op, ep);
						break;

					case 3:
						if (strncmp(oct.val, "set", 3)
						    == 0)
							r = do_set(op, ep);
						break;
					}
				}
			} else
				r = do_list(&oct, op, ep, base);

		}
	} else {		/* has to be a atom thingy */
		ep->type = SPOC_ATOM;
		if ((ep->e.atom = get_atom(op, &r)) == 0) {
			element_free(ep);
			return r;
		}
	}

	if (r != SPOCP_SUCCESS) {
		element_free(ep);
		*epp = 0;
	}

	return r;
}

/*
 * ============================================================================ 
 */
/*
 * ============================================================================ 
 */
