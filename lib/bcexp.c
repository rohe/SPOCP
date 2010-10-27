/*
 *  bcexp.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/4/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#include <octet.h>
#include <varr.h>
#include <bcond.h>

#include <log.h>
#include <wrappers.h>

/*
 * ---------------------------------------------------------------------- 
 */

static bcexp_t *
bcexp_new()
{
    return (bcexp_t *) Calloc(1, sizeof(bcexp_t));
}

/*
 * ---------------------------------------------------------------------- 
 */

bcexp_t *
varr_bcexp_pop(varr_t * va)
{
    return (bcexp_t *) varr_pop(va);
}

/*
 * ---------------------------------------------------------------------- 
 */

void
bcexp_free(bcexp_t * bce)
{
    bcexp_t        *bp;
    
    if (bce) {
        switch (bce->type) {
            case AND:
            case OR:
                while ((bp = varr_bcexp_pop(bce->val.arr)) != 0)
                    bcexp_free(bp);
                if (bce->val.arr)
                    Free(bce->val.arr);
                break;
                
            case NOT:
                bcexp_free(bce->val.single);
                break;
                
            case REF:
                break;
                
            case SPEC:
                bcspec_free(bce->val.spec);
            default:
                break;
                
        }
        Free(bce);
    }
}

/*
 * ----------------------------------------------------------------------------- 
 */

void
bcstree_free(bcstree_t * stp)
{
    if (stp) {
        if (stp->part)
            bcstree_free(stp->part);
        if (stp->next)
            bcstree_free(stp->next);
        if (stp->val.size)
            Free(stp->val.val);
        Free(stp);
    }
}

/*
 * bcond = bcondexpr / bcondref
 * bcondexpr = bcondspec / bcondvar
 * bcondvar = bcondor / bcondand / bcondnot / bcondref
 * bcondspec = bcondname ":" typespecific
 * bcondname = 1*dirchar
 * typespecific = octetstring
 * bcondref = "(" "ref" 1*dirchar ")"
 * bcondand = "(" "and" 1*bcondvar ")"
 * bcondor = "(" "or" 1*bcondvar ")"
 * bcondnot = "(" "not" bcondvar ")"
 *
 */

bcstree_t *
parse_bcexp(octet_t * sexp)
{
    bcstree_t   *ptp, *ntp = 0, *ptr;
    
    if (*sexp->val == '(') {
        ptp = (bcstree_t *) Calloc(1, sizeof(bcstree_t));
        ptp->list = 1;
        sexp->val++;
        sexp->len--;
        while (sexp->len && *sexp->val != ')') {
            if ((ptr = parse_bcexp(sexp)) == 0) {
                bcstree_free(ptp);
                return 0;
            }
            
            if (ptp->part == 0) {
                if (ptr->list == 0) {
                    if (oct2strcmp( &ptr->val, "ref") == 0 );
                    else if (oct2strcmp( &ptr->val, "and") == 0 );
                    else if (oct2strcmp( &ptr->val, "or") == 0 );
                    else if (oct2strcmp( &ptr->val, "not") == 0 );
                    else {
                        bcstree_free(ptr);
                        return 0;
                    }
                }
                else {
                    bcstree_free(ptr);
                    return 0;
                }
                
                ntp = ptp->part = ptr;
            } else {
                ntp->next = ptr;
                ntp = ntp->next;
            }
        }
        
        if (*sexp->val == ')') {
            sexp->val++;
            sexp->len--;
        } else {    /* error */
            bcstree_free(ptp);
            return 0;
        }
    } else {
        ptp = (bcstree_t *) Calloc(1, sizeof(bcstree_t));
        if (get_str(sexp, &ptp->val) != SPOCP_SUCCESS) {
            bcstree_free(ptp);
            return 0;
        }
    }
    
    return ptp;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*!
 * \brief Translates a stree representation of a boundary condition expression
 *      into a bcexp struct
 * \param plt Pointer to the list of known plugins
 * \param st The stree that is to be turned into a bounday condition 
 *      definition
 * \param list The list of the present boundary condition definitions
 * \param parent
 * \return a struct representing a boundary expression
 */
bcexp_t *
transv_stree(plugin_t * plt, bcstree_t * st, bcdef_t * list, bcdef_t * parent)
{
    bcexp_t        *bce, *tmp;
    bcdef_t        *bcd;
    bcspec_t       *bcs;
    
    if (st == NULL)
        return NULL;
        
    if (!st->list) {
        if (!st->next) {    /* should be a bcond spec */
            if (plt == 0 || (bcs = bcspec_new(plt, &st->val)) == 0) {
                if (plt == 0)
                    traceLog(LOG_ERR,
                             "Reference to plugin while non is defined");
                return 0;
            }
            bce = bcexp_new();
            bce->type = SPEC;
            bce->parent = parent;
            bce->val.spec = bcs;
        } else {
            if (oct2strcmp(&st->val, "ref") == 0) {
                /*
                 * if the ref isn't there this def isn't valid 
                 */
                if ((bcd =
                     bcdef_find(list, &st->next->val)) == 0)
                    return 0;
                bce = bcexp_new();
                bce->type = REF;
                bce->parent = parent;
                bce->val.ref = bcd;
                /*
                 * so I know who to notify if anything happens 
                 * to this ref 
                 */
                bcd->users = varr_add(bcd->users, bce);
            } else if (oct2strcmp(&st->val, "and") == 0) {
                bce = bcexp_new();
                bce->type = AND;
                bce->parent = parent;
                for (st = st->next; st; st = st->next) {
                    if ((tmp =
                         transv_stree(plt, st, list,
                                      parent)) == 0) {
                             bcexp_free(bce);
                             return 0;
                         }
                    bce->val.arr =
                    varr_add(bce->val.arr,
                             (void *) tmp);
                }
            } else if (oct2strcmp(&st->val, "or") == 0) {
                bce = bcexp_new();
                bce->type = OR;
                bce->parent = parent;
                for (st = st->next; st; st = st->next) {
                    if ((tmp =
                         transv_stree(plt, st, list,
                                      parent)) == 0) {
                             bcexp_free(bce);
                             return 0;
                         }
                    bce->val.arr =
                    varr_add(bce->val.arr,
                             (void *) tmp);
                }
            } else if (oct2strcmp(&st->val, "not") == 0) {
                if (st->next->next)
                    return 0;
                bce = bcexp_new();
                bce->type = NOT;
                bce->parent = parent;
                if ((bce->val.single =
                     transv_stree(plt, st->next, list,
                                  parent)) == 0) {
                         Free(bce);
                         return 0;
                     }
            } else
                return 0;
        }
        
        return bce;
    } else
        return transv_stree(plt, st->part, list, parent);
}

/*
 * ---------------------------------------------------------------------- 
 */

static void
bcexp_deref(bcexp_t * bce)
{
    bcdef_t        *bcd;
    
    bcd = bce->val.ref;
    
    bcd->users = varr_rm(bcd->users, bce);
}

/*
 * ---------------------------------------------------------------------- 
 */

void
bcexp_delink(bcexp_t * bce)
{
    int             i, n;
    varr_t         *va;
    
    switch (bce->type) {
        case SPEC:
            break;
            
        case AND:
        case OR:
            va = bce->val.arr;
            n = varr_len(va);
            for (i = 0; i < n; i++)
                bcexp_delink((bcexp_t *) varr_nth(va, i));
            break;
            
        case NOT:
            bcexp_delink(bce->val.single);
            break;
            
        case REF:
            bcexp_deref(bce);
            break;
            
    }
}

/*
 * ---------------------------------------------------------------------- 
 */

void
bcdef_free(bcdef_t * bcd)
{
    bcexp_t        *bce;
    
    if (bcd) {
        if (bcd->name)
            Free(bcd->name);
        if (bcd->exp)
            bcexp_free(bcd->exp);
        while (bcd->users && bcd->users->n) {
            bce = varr_bcexp_pop(bcd->users);
            /*
             * don't free the bce it's just a pointer 
             */
        }
        
        if (bcd->users)
            Free(bcd->users);
    }
}

/*
 * ---------------------------------------------------------------------- 
 */

bcdef_t *
bcdef_new(void)
{
    bcdef_t        *new;
    
    new = (bcdef_t *) Calloc(1, sizeof(bcdef_t));
    
    return new;
}

/*
 * ---------------------------------------------------------------------- 
 */

bcdef_t *
bcdef_find(bcdef_t * bcd, octet_t * pattern)
{
    for (; bcd; bcd = bcd->next)
        if (oct2strcmp(pattern, bcd->name) == 0)
            return bcd;
    
    return 0;
}

/*
 * ---------------------------------------------------------------------- 
 */

bcspec_t *
bcspec_new(plugin_t * plt, octet_t * spec)
{
    octet_t         oct, namn;
    bcspec_t       *bcs = 0;
    plugin_t       *p;
    int             n;
    
    /*
     * two parts: <name> ":" <typespec> 
     */
    if ((n = octchr(spec, ':')) < 0) {
        oct_print( LOG_ERR,
                  "Error in boundary condition specification", spec);
        return 0;
    }
    
    octln(&oct, spec);
    oct.len -= n + 1;
    oct.val += n + 1;
    /* make it non intrusive */
    octln(&namn, spec);
    namn.len = n;
    
    if ((p = plugin_match(plt, &namn)) == 0) {
        traceLog(LOG_ERR,"Doesn't match any known plugin:",&namn);
        return 0;
    }
    
    bcs = (bcspec_t *) Calloc(1,sizeof(bcspec_t));
    
    bcs->plugin = p;
    bcs->name = oct2strdup(&namn, 0);
    bcs->spec = octdup(&oct);
    
    return bcs;
}

/*
 * ---------------------------------------------------------------------- 
 */

void
bcspec_free(bcspec_t * bcs)
{
    if (bcs) {
        if (bcs->name)
            Free(bcs->name);
        if (bcs->args)
            octarr_free(bcs->args);
        if (bcs->spec)
            oct_free(bcs->spec);
        
        Free(bcs);
    }
}


