#include <stdlib.h>
#include <string.h>

#include <element.h>
#include <range.h>
#include <wrappers.h>
#include <log.h>

/* #define _ELEMENT_DEBUG */

/*
 * ----------------------------------------------------------------------
 */

/* void element_reduce( element_t *ep ) ; */

element_t      *
element_new()
{
	element_t      *ep;
    
	ep = (element_t *) Calloc(1, sizeof(element_t));
#ifdef _ELEMENT_DEBUG
	traceLog(LOG_DEBUG,"New element: %p", (void *) ep);
#endif
    
	return ep;
}

/*
 * ----------------------------------- 
 */

list_t  *
list_new(void)
{
	return (list_t *) Calloc(1, sizeof(list_t));
}

/*
 * ----------------------------------------------------------------------
 */

static int
P_element_cmp( void *a, void *b, spocp_result_t *rc )
{
    return element_cmp( (element_t *) a, (element_t *) b, rc );
}

static void
P_element_free( void *a )
{
    element_free((element_t *) a );
}


/*
 * ----------------------------------------------------------------------
 * Reduce functions, removes duplicates from element sets
 * ----------------------------------------------------------------------
 */

static void
element_set_reduce( element_t *ep )
{
    varr_t      *va;
    size_t      i;
    element_t   *te;
#ifdef _ELEMENT_DEBUG
    octet_t     *op;
    char        *tmp;

    op = oct_new( 512, NULL);
    element_print( op, ep );
    tmp = oct2strdup( op, 0 );
    traceLog(LOG_DEBUG,"Reducing: [%s]", tmp );
    Free( tmp );
#endif

    varr_rm_dup( ep->e.set, P_element_cmp, P_element_free ); 

#ifdef _ELEMENT_DEBUG
    op->len = 0;
    element_print( op, ep );
    tmp = oct2strdup( op, 0 );
    traceLog(LOG_DEBUG, "1:st Reduction to [%s]", tmp);
    Free(tmp);
#endif

    va = ep->e.set;
    if( va->n > 1 ) {
        for( i = 0; i < va->n; i++) {
            if( va->arr[i] == 0 ) continue;

            te = (element_t *) va->arr[i];
            va->arr[i] = element_reduce(te);
        }
    }

#ifdef _ELEMENT_DEBUG
    op->len = 0;
    element_print( op, ep );
    tmp = oct2strdup( op, 0 );
    traceLog(LOG_DEBUG, "2:nd Reduction to [%s]", tmp);
    Free(tmp);
    oct_free( op );
#endif

}

/*
 * =============================================================== 
 */

element_t *
element_reduce( element_t *ep )
{
    element_t   *nep, *te, *prev, *next;
    
    if (ep == 0)
        return NULL;
    
    switch( ep->type ){
        case SPOC_LIST:
            prev = 0;
            for( ep = ep->e.list->head; ep ; ep = next ) {
                next = ep->next;
                nep = element_reduce( ep );
                if (nep == NULL)
                    continue;
                if (ep != nep) {
                    if( prev )
                        prev->next = nep;
                    nep->next = next;
                    ep = nep;
                    next = ep->next;
                }
                prev = ep;
            }
            break;
            
        case SPOC_SET:
            element_set_reduce( ep );
            if( ep->e.set->n == 1 ) {
                te = (element_t *) varr_pop( ep->e.set );
                element_free( ep );
                ep = te;
            }
            break;
    }
    
    return ep;
}

/*
 * =============================================================== 
 */

spocp_result_t
do_prefix(octet_t * oct, element_t * ep)
{
	spocp_result_t  rc = SPOCP_SUCCESS;
    
	DEBUG(SPOCP_DPARSE) traceLog(LOG_DEBUG,"Parsing 'prefix' expression");
    
	ep->type = SPOC_PREFIX;
    
	if ((ep->e.atom = get_atom(oct, &rc)) == 0)
		return rc;
    
	/*
	 * a check for external references, which is not allowed in ranges 
	 */
    
	if (*oct->val != ')') {
		LOG(SPOCP_ERR) traceLog(LOG_ERR,"Missing ending ')'");
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

void
set_memberof(varr_t * va, element_t * parent)
{
	void           *vp;
	element_t      *ep;
    
#ifdef _ELEMENT_DEBUG
	traceLog(LOG_DEBUG,"set_memberof = %p",(void *) parent);
#endif
    
	for (vp = varr_first(va); vp; vp = varr_next(va, vp)) {
		ep = (element_t *) vp;
		ep->memberof = parent;
	}
}

/*
 * =============================================================== 
 */

spocp_result_t
do_suffix(octet_t * oct, element_t * ep)
{
	spocp_result_t  rc = 0;
    
	DEBUG(SPOCP_DPARSE) traceLog(LOG_DEBUG,"Parsing 'suffix' expression");
    
	ep->type = SPOC_SUFFIX;
    
	if ((ep->e.atom = get_atom(oct, &rc)) == 0)
		return rc;
    
	if (*oct->val != ')') {
		LOG(SPOCP_ERR) traceLog(LOG_ERR,"Missing ending ')'");
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

static spocp_result_t
get_set(octet_t * oct, element_t * ep)
{
	element_t      *nep = 0, *parent=ep;
	varr_t         *pa, *sa = 0;
	spocp_result_t  rc;
    
	/*
	 * until end of star expression is found 
	 */
	while (oct->len && *oct->val != ')') {
        
		if ((rc = get_element(oct, &nep)) != SPOCP_SUCCESS) {
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
        /* oct_print(0, "[get_set] remainder", oct); */
	}
    
	DEBUG(SPOCP_DPARSE) traceLog(LOG_DEBUG,"Got end of set");
    
	/*
	 * only one item in the and expr, so it's not really a set. It's just
	 * a element and should therefore be treated as such 
	 */
	if (sa->n == 1) {
		ep = (element_t *) varr_pop(sa);
		varr_free(sa);
	} else {
		ep->e.set = sa;
		set_memberof(sa, parent);
	}
    
	if (*oct->val == ')') {
		oct->val++;
		oct->len--;
	}
    
    /* printf("[get_set] returned SPOCP_SUCCESS\n"); */
	return SPOCP_SUCCESS;
}

/*
 * -------------------------------------------------------------------------- 
 */

spocp_result_t
do_set(octet_t * oct, element_t * ep)
{
	DEBUG(SPOCP_DPARSE) traceLog(LOG_DEBUG,"Parsing 'set' expression");
    
	ep->type = SPOC_SET;
    
	return get_set(oct, ep);
}


/*
 * ============================================================================ 
 */

spocp_result_t
do_range(octet_t * op, element_t * ep)
{
	octet_t     oct;
	range_t     *rp = 0;
    
	DEBUG(SPOCP_DPARSE) traceLog(LOG_DEBUG,"Parsing range");

    /* store the starting point */    
    octln(&oct, op);
    
    rp = set_range(op);
    if (rp == NULL) {
        return SPOCP_SYNTAXERROR;
    }
    
	if (is_atom(rp) == SPOCP_SUCCESS) {
		ep->type = SPOC_ATOM;
		ep->e.atom = get_atom_from_range_definition(&oct);
		range_free(rp);
	} else {
        ep->type = SPOC_RANGE;    
        ep->e.range = rp;
    }
    
	return SPOCP_SUCCESS;
}

/*
 * ============================================================================ 
 */

spocp_result_t
do_list(octet_t * to, octet_t * lo, element_t * ep, char *base)
{
	spocp_result_t  r = SPOCP_SUCCESS;
	element_t      *nep = 0, *pep;
    
	DEBUG(SPOCP_DPARSE) traceLog(LOG_DEBUG,"List tag");
    
	ep->type = SPOC_LIST;
#ifdef _ELEMENT_DEBUG
	traceLog(LOG_DEBUG,"list_new");
#endif
	ep->e.list = list_new();
    
#ifdef _ELEMENT_DEBUG
	traceLog(LOG_DEBUG,"element_new");
#endif
	pep = ep->e.list->head = element_new();
    
	/*
	 * first element has to be a atom 
	 */
	pep->memberof = ep;
	pep->type = SPOC_ATOM;
#ifdef _ELEMENT_DEBUG
	traceLog(LOG_DEBUG,"atom_new");
#endif
	pep->e.atom = atom_new(to);
    
	/*
	 * get rest of the element 
	 */
    
	while (lo->len) {
		nep = 0;
        
		r = get_element(lo, &nep);
        
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
		traceLog(LOG_DEBUG,"Input end of list with tag [%s]", tmp);
		Free(tmp);
	}
    
	return SPOCP_SUCCESS;
}


/*
 * ======================================================================
 */

/*
 * S-expression format is (3:tag7:element) the so called canonical
 * representation, and not (tag element) the advanced format
 * 
 * which among other things means that spaces does not occur between elements 
 */

spocp_result_t
get_element(octet_t * op, element_t ** epp)
{
	char           *base;
	octet_t         oct;
	spocp_result_t  r = SPOCP_SUCCESS;
	element_t      *ep;
    
#ifdef _ELEMENT_DEBUG
	traceLog(LOG_DEBUG,"get_element()");
#endif

    /* oct_print(0, "[get_element]", op); */
    
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
        /* printf("[get_element] non atom\n"); */
		base = op->val;
        
		op->val++;
		op->len--;
        if (*op->val == ')'){
            ep->type = SPOC_ANY;
            op->val++;
            op->len--;
            r = SPOCP_SUCCESS;
        }
		else if ((r = get_str(op, &oct)) == SPOCP_SUCCESS) {
            /* oct_print(0, "[get_element] oct", &oct); */
			if (oct.len == 1 && oct.val[0] == '*') {
				/* not a simple list, star expression */
                /* printf("[get_element] starexpression\n"); */
				if ((r = get_str(op, &oct)) ==
                           SPOCP_SUCCESS) {
                    
					/*
					 * unless proven otherwise 
					 */
					r = SPOCP_SYNTAXERROR;
                    
					switch (oct.len) {
                        case 6:
                            if (oct2strcmp(&oct, "prefix") == 0)
                                r = do_prefix(op, ep);
                            else if (oct2strcmp(&oct, "suffix") == 0)
                                r = do_suffix(op, ep);
                            break;
                            
                        case 5:
                            if (oct2strcmp(&oct, "range") == 0) {
                                r = do_range(op, ep);
                            }
                            break;
                            
                        case 3:
                            if (oct2strcmp(&oct, "set") == 0)
                                r = do_set(op, ep);
                            break;
					}
				}
			} else {
                r = do_list(&oct, op, ep, base);
            }
		}
	} else {		/* has to be a atom thingy */
        /* printf("[get_element] atom\n"); */
		ep->type = SPOC_ATOM;
		if ((ep->e.atom = get_atom(op, &r)) == 0) {
			element_free(ep);
            *epp = NULL;
			return r;
		}
	}
    
	if (r != SPOCP_SUCCESS) {
		element_free(ep);
		*epp = 0;
	}
    
    /* printf("[get_element] returned: %d\n", r); */
	return r;
}

/*
 * --------------------------------------------------------------- 
 */

void
list_free(list_t * lp)
{
	element_t      *ep, *nep;
    
	if (lp) {
		for (ep = lp->head; ep; ep = nep) {
			nep = ep->next;
			element_free(ep);
		}
        
		Free(lp);
	}
}

/*
 * --------------------------------------------------------------- 
 */

void
element_free(element_t * ep)
{
	element_t	*e;
	varr_t		*va;
    
	if (ep) {
		switch (ep->type) {
            case SPOC_ATOM:
            case SPOC_PREFIX:
            case SPOC_SUFFIX:
                atom_free(ep->e.atom);
                break;
                
            case SPOC_LIST:
                list_free(ep->e.list);
                break;
                
            case SPOC_SET:
                va = ep->e.set;
                if (va) {
                    while ((e = varr_pop(va)))
                        element_free(e);
                    varr_free(va);
                }
                break;
                
            case SPOC_RANGE:
                range_free(ep->e.range);
                break;
                
		}
        
		Free(ep);
	}
}

