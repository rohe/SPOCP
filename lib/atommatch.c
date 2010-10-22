/*
 *  atommatch.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/9/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#include <config.h>

#include <string.h>

#include <octet.h>
#include <hash.h>
#include <atom.h>
#include <atommatch.h>
#include <verify.h>
#include <branch.h>

#include <wrappers.h>
#include <macros.h>
#include <log.h>

varr_t         *
get_all_atom_followers(branch_t * bp, varr_t * in)
{
    phash_t        *ht = bp->val.atom;
    buck_t        **arr = ht->arr;
    unsigned int    i;
    
    for (i = 0; i < ht->size; i++) {
        if (arr[i])
            in = varr_junc_add(in, arr[i]->next);
    }
    
    return in;
}

varr_t         *
prefix2atoms_match(char *prefix, phash_t * ht, varr_t * pa)
{
    buck_t        **arr = ht->arr;
    unsigned int    i;
    size_t          len = strlen(prefix);
    
    for (i = 0; i < ht->size; i++)
        if (arr[i] != NULL && strncmp(arr[i]->val.val, prefix, len) == 0)
            pa = varr_junc_add(pa, arr[i]->next);
    
    return pa;
}

varr_t         *
suffix2atoms_match(char *suffix, phash_t * ht, varr_t * pa)
{
    buck_t        **arr = ht->arr;
    unsigned int    i;
    size_t          len = strlen(suffix), l;
    char           *s;
    
    for (i = 0; i < ht->size; i++) {
        if (arr[i]) {
            s = arr[i]->val.val;
            
            l = strlen(s);
            if (l >= len) {
                if (strncmp(&s[l - len], suffix, len) == 0)
                    pa = varr_junc_add(pa, arr[i]->next);
            }
        }
    }
    
    return pa;
}

varr_t         *
range2atoms_match(range_t * rp, phash_t * ht, varr_t * pa)
{
    buck_t      **arr = ht->arr;
    int         r, i, dtype = rp->lower.type & RTYPE;
    int         ll = rp->lower.type & BTYPE;
    int         ul = rp->upper.type & BTYPE;
    octet_t     *op, *lo = 0, *uo = 0;
    long        li=0, lv = 0, uv = 0;
    int         d;
    
    struct in_addr *v4l, *v4u, ia;
#ifdef USE_IPV6
    struct in6_addr *v6l, *v6u, i6a;
#endif
    
    switch (dtype) {
        case SPOC_NUMERIC:
        case SPOC_TIME:
            lv = rp->lower.v.num;
            uv = rp->upper.v.num;
            break;
            
        case SPOC_ALPHA:
        case SPOC_DATE:
            lo = &rp->lower.v.val;
            uo = &rp->upper.v.val;
            break;
            
        case SPOC_IPV4:
            v4l = &rp->lower.v.v4;
            v4u = &rp->upper.v.v4;
            break;
            
#ifdef USE_IPV6
        case SPOC_IPV6:
            v6l = &rp->lower.v.v6;
            v6u = &rp->upper.v.v6;
            break;
#endif
    }
    
    for (i = 0; i < (int) ht->size; i++) {
        if (arr[i]) {
            op = &arr[i]->val;
            r = 0;
            
            switch (dtype) {
                case SPOC_NUMERIC:
                case SPOC_TIME:
                    if (is_numeric(op, &li) == SPOCP_SUCCESS) {
                        /*
                         * no upper or lower limits 
                         */
                        if (ll == 0 && ul == 0)
                            r = 1;
                        /*
                         * no upper limits 
                         */
                        else if (ul == 0) {
                            if ((ll == (GT|EQ) && li >= lv)
                                || (ll == GT && li > lv))
                                r = 1;
                        }
                        /*
                         * no upper limits 
                         */
                        else if (ll == 0) {
                            if ((ul == (LT|EQ) && li <= uv)
                                || (ul == LT && li < uv))
                                r = 1;
                        }
                        /*
                         * upper and lower limits 
                         */
                        else {
                            if (((ll == (GT|EQ) && li >= lv)
                                 || (ll == GT && li > lv))
                                && ((ul == (LT|EQ) && li <= uv)
                                    || (ul == LT && li < uv)))
                                r = 1;
                        }
                    }
                    break;
                
                case SPOC_DATE: 
                    if (is_date(op) != SPOCP_SUCCESS)
                        break;

                 /*@fallthrough@*/
                case SPOC_ALPHA:
                    /*
                     * no upper or lower limits 
                     */
                    if (lo == 0 && uo == 0)
                        r = 1;
                    /*
                     * no upper limits 
                     */
                    else if (uo == NULL || uo->len == 0) {
                        d = octcmp(op, lo);
                        if ((ll == (GT|EQ) && d >= 0) || (ll == GT && d > 0))
                            r = 1;
                    }
                    /*
                     * no lower limits 
                     */
                    else if (lo == NULL || lo->len == 0) {
                        d = octcmp(op, uo);
                        if ((ul == (LT|EQ) && d <= 0) || (ul == LT && d < 0))
                            r = 1;
                    }
                    /*
                     * upper and lower limits 
                     */
                    else {
                        d = octcmp(op, lo);
                        if ((ll == (GT|EQ) && d >= 0) || (ll == GT && d > 0)) {
                            d = octcmp(op, uo);
                            if ((ul == (LT|EQ) && d <= 0) || 
                                (ul == LT && d < 0))
                                r = 1;
                        }
                    }
                    break;
                    
                case SPOC_IPV4:
                    memset(&ia, 0, sizeof(struct in_addr));
                    if (is_ipv4(op, &ia) != SPOCP_SUCCESS)
                        break;
                    /*
                     * MISSING CODE 
                     */
                    break;
                    
#ifdef USE_IPV6
                case SPOC_IPV6:
                    memset(&i6a, 0, sizeof(struct in_6addr));
                    if (is_ipv6(op, &i6a) != SPOCP_SUCCESS)
                        break;
                    /*
                     * MISSING CODE 
                     */
                    break;
#endif
            }
            
            if (r != 0) {
                /* oct_print(0, "--", op); */
                pa = varr_junc_add(pa, arr[i]->next);
            }
        }
    }
    
    return pa;
}
