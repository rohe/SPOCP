/*!
 * \file lib/rvapi.c \author Roland Hedberg <roland@catalogi.se> 
 * \brief Functions that the backends can use to handle parsed S-expressions
 */

/***************************************************************************
              rvapi.c  -  description
                -------------------
    begin       : Thr Jan 29 2004
    copyright       : (C) 2004 by Stockholm university, Sweden
    email       : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "db0.h"
#include "proto.h"
#include "spocp.h"
#include "wrappers.h"
#include "macros.h"
#include "element.h"
#include "rvapi.h"

static varr_t   *list_search(element_t * e, octet_t * tag, varr_t * r);
element_t       *element_dup(element_t * ep);
/*
 * from input.c 
 */

/********************************************************************/

static atom_t  *
_atom_dup(atom_t * ap)
{
    atom_t   *new = 0;

    if (ap == 0)
        return 0;

    new = atom_new(&ap->val);

    return new;
}

/*
 * ====================================================================== 
 */
/*! \brief Joins a set of atoms with a separator in betweem into one string
 * of bytes. If given a list which contains both atoms and other elements all
 * non-atom elements will just be silently ignored.
 * \param e A pointer to a element structure which should contain either a 
 *  set or a list of elements each containing a atom.
 * \param sep The separator which is to be placed between the atom values
 * \return A pointer to a a octet struct which contains the result of the 
 *  concatenation 
 */
octet_t *
atoms_join(element_t * e, char *sep)
{
    char        *result = 0, *sp;
    int         n, size = 0, slen = 0;
    element_t   *ep;
    octet_t     *oct = 0;
    void        *v;
    varr_t      *va;

    if (e == 0)
        return 0;

    if (sep && *sep)
        slen = strlen(sep);

    switch (e->type) {
    case SPOC_SET:
    case SPOC_ARRAY:
        va = e->e.set;
        /* First get the necessary size of the string */
        for (v = varr_first(va), n = 0; v; v = varr_next(va, v)) {
            ep = (element_t *) v;
            if (ep->type == SPOC_ATOM) {
                size += ep->e.atom->val.len;
                n++;
            }
        }
        if (slen)
            size += (n - 1) * slen;

        result = (char *) Malloc(size * sizeof(char));
        /* Then fill it up */
        for (sp = result, n = 0, v = varr_first(va); v;
            v = varr_next(va, v)) {
            ep = (element_t *) v;
            if (ep->type == SPOC_ATOM) {
                if (n) {
                    memcpy(sp, sep, slen);
                    sp += slen;
                }
                memcpy(sp, ep->e.atom->val.val,
                    ep->e.atom->val.len);
                sp += ep->e.atom->val.len;
                n++;
            }
        }
        break;

    case SPOC_LIST:
        for (n = 0, ep = e->e.list->head; ep; ep = ep->next) {
            if (ep->type == SPOC_ATOM) {
                size += ep->e.atom->val.len;
                n++;
            }
        }
        if (slen)
            size += (n - 1) * slen;

        result = (char *) Malloc(size * sizeof(char));

        for (n = 0, sp = result, ep = e->e.list->head; ep;
            ep = ep->next) {
            if (ep->type == SPOC_ATOM) {
                if (n) {
                    memcpy(sp, sep, slen);
                    sp += slen;
                }
                memcpy(sp, ep->e.atom->val.val,
                    ep->e.atom->val.len);
                sp += ep->e.atom->val.len;
                n++;
            }
        }
        break;

    }

    if (result) {
        oct = oct_new(0, 0);
        octset(oct, result, size);
    }

    return oct;
}

/********************************************************************/

static list_t  *
list_dup(list_t * lp, element_t *parent)
{
    list_t      *new = 0;
    element_t   *ep = 0, *le;

    if (lp == 0)
        return 0;

    new = list_new();
    new->head = ep = new_member(parent, lp->head);
    for (le = lp->head->next; le; le = le->next) {
        ep->next = new_member(parent, le);
        ep = ep->next;
    }

    return new;
}

/********************************************************************/

static range_t *
range_dup(range_t * rp)
{
    range_t *new = 0;

    if (rp == 0)
        return 0;

    new = (range_t *) Malloc(sizeof(range_t));

    boundary_cpy(&new->lower, &rp->lower);
    boundary_cpy(&new->upper, &rp->upper);

    return new;
}

/********************************************************************/

static item_t
i_element_dup(item_t a)
{
    return (item_t) element_dup((element_t *) a);
}

static varr_t  *
set_dup(varr_t * va, element_t * parent)
{
    varr_t   *new;

    new = varr_dup(va, &i_element_dup);

    set_memberof(new, parent);

    return new;
}

/****************************************************************************/
/*!
 * \brief Create a new element struct that contains a atom 
 * \param oct The octet value to be stored in the atom 
 * \return A element struct
 */

element_t *
element_new_atom( octet_t *oct )
{
    element_t *e;

    e = element_new();
    e->e.atom = atom_new( oct );
    e->type = SPOC_ATOM;

    return e;
}

/****************************************************************************/
/*!
 * \brief Create a new element struct that contains a list of elements
 * \param oct The first element in the element list
 * \return A element struct
 */

element_t *
element_new_list( element_t *head)
{
    element_t *e;
    
    e = element_new();

    e->type = SPOC_LIST;
    e->e.list = list_new();
    if( head ) {
        e->e.list->head = head;
        head->memberof = e;
    }

    return e;
}

/****************************************************************************/
/*!
 * \brief Create a new element struct that contains a set of elements
 * \param varr A array of things representing the set.
 * \return A element struct
 */

element_t *
element_new_set( varr_t *varr )
{
    element_t *e;
    
    e = element_new();

    e->type = SPOC_SET;

    if (varr) 
        e->e.set = varr;
    else
        e->e.set = varr_new(4);

    return e;
}

/********************************************************************/
/*!
 * \brief Duplicates a element struct 
 * \param parent The parent of the copy 
 * \param original The element to be duplicated
 * \return A pointer to the copy 
 */
element_t   *
new_member( element_t *parent, element_t *original)
{
    element_t *duplicate;
    
    duplicate = element_dup(original);
    duplicate->memberof = parent;
    
    return duplicate;
}

/********************************************************************/
/*!
 * \brief Duplicates a element struct 
 * \param ep The element to be duplicated
 * \return A pointer to the copy 
 */
element_t   *
element_dup(element_t * ep)
{
    element_t   *new = 0;

    if (ep == 0)
        return 0;

    new = element_new();
    new->type = ep->type;

    switch (ep->type) {
    case SPOC_ATOM:
    case SPOC_PREFIX:
    case SPOC_SUFFIX:
        new->e.atom = _atom_dup(ep->e.atom);
        break;

    case SPOC_LIST:
        new->e.list = list_dup(ep->e.list, new);
        break;

    case SPOC_SET:
    case SPOC_ARRAY:
        new->e.set = set_dup(ep->e.set, new);
        break;

    case SPOC_RANGE:
        new->e.range = range_dup(ep->e.range);
        break;

    default:
        break;

    }

    return new;
}

/*
 * ====================================================================== 
 */
/*!
 * \brief Adds a element to a list of elements, the element is added to the
 * end of the list. If the list is empty a new list is created and the element 
 * is added as the first of the list. The S-expression restriction that the
 * first element in a list has to be a atom is not inforced in this case. Any
 * type of element can be added anywhere. 
 * \param le A pointer to the element list, might be NULL 
 * \param e The element that is to be added to the list
 * \return A pointer to the possibly newly created list 
 */
element_t   *
element_list_add(/*@null@*/ element_t * le, /*@null@*/ element_t * e)
{
    element_t   *ep, *end;

    if (e == 0)
        return le;

    if (le == 0) {
        le = element_new_list(element_dup(e));
    } else {
        if (le->type != SPOC_LIST)
            return 0;

        /*
         * next to last element 
         */
        for (ep = le->e.list->head; ep->next; ep = ep->next);

        end = ep->next;
        ep->next = new_member(le, e);
        ep->next->next = end;
    }

    return le;
}

/*
 * ====================================================================== 
 */

/*!
 * \brief Adds a element to an array of elements. The difference between
 * this and element_add_list is that the array is by definition unsorted, so
 * the added element might end up anywhere in the array. Array differs from
 * Set in that the special restrictions that are valid for sets are not so for 
 * arrays. An array can for instance contain two lists with the same tag.
 * \param le A pointer to the element array 
 * \param e A pointer to the element to be added 
 * \return A pointer to the array, which might be created by the
 * function 
 */
static element_t *
element_array_add(element_t * le, element_t * e)
{
    if (e == 0)
        return le;

    if (le == 0) {
        le = element_new();
        le->type = SPOC_ARRAY;
        le->e.set = varr_add(0, e);
    } else {
        if (le->type != SPOC_ARRAY)
            return 0;
        le->e.set = varr_add(le->e.set, e);
    }

    return le;
}

/*
 * ====================================================================== 
 */
/*!
 * \brief Adds a element to an array of elements. The difference between
 * this and element_add_list is that the array is by definition unsorted, so
 * the added element might end up anywhere in the array. Array differs from
 * Set in that the special restrictions that are valid for sets are not so for 
 * arrays. An Set can not contain two lists with the same tag.
 * \param le A pointer to the element array 
 * \param e A pointer to the element to be added 
 * \return A pointer to the array, which might be created by the function 
 */
element_t *
element_set_add(element_t * le, element_t * e)
{
    varr_t *va = 0;

    if (e == 0)
        return le;

    if (le == 0) {
        va = varr_add( va, (void *) e );
        le = element_new_set( va );
    } else {
        if (le->type != SPOC_SET)
            return 0;
        le->e.set = varr_add(le->e.set, e);
    }

    return le;
}

/*
 * ---------------------------------------------------------------------- 
 */

static varr_t  *
set_search(varr_t * va, octet_t * tag, varr_t * r)
{
    element_t   *ep;
    void       *v;

    if (va == 0 || tag == 0 || tag->len == 0)
        return 0;

    for (v = varr_first(va); v; varr_next(va, v)) {
        ep = (element_t *) v;
        if (ep->type == SPOC_LIST)
            r = list_search(ep->e.list->head, tag, r);
        else if (ep->type == SPOC_SET || ep->type == SPOC_ARRAY)
            r = set_search(ep->e.set, tag, r);
    }

    return r;
}

/*
 * ---------------------------------------------------------------------- 
 */

static varr_t  *
list_search(element_t * e, octet_t * tag, varr_t * r)
{
    if (e == 0)
        return 0;

    if (octcmp(tag, &(e->e.atom->val)) == 0) {
        r = varr_add(r,
                new_member(e->memberof->memberof, e->memberof));
    }

    for (e = e->next; e; e = e->next) {
        if (e->type == SPOC_LIST)
            r = list_search(e->e.list->head, tag, r);
        else if (e->type == SPOC_SET || e->type == SPOC_ARRAY)
            r = set_search(e->e.set, tag, r);
    }

    return r;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*!
 * \brief This function finds all lists in a S-expression that has a
 *  specific tag 
 * \param e The parsed respresentation of the S-expression that 
 *  should be searched 
 * \param tag the list tag 
 * \return An element array containing the list/lists or NULL if no list 
 *  with that tag was found. 
 */
element_t   *
element_find_list(element_t * e, octet_t * tag)
{
    element_t   *re = 0;
    varr_t   *varr = 0;

    if (e == 0)
        return 0;

    if (e->type == SPOC_SET || e->type == SPOC_ARRAY) {
        varr = set_search(e->e.set, tag, varr);
    } else if (e->type == SPOC_LIST) {
        varr = list_search(e->e.list->head, tag, varr);
    }

    if (varr) {
        re = element_new();
        re->type = SPOC_ARRAY;
        re->e.set = varr;
    }

    return re;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*!
 * \brief Given a path specification find the element list that matches that 
 *  description 
 * \param e The element to be searched 
 * \param oa An octarr_t struct representing the path specification 
 * \param i The level in the tree to which the search has reached 
 * \return A Pointer to the matching list or
 *  NULL if no list was found 
 */
static element_t *
element_traverse(element_t * e, octarr_t * oa, int i)
{
    element_t   *ep, *rep;
    int     j, n;
    varr_t   *varr = 0;

    if (e == 0)
        return 0;

    if (e->type == SPOC_SET || e->type == SPOC_ARRAY) {
        varr = e->e.set;
        n = varr_len(varr);
        for (j = 0; j < n; j++) {
            ep = varr_nth(varr, j);
            if (ep->type == SPOC_LIST) {
                if ((rep = element_traverse(ep, oa, i)) != 0)
                    return rep;
            }
        }
    } else if (e->type == SPOC_LIST) {
        ep = e->e.list->head;
        if (octcmp(&(ep->e.atom->val), oa->arr[i]) == 0) {
            i++;
            if (i == oa->n)
                return ep->memberof;    /* the start of the
                             * sublist */

            /*
             * go down the list 
             */
            for (ep = ep->next; ep; ep = ep->next) {
                if (ep->type == SPOC_LIST) {
                    if ((rep = element_traverse(ep, oa, i)) != 0)
                        return rep;
                }
                else if (ep->type == SPOC_SET || ep->type == SPOC_ARRAY) {
                    if ((rep = element_traverse(ep, oa, i)) != 0)
                        return rep;                    
                }

            }
        }
    }

    return 0;
}

/*
 * ---------------------------------------------------------------------- 
 */

/*!
 * \brief Gives you the first element in a list, or one element from a array
 * or set element 
 * \param e A pointer to the element from which the element should be picked 
 * \return A pointer to the subelement or NULL if the element given 
 *  was not a list, array or set.
 */
element_t   *
element_first(element_t * e)
{
    if (e == 0)
        return NULL;

    if (e->type == SPOC_LIST)
        return e->e.list->head;
    else if (e->type == SPOC_SET || e->type == SPOC_ARRAY)
        return (element_t *) varr_nth(e->e.set, 0);
    else
        return NULL;
}

/*
 * ---------------------------------------------------------------------- 
 */

/*!
 * \brief Gives you the last element in a list, or one element from a array
 *  or set element. If you use this function on a array or set it should not
 *  return the same element as element_first() provided that the array or set
 *  element contains more than one element. 
 * \param e A pointer to the element from which the element should be picked 
 * \return A pointer to the subelement or if the element given was not a list,
 *  array or set NULL. 
 */
element_t   *
element_last(element_t * e)
{
    if (e == 0)
        return NULL;

    if (e->type == SPOC_LIST) {
        for (e = e->e.list->head; e->next; e = e->next);
        return e;
    } else if (e->type == SPOC_SET || e->type == SPOC_ARRAY)
        return (element_t *) varr_nth(e->e.set,
                        varr_len(e->e.set) - 1);
    else
        return NULL;
}

/*
 * ---------------------------------------------------------------------- 
 */

/*!
 * \brief Another function that lets you pick a list of subelements. Here
 * you specify the index of the first and the last subelement. -1 denotes that 
 * last subelement in the list. 
 * \param e A pointer to the list element 
 * \param start The starting index 
 * \param end The ending index. 
 * \return A pointer to a copy of the sublist or subelement depending on 
 *  whether the range contained more than one or just one element. If the 
 *  range doesn't make sense, NULL is returned. 
 */

static element_t *
element_intervall(element_t * e, int start, int end)
{
    element_t   *re = 0, *ep, *next;
    int     i, n;

    if (e == 0)
        return 0;

    /*
     * only works on lists 
     */
    if (e->type != SPOC_LIST)
        return 0;

    if (start < -1 || end < -1)
        return 0;

    /*
     * The total number of elements in the list 
     */
    n = element_size(e);

    /*
     * -1 == last 
     */
    if (start == -1)
        start = n - 1;
    if (end == -1)
        end = n - 1;

    /*
     * bogus intervalls 
     */
    if (end < start)
        return 0;
    if (end >= n || start >= n)
        return 0;

    if (end == start) {
        for (i = 0, ep = e->e.list->head; i < start;
            ep = ep->next, i++);
        re = element_dup(ep);
    } else {
        /*
         * get the starting point 
         */
        for (i = 0, ep = e->e.list->head; i < start;
            ep = ep->next, i++);

        re = element_list_add(re, ep);

        for (next = ep->next; i < end; next = next->next, i++)
            re = element_list_add(re, next);
    }

    return re;
}


/*
 * ---------------------------------------------------------------------- 
 */
/*!
 * \brief Swaps the places of the subelements in a list such that the first
 * subelement becomes the last and the last the first. This function only
 * works on element lists. 
 * \param e The element list to be reversed. 
 */
void
element_reverse(element_t * e)
{
    if (e == 0)
        return;

    /*
     * the only thing that can be reversed, a set is a unordered set of
     * element and can therefor can not be reversed 
     */
    if (e->type == SPOC_LIST) {
        element_t   *a, *b, *c = NULL;

        a = e->e.list->head;
        b = a->next;
        a->next = NULL;
        while (b->next) {
            c = b->next;
            b->next = a;
            /* -- */
            a = b;
            b = c;
        }
        b->next = a;
        e->e.list->head = b;
    }
}

/*
 * ----------------------------------------------------------------------
 * void element_free( element_t *e ) defined in lib/free.c *
 * ---------------------------------------------------------------------- 
 */


/*
 * ---------------------------------------------------------------------- 
 */

/*!
 * \brief For transversals in a tree, this function will give you the parent 
 * of the element you supply it with. 
 * \param e A element 
 * \return A pointer to the parent element if it exists, otherwise NULL. 
 */
element_t   *
element_parent(element_t * e)
{
    if (e == 0)
        return 0;

    return e->memberof;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * allowed formats [n], [-m], [n-m], [n-], [last], [-last], [n-last] 
 */
char    *
parse_intervall(octet_t * o, int *start, int *end)
{
    char    *sp;
    int     n, i = 0;

    if ((n = octchr(o, ']')) < 0)
        return 0;

    *start = *end = 0;

    sp = o->val;
    if (*sp <= '9' && *sp >= '0') {
        *start = *sp - '0';
        sp++;
        i++;
        while (i < n && *sp <= '9' && *sp >= '0') {
            *start *= 10;
            *start += *sp - '0';
            i++;
        }
        if (i == n)
            *end = *start;
    } else if (strncmp(sp, "last", 4) == 0) {
        i += 4;
        if (i == n)
            *start = *end = -1;
    }

    if (i < n) {
        if (*sp != '-')
            return 0;
        i++;
        sp++;

        if (i == n)
            *end = -1;
        else if (*sp <= '9' && *sp >= '0') {
            *end = *sp - '0';
            sp++;
            i++;
            while (i < n && *sp <= '9' && *sp >= '0') {
                *end *= 10;
                *end += *sp - '0';
                i++;
            }
        } else if (strncmp(sp, "last", 4) == 0) {
            i += 4;
            if (i == n)
                *end = -1;
        }
    }

    if (*end < *start)
        return 0;
    if (i != n)
        return 0;
    
    return o->val + n + 1;
}

/*
 * ---------------------------------------------------------------------- 
 */

octarr_t *
parse_path(octet_t * o)
{
    char            *sp, *np = 0;
    unsigned int    i;
    octarr_t        *oa = 0;
    octet_t         *oct = 0;

    for (sp = o->val, i = 0; TOKENCHAR(*sp) && i < o->len; sp++, i++) {
        if (*sp == '/') {
            if (*(sp+1) == '/') {
                break;
            }
            
            if (oa == 0) {
                oa = octarr_new(2);
                oct = oct_new(sp - o->val, o->val);
                octarr_add(oa, oct);
            } else {
                oct = oct_new(sp - np, np);
                octarr_add(oa, oct);
            }
            np = sp + 1;
        }
    }

    if (np && !(*np == '*' && (o->len - i == 1))) {
        oct = oct_new(sp - np, np);
        octarr_add(oa, oct);
    }
    else {
        oa = octarr_new(2);
        oct = oct_new(sp - o->val, o->val);
        octarr_add(oa, oct);            
    }

    o->val = sp;
    o->len -= i;

    return oa;
}

/*
 * ---------------------------------------------------------------------- 
 */

/*!
 * This function allows you to pick specific elements from a element
 * representing a parsed S-expression. 
 * \brief Picks subelements from a S-expression
 * \param spec The specification of which element to pick, allowed syntaxes 
 *  /A/B/C - the list with tag 'C' within the list with tag 'B' within the 
 *  list with tag 'A' 
 *  //D - The list/-s with the tag D THIS IS NOT CORRECT 
 *  ..A* all elements in the list with tag A, except the first tag. 
 *  ..A[n-m] elements n to m allowed formats [n], [-m], [n-m], [n-], 
 *  [last], [-last], [n-last] first element has index 0 
 *  ..A* and ..A[1-] is equivalent 
 *  //A | //B All list starting with either A or B
 * \param e The element representing the parsed S-expression 
 * \param rc A pointer to a spocp result code enum, this is where the error 
 *  code will be placed if some problem was encountered while doing the 
 *  picking
 * \result A pointer to a element containing the picked element/-s 
 */

element_t   *
element_eval(octet_t * spec, element_t * e, spocp_result_t *rc)
{
    element_t   *re = 0, *ep = 0, *ie = 0, *sep = 0;
    element_t   *pe = 0, *se = 0;
    char        *sp;
    octet_t     tag, cpy;
    octarr_t    *oa = 0;
    int         p, r, n = 0;

    octln(&cpy, spec);
    do {
        sp = cpy.val;
        p = r = 0;
        pe = se = ie = 0;

        if (*sp == '/') {
            if (*(sp + 1) != '/') { /* path */
                cpy.val++;
                cpy.len--;

                oa = parse_path(&cpy);
                pe = element_traverse(e, oa, 0);
                octarr_free(oa);

                if (pe == 0) {  /* skip the rest of this part */
                    goto NEXTpart;
                }
                r = 1;
            }

            if (cpy.len) {
                sp = cpy.val;
                if (*sp == '/' && *(sp + 1) == '/') {   /* search */
                    if (pe == 0)
                        ep = e;
                    else
                        ep = pe;

                    sp += 2;
                    cpy.val += 2;
                    cpy.len -= 2;
                    tag.val = sp;
                    for (tag.len = 0; *sp != '}' && TOKENCHAR(*sp); 
                         tag.len++, sp++);

                    cpy.len -= tag.len;
                    cpy.val += tag.len;

                    se = element_find_list(ep, &tag);
                    if (se == 0) {  /* skip the rest of
                             * this part */
                        r = 0;
                        goto NEXTpart;
                    }
                    if (varr_len(se->e.set) == 1) { /* only one element, 
                                                     * get rid of the set */
                        ep = varr_pop(se->e.set);
                        element_free(se);
                        se = ep;
                    }
                    r = 1;
                }
            }

            if (cpy.len) {
                if (se == 0) {
                    if (pe == 0)
                        ep = e;
                    else
                        ep = pe;
                } else
                    ep = se;

                sp = cpy.val;

                if (*sp == '[') {   /* subset of the list */
                    int     b, e;

                    cpy.val++;
                    cpy.len--;

                    if ((sp =
                        parse_intervall(&cpy, &b,
                                &e)) == 0) {
                        if (re)
                            element_free(re);
                        if (se)
                            element_free(se);
                        return 0;
                    }
                    cpy.len -= sp - cpy.val;
                    cpy.val = sp;

                    ie = element_intervall(ep, b, e);
                }
                /*
                 * the whole list except the tag 
                 */
                else if (*sp == '*'
                     || (*sp == '/' && *(sp + 1) == '*')) {
                    ie = element_intervall(ep, 1, -1);
                    cpy.val++;
                    cpy.len--;
                    if (*sp == '/') {
                        cpy.val++;
                        cpy.len--;
                    }
                }

                if (ie == 0) {
                    if (se) {
                        element_free(se);
                        se = 0;
                    }
                    r = 0;
                } else
                    r = 1;
            }
        } else {
            *rc = SPOCP_SYNTAXERROR;
            return 0;
        }

        NEXTpart:
        for (sp = cpy.val, p = 0;
            cpy.len && (*sp == ' ' || *sp == '|'); sp++) {
            if (*sp == '|')
                p++;
            cpy.val++;
            cpy.len--;
        }

        if (r) {
            if (n) {
                if (n == 1) {
                    re = element_array_add(re, sep);
                    sep = 0;
                }

                if (ie)
                    re = element_array_add(re, ie);
                else if (se)
                    re = element_array_add(re, se);
                else
                    re = element_array_add(re, element_dup(pe));
            } else {
                if (ie)
                    sep = ie;
                else if (se)
                    sep = se;
                else
                    sep = element_dup(pe);
            }
            n++;
        }

        ep = 0;
    } while (p && cpy.len);

    if (sep)
        return sep;
    else
        return re;
}

/*
 * ---------------------------------------------------------------------- 
 */

static octet_t *
element2oct( element_t *e)
{
    octet_t     *oct = 0, *tmp;
    element_t   *ep;
    int     i;

    switch (element_type(e)) {
    case SPOC_ATOM:
        oct = octdup((octet_t *) element_data(e)) ;
        break;

    case SPOC_LIST:
        for( ep = element_data(e), i=0; ep; ep = ep->next, i++ ) {
            tmp = element2oct( ep );
            if (tmp == 0) {
                oct_free( oct );
                break;
            }

            if (i) {
                octcat( oct, " ", 1);
                octcat( oct, tmp->val, tmp->len );
                oct_free( tmp );
            } else
                oct = tmp;
        }
        break;
    }

    return oct;
}

/*!
 * This function takes a 'string' containing ${0}, ${1} and so on, where
 * substitutions should be made and a list of ATOM elements that should be
 * inserted at the substitution points. The number within the "${" "}" refers
 * to the index of the element in the elementlist. If the element provided to
 * the function is a atom element, then it goes without saying that the only
 * index allowed within a variable reference can be 0. 
 * \brief Makes a variable substitution. 
 * \param val The 'string' containing substitution points is any
 * \param xp An element list or a atom element 
 * \return The string after variable substitutions 
 */

octet_t *
element_atom_sub(octet_t * val, element_t * xp)
{
    int     subs = 0, n, p, i;
    octet_t     oct, spec, *res = 0;
    element_t   *vs;
    octet_t     *tmp;

    /*
     * no variable substitutions necessary 
     */
    p = octstr(val, "${");
    if (p < 0)
        return val;

    /*
     * If there is nothing to substitute with then it can't succeed 
     */
    if (xp == 0)
        return 0;

    /*
     * absolutely arbitrary 
     */
    res = oct_new(val->len * 2, 0);
    octln(&oct, val);

    while (oct.len) {
        /*
         * No more variable substitutions 
         */
        if ((n = octstr(&oct, "${")) < 0) {
            if (!subs)
                return val;
            else {
                if (octcat(res, oct.val, oct.len) !=
                    SPOCP_SUCCESS) {
                    oct_free(res);
                    return 0;
                }
                return res;
            }
        }

        if (n) {
            if (octcat(res, oct.val, n) != SPOCP_SUCCESS) {
                oct_free(res);
                return 0;
            }
            oct.val += n;
            oct.len -= n;
        }

        /*
         * step past the ${ 
         */
        oct.val += 2;
        oct.len -= 2;

        /*
         * strange if no } is found 
         */
        if ((n = oct_find_balancing(&oct, '{', '}')) < 0) {
            oct_free(res);
            return 0;
        }

        spec.val = oct.val;
        spec.len = n;
        DEBUG(SPOCP_DSTORE)
            oct_print( LOG_DEBUG,"element #", &spec );

        if ((i = octtoi(&spec)) < 0 || (vs = element_nth(xp, i)) == 0) {
            oct_free(res);
            return 0;
        }

        {
            octet_t *eop ;

            eop = oct_new( 512,NULL);
            element_print( eop, vs );
            DEBUG(SPOCP_DSTORE)
                oct_print(LOG_WARNING, "Substitution using", eop);
            oct_free( eop);
        }
        /*
         */
        if((tmp = element2oct( vs )) == 0 ) {
            /* */
            octet_t *eop ;

            eop = oct_new( 512,NULL);
            element_print( eop, vs );
            DEBUG(SPOCP_DEBUG)
                oct_print(LOG_WARNING, "Substitution error", eop);
            oct_free( eop);

            return 0;
        }

        octcat(res, tmp->val, tmp->len);
        oct_free( tmp );

        oct.val += n + 1;
        oct.len -= n + 1;

        subs++;
    }

    return res;
}
