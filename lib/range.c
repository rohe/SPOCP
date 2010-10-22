/*
 *  range.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/4/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#include <config.h>

#include <atom.h>
#include <range.h>
#include <element.h>
#include <spocp.h>
#include <verify.h>
#include <basic.h>
#include <wrappers.h>
#include <log.h>

/* #define AVLUS 1 */

/*
 * what's invalid ? 1) the upper boundary being lower than the lower 2) not
 * allowing the only possible value to be within the boundaries equivalent to
 * ( x > 5 && x < 5 )
 * 
 * Returns: FALSE if invalid TRUE if valid 
 */

static spocp_result_t
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
                c = ipv4cmp(&rp->upper.v.v4, &rp->lower.v.v4);
                break;
                
#ifdef USE_IPV6
            case SPOC_IPV6:
                c = ipv6cmp(&rp->upper.v.v6, &rp->lower.v.v6);
                break;
#endif
                
            case SPOC_ALPHA:
            case SPOC_DATE:
                c = octcmp(&rp->upper.v.val, &rp->lower.v.val);
                break;
                
            default:
                return SPOCP_UNKNOWN_TYPE;
		}
        
		if (c < 0) {
			LOG(SPOCP_ERR)
            traceLog(LOG_ERR,"Upper limit less then lower");
			r = SPOCP_SYNTAXERROR;
		} else if (c == 0 && !(rp->upper.type & EQ)
                   && !(rp->lower.type & EQ)) {
			LOG(SPOCP_ERR)
            traceLog(LOG_ERR,
                     "Upper limit equal to lower when it shouldn't");
			r = SPOCP_SYNTAXERROR;
		}
	}
    
	return r;
}

void
range_free(range_t * rp)
{
	if (rp) {
		switch (rp->lower.type) {
            case SPOC_DATE:
            case SPOC_ALPHA:
                if (rp->lower.v.val.len)
                    Free(rp->lower.v.val.val);
                if (rp->upper.v.val.len)
                    Free(rp->upper.v.val.val);
                break;
                
            case SPOC_TIME:
            case SPOC_NUMERIC:
            case SPOC_IPV4:
            case SPOC_IPV6:
                break;
		}
        
		Free(rp);
        rp = NULL ;
	}
}

boundary_t     *
set_delimiter(range_t * range, octet_t oct)
{
	boundary_t     *bp = 0;
    
	if (oct.len != 2)
		return 0;
    
	switch (oct.val[0]) {
        case 'g':
            /* not allowed to overwrite limit type */
            if (range->lower.type & 0xF0) {
                return NULL;
            }
            switch (oct.val[1]) {
                case 'e':
                    range->lower.type |= (GT | EQ);
                    bp = &range->lower;
                    break;
                    
                case 't':
                    range->lower.type |= GT;
                    bp = &range->lower;
                    break;
            }
            break;
            
        case 'l':
            /* not allowed to overwrite */
            if (range->upper.type & 0xF0) {
                return NULL;
            }
            switch (oct.val[1]) {
                case 'e':
                    range->upper.type |= (LT | EQ);
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

spocp_result_t range_limit(octet_t *op, range_t *rp) 
{
	octet_t         oct;
    spocp_result_t  r;
    boundary_t      *bp;
    /* char            *c; */
    
    if ((r = get_str(op, &oct)) != SPOCP_SUCCESS)
        return r;

    /* print_oct_info("[range_limit] delimiter", &oct); */
    
    if ((bp = set_delimiter(rp, oct)) == 0) {
        LOG(SPOCP_ERR) traceLog(LOG_ERR,"Error in delimiter specification");
        return SPOCP_SYNTAXERROR;
    }

    /*
     * and then some value 
     * it can't be a external reference 
     */

    if ((r = get_str(op, &oct)) != SPOCP_SUCCESS)
        return r;

    /*
    print_oct_info("[range_limit] value octet", &oct);
    oct_print(0, "[range_limit] value", &oct);
    printf("[range_limit] type: %d\n", bp->type & 0x07);
     */
    
    if ((r = set_limit(bp, &oct)) != SPOCP_SUCCESS)
        return r;
    
    /*
    c = boundary_prints(bp);
    printf("boundary: %s\n", c); 
    Free(c);
     */
    
    return SPOCP_SUCCESS;
}

range_t *set_range(octet_t * op)
{
	octet_t         oct;
	range_t        *rp = 0;
	int             r = SPOCP_SUCCESS;
    
	DEBUG(SPOCP_DPARSE) traceLog(LOG_DEBUG,"Parsing range");
    
    /*oct_print(0, "[set_range]", op); */
	/*
	 * next part should be type specifier 
	 */
    
	if ((r = get_str(op, &oct)) != SPOCP_SUCCESS)
		return NULL;
    
    /* oct_print(0, "[set_range] type", &oct); */
	DEBUG(SPOCP_DPARSE) traceLog(LOG_DEBUG,"new_range");
	rp = (range_t *) Calloc(1, sizeof(range_t));
        
	switch (oct.len) {
        case 4:
            if (strncasecmp(oct.val, "date", 4) == 0)
                rp->lower.type = SPOC_DATE;
            else if (strncasecmp(oct.val, "ipv4", 4) == 0)
                rp->lower.type = SPOC_IPV4;
#ifdef USE_IPv6
            else if (strncasecmp(oct.val, "ipv6", 4) == 0)
                rp->lower.type = SPOC_IPV6;
#endif
            else if (strncasecmp(oct.val, "time", 4) == 0)
                rp->lower.type = SPOC_TIME;
            else {
                LOG(SPOCP_ERR) traceLog(LOG_ERR,"Unknown range type");
                r = SPOCP_SYNTAXERROR;
            }
            break;
            
        case 5:
            if (strncasecmp(oct.val, "alpha", 5) == 0)
                rp->lower.type = SPOC_ALPHA;
            else {
                LOG(SPOCP_ERR) traceLog(LOG_ERR,"Unknown range type");
                r = SPOCP_SYNTAXERROR;
            }
            break;
            
        case 7:
            if (strncasecmp(oct.val, "numeric", 7) == 0)
                rp->lower.type = SPOC_NUMERIC;
            else {
                LOG(SPOCP_ERR) traceLog(LOG_ERR,"Unknown range type");
                r = SPOCP_SYNTAXERROR;
            }
            break;
            
        default:
            LOG(SPOCP_ERR) traceLog(LOG_ERR,"Unknown range type");
            r = SPOCP_SYNTAXERROR;
            break;
	}
    
	if (r == SPOCP_SYNTAXERROR) {
		range_free(rp);
        return NULL;
    }
    
	rp->upper.type = rp->lower.type;
    /* printf("[set_range] type: %d\n", rp->upper.type); */
	if (*op->val == ')') {	/* no limits defined */
		rp->lower.v.num = 0;
		rp->upper.v.num = 0;
	}
    else {
        r = range_limit(op, rp);
        /* printf("[range_limit](0) result: %d\n", r); */
    
        if (r == SPOCP_SUCCESS) {
            if (*op->val != ')') {
                r = range_limit(op, rp); 
                /* printf("[range_limit](1) result: %d\n", r); */
        
                if (r == SPOCP_SUCCESS && *op->val != ')') {
                    LOG(SPOCP_ERR) traceLog(LOG_ERR,"Missing closing ')'");
                    r = SPOCP_SYNTAXERROR;
                }
            }
        }
        /* printf("[set_range] check if valid\n"); */
        if (r == SPOCP_SUCCESS) {
            r = is_valid_range(rp) ;
        }
    }
        
	if (r != SPOCP_SUCCESS) {
		range_free(rp);
        return NULL;
	} else {
        /*
        char *l, *u ;
        printf("-boundaries-\n");
        u = boundary_prints( &rp->upper ) ; 
        printf("upper: %s\n",u);
        l = boundary_prints( &rp->lower ) ;
        printf("lower: %s\n",l);
        */
		op->val++;
		op->len--;
        return rp;
	}
    
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
            if (rp->lower.v.num == rp->upper.v.num)
                sr = SPOCP_SUCCESS;
            break;
            
        case SPOC_IPV4:
            l = (uint32_t *) & rp->lower.v.v4;
            u = (uint32_t *) & rp->upper.v.v4;
            if (*l == *u)
                sr = SPOCP_SUCCESS;
            break;
            
#ifdef USE_IPV6
        case SPOC_IPV6:
            l = (uint32_t *) & rp->lower.v.v6;
            u = (uint32_t *) & rp->upper.v.v6;
            if (l[0] == u[0] && l[1] == u[1] && l[2] == u[2]
                && l[3] == u[3])
                sr = SPOCP_SUCCESS;
            break;
#endif
            
        case SPOC_ALPHA:
        case SPOC_DATE:
            if (rp->lower.v.val.len == rp->upper.v.val.len) {
                if (rp->lower.v.val.len != 0 && 
                    octcmp(&rp->lower.v.val, &rp->upper.v.val) == 0)
                    sr = SPOCP_SUCCESS;
            }
            break;
	}
    
	return sr;
}

atom_t *
get_atom_from_range_definition(octet_t *op)
{
    octet_t tmp;
    
    /* step by the type and the delimiter info */
    get_str(op, &tmp);
    get_str(op, &tmp);
    /* and then the value */
    get_str(op, &tmp);
    return atom_new(&tmp);    
}

int
range_print(octet_t *oct, range_t *r)
{
    if( boundary_oprint( oct, &r->lower) == 0)
        return 0;
    if( boundary_oprint( oct, &r->upper) == 0)
        return 0;
    
    return 1;
}
