/*
 *  boundary.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/4/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#include <config.h>

#include <boundary.h>
#include <element.h>
#include <verify.h>
#include <basic.h>

#include <wrappers.h>
#include <log.h>

/************************************************************************/

boundary_t *boundary_new(int type)
{
    boundary_t  *bp;

    bp = Calloc(1, sizeof(boundary_t));
    bp->type = type;
    bp->dynamic = 1;
    
    return bp;
}

void
boundary_free(boundary_t * bp)
{
    if (bp) {
        switch (bp->type & 0x07) {
            case SPOC_ALPHA:
            case SPOC_DATE:
                if (bp->v.val.len)
                    Free(bp->v.val.val);
                break;
                
            case SPOC_NUMERIC:
            case SPOC_TIME:
            case SPOC_IPV4:
            case SPOC_IPV6:
                break;
        }
        
        if (bp->dynamic)
            Free(bp);
    }
}

/*
 */

spocp_result_t
set_limit(boundary_t * bp, octet_t * op)
{
    int r = SPOCP_SYNTAXERROR;
    
    switch (bp->type & 0x07) {
        case SPOC_NUMERIC:
            r = is_numeric(op, &bp->v.num);
            break;
            
        case SPOC_ALPHA:
           
            /*
             print_oct_info("[set_limit] bp->val", &bp->v.val);
             print_oct_info("[set_limit] op", op);
             */
            octcpy( &bp->v.val, op);
            /*
             print_oct_info("[set_limit] &bp->v.val", &bp->v.val);
             print_oct_info("[set_limit] op", op);
             printf("alpha - stored\n");
             */
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
            
#ifdef USE_IPv6
        case SPOC_IPV6:
            r = is_ipv6(op, &bp->v.v6);
            break;
#endif
    }
    
    /*printf("[set_limit] r=%d\n", r);*/
    return r;
}

/*
 * returns 
 *  1  if a > b, 
 *  0  if a == b,
 * -1  if a < b and  
 * -2  if a and b are of different types and therefor can not be compared.
 *
 * When it comes to '<', '<=', '>=' and '>', they are ordered in 
 * that order. 
 */


int
boundarycmp( boundary_t *a, boundary_t *b, spocp_result_t *rc)
{
    int     v = 0;
    
    if (b == NULL)
        return -1;
    if (( a->type & RTYPE) != (b->type & RTYPE)) {
        *rc = SPOCP_LOCAL_ERROR;
        return -2;
    }
    
    switch (b->type & RTYPE) {
        case SPOC_ALPHA:
        case SPOC_DATE:
            v = octcmp(&a->v.val, &b->v.val);
            break;
            
        case SPOC_TIME:
        case SPOC_NUMERIC:
            v = a->v.num - b->v.num;
            break;
            
        case SPOC_IPV4:
            v = ipv4cmp(&a->v.v4, &b->v.v4);
            break;
            
#ifdef USE_IPV6
        case SPOC_IPV6: /* Can be viewed as a array of unsigned ints */
            v = ipv6cmp(&a->v.v6, &b->v.v6);
            break;
#endif
    }
    if (v < 0) {
        v = -1;
    } else {
        if (v > 0) {
            v = 1;
        }
        else { /* only if the values are the same do I look at the lt part */
            int blg = (b->type & BTYPE);
            int alg = (a->type & BTYPE);
            if (blg == alg)
                v = 0;
            else {
                switch (alg) {
                    case 0x10: /* < */
                        v = -1;
                        break;
                    case 0x30:  /* <= */
                        if (blg == 0x10)
                            v = 1;
                        else
                            v = -1;
                        break;
                    case 0x60:  /* >= */
                        if (blg < alg)
                            v = 1;
                        else
                            v = -1;
                        break;
                    case 0x40:  /* > */
                        v = 1;
                        break;
                    default:
                        break;
                }
            }

        }
    }
    
    return v;
}

/*
 */

int
boundary_cpy(boundary_t * dest, boundary_t * src)
{
    int     i;
    char       *a, *b;
    
    switch (src->type & 0x07) {
        case SPOC_ALPHA:
        case SPOC_DATE:
            if (src->v.val.len) {
                dest->v.val.val =
                Strndup(src->v.val.val, src->v.val.len);
                dest->v.val.len = src->v.val.len;
            }
            break;
            
        case SPOC_NUMERIC:
        case SPOC_TIME:
            dest->v.num = src->v.num;
            break;
            
        case SPOC_IPV4:
            a = (char *) &dest->v.v4;
            b = (char *) &src->v.v4;
            for (i = 0; i < (int) sizeof(struct in_addr); i++)
                *a++ = *b++;
            break;
            
#ifdef USE_IPV6
        case SPOC_IPV6:
            a = (char *) &dest->v.v6;
            b = (char *) &src->v.v6;
            for (i = 0; i < (int) sizeof(struct in6_addr); i++)
                *a++ = *b++;
            break;
#endif
    }
    
    dest->type = src->type;
    
    return 0;
}

/*
 */

char *
boundary_prints(boundary_t * bp)
{
    char    ip[65], *lte = "-", *str = NULL ;
    
    if (bp == 0)
        return Strdup("");

    if (bp->type == 0 && bp->v.val.len == 0)
        return Strdup("");
    
    switch (bp->type & 0xF0) {
        case LT | EQ:
            lte = "<=";
            break;
        case GT | EQ:
            lte = ">=";
            break;
        case EQ:
            lte = "=";
            break;
        case GT:
            lte = ">";
            break;
        case LT:
            lte = "<";
            break;
    }

    switch (bp->type & 0x07) {
        case SPOC_ALPHA:
        case SPOC_DATE:
            if (bp->v.val.len) {
                char *tmp;
                tmp = oct2strdup(&bp->v.val, 0);
                str = Calloc( 5+bp->v.val.len, sizeof(char));
                sprintf(str,"%s %s", lte, tmp);
                free(tmp);
            }
            else {
                str = Calloc( 10, sizeof(char));
                sprintf(str,"%s NULL", lte);
            }
            break;
            
        case SPOC_TIME:
        case SPOC_NUMERIC:
            str = Calloc( 32, sizeof(char));
            sprintf(str,"%s %ld", lte, bp->v.num);
            break;
            
        case SPOC_IPV4:
#ifdef HAVE_INET_NTOP
            inet_ntop(AF_INET, (void *) &bp->v.v4, ip, 65);
#else
        {
            char    *tmp;
            tmp = inet_ntoa( bp->v.v4 );
            strlcat( ip, tmp, sizeof(op));
        }
#endif
            str = Calloc( 32, sizeof(char));
            sprintf(str, " %x %s %s", bp->type, lte, ip);
            break;
            
#ifdef USE_IPV6
        case SPOC_IPV6:
            inet_ntop(AF_INET6, (void *) &bp->v.v6, ip, 65);
            str = Calloc( 96, sizeof(char));
            sprintf(str, " %s %s", lte, ip);
            break;
#endif
            
    }
    
    return str;
}

/*
 */

boundary_t     *
boundary_dup(boundary_t * bp)
{
    boundary_t     *nbp;
    int             i;
    char           *a, *b;
    
    if (bp == 0)
        return 0;
    
    nbp = boundary_new(0);
    
    switch (bp->type & 0x07) {
        case SPOC_ALPHA:
        case SPOC_DATE:
            if (bp->v.val.len) 
                octcpy( &nbp->v.val, &bp->v.val);
            break;
            
        case SPOC_NUMERIC:
        case SPOC_TIME:
            nbp->v.num = bp->v.num;
            break;
            
        case SPOC_IPV4:
            a = (char *) &nbp->v.v4;
            b = (char *) &bp->v.v4;
            for (i = 0; i < (int) sizeof(struct in_addr); i++)
                *a++ = *b++;
            break;
            
#ifdef USE_IPV6
        case SPOC_IPV6:
            a = (char *) &nbp->v.v6;
            b = (char *) &bp->v.v6;
            for (i = 0; i < (int) sizeof(struct in6_addr); i++)
                *a++ = *b++;
            break;
#endif
    }
    
    nbp->type = bp->type;
    
    return nbp;
}

/************************************************************************/

int
boundary_xcmp(boundary_t * b1p, boundary_t * b2p)
{
    int             v = 0;    

    /*
     * NULL is a tail marker, everything is 'less' than the tail value 
     */
    if (b2p == 0)
        return -1;

    if (( b1p->type & RTYPE) != (b2p->type & RTYPE)) return -2;

#ifdef AVLUS
    DEBUG(SPOCP_DMATCH) {
        char *b1, *b2 ;
        
        traceLog(LOG_DEBUG,"boundary_xcmp (%d)", b2p->type) ;
        b1 = boundary_prints( b1p) ;
        b2 = boundary_prints( b2p) ; 
        traceLog(LOG_DEBUG, "%s <> %s", b1, b2 );
        Free(b1);
        Free(b2);
    }
#endif
    /*
     */
    
    switch (b2p->type & 0x07) {
        case SPOC_ALPHA:
        case SPOC_DATE:
            
            if (b1p->v.val.len == 0) {
                if (b2p->v.val.len == 0)
                    v = 0;
                else
                    v = -1;
            } else if (b2p->v.val.len == 0)
                v = 1;
            else {
                v = octcmp(&b1p->v.val, &b2p->v.val) ;
            }
            break;
            
        case SPOC_NUMERIC:
        case SPOC_TIME:
            v = b1p->v.num - b2p->v.num ;
#ifdef AVLUS
            traceLog(LOG_DEBUG,"Numeric %ld, %ld => %d", b1p->v.num,
                     b2p->v.num, v);
#endif
            break;
            
        case SPOC_IPV4:
            v = ipv4cmp(&b1p->v.v4, &b2p->v.v4);
            break;
            
#ifdef USE_IPV6
        case SPOC_IPV6: /* Can be viewed as a array of unsigned ints */
            v = ipv6cmp(&b1p->v.v6, &b2p->v.v6);
            break;
#endif
    }
    
    if (v < 0) {
        v = -1;
    } else {
        if (v > 0) {
            v = 1;
        }
        else {
            v = 0;
        }
    }
    
    if (!v)
        if (!(b1p->type & b2p->type)) v = 1;
    
#ifdef AVLUS
    DEBUG(SPOCP_DMATCH) {
        traceLog(LOG_DEBUG, "res: %d", v ) ; 
    }
#endif
    /*
     */
    
    return v;
}

int 
boundary_oprint( octet_t *oct, boundary_t *b)
{
    char    *val, spec[1024], *dyn=0;
    int     len, n=0;
    
    switch (b->type & 0x0F) {
        case SPOC_NUMERIC:
            val = Calloc(64, sizeof(char));
            sprintf(val, "%ld", b->v.num );
            break;
        case SPOC_ALPHA:
            len = b->v.val.len+1;
            val = Calloc(len, sizeof(char));
            oct2strcpy(&b->v.val, val, len, 0);
            break;

        default:
            return 1;
    }
        
    switch (b->type & 0xF0){
        case (GT|EQ):
            n = snprintf(spec, 1024, "2:ge%u:%s", (unsigned int) strlen(val),
                         val);
            break;
        case GT:
            n = snprintf(spec, 1024, "2:gt%u:%s", (unsigned int) strlen(val),
                         val);
            break;
        case LT|EQ:
            n = snprintf(spec, 1024, "2:le%u:%s", (unsigned int) strlen(val),
                         val);
            break;
        case LT:
            n = snprintf(spec, 1024, "2:lt%u:%s", (unsigned int) strlen(val),
                         val);
            break;
    }
    
    if (n < 0) { /* do something reasonable */
        dyn = Calloc(n+1, sizeof(char));
    }
    
    oct_free((void *) val);
    
    if( octcat( oct, spec, strlen(spec)) != SPOCP_SUCCESS)
        return 0; 
    
    return 1;
}

