/*! \file lib/bcond.c \author Roland Hedberg <roland@catalogix.se> 
 */

#include <config.h>

#include <string.h>

#ifdef HAVE_LIBCRYPTO
#include <openssl/sha.h>
#else
#include <sha1.h>
#endif

#include <bcond.h>
#include <bcondfunc.h>
#include <plugin.h>
#include <spocp.h>
#include <ruleinst.h>
#include <proto.h>
#include <dback.h>
#include <octet.h>

#include <wrappers.h>
#include <log.h>

/* #define AVLUS 1 */

/*
 * ====================================================================== *
 * LOCAL FUNCTIONS
 * ====================================================================== 
 */

static bcdef_t *
bcdef_rm(bcdef_t * root, bcdef_t * rm)
{
    if (rm == root) {
        if (root->next) {
            root->next->prev = 0;
            return root->next;
        }
        return 0;
    }

    if (rm->next)
        rm->next->prev = rm->prev;
    if (rm->prev)
        rm->prev->next = rm->next;

    return root;
}

/*
 * ---------------------------------------------------------------------- 
 */

#ifdef HAVE_LIBCRYPTO
#define uint8  unsigned char
#endif

char    *
make_hash(char *str, octet_t * input)
{
#ifndef HAVE_LIBCRYPTO
    struct          sha1_context ctx;
#endif
    unsigned char   sha1sum[21], *ucp;
    int             j;

    if (input == 0 || input->len == 0)
        return 0;

#ifdef HAVE_LIBCRYPTO
    SHA1((uint8 *) input->val, input->len, sha1sum);
#else
    sha1_starts(&ctx);
    sha1_update(&ctx, (uint8 *) input->val, input->len);
    sha1_finish(&ctx, (unsigned char *) sha1sum);
#endif

    for (j = 0, ucp = (unsigned char *) str; j < 20; j++, ucp += 2)
        sprintf((char *) ucp, "%02x", sha1sum[j]);

    str[0] = '_';
    *ucp++ = '_';
    *ucp = '\0';

    return str;
}


/*
 * ---------------------------------------------------------------------- 
 */

/*
 * recursively find all the rules that uses this boundary condition directly
 * or through some intermediate 
 */
static varr_t  *
all_rule_users(bcdef_t * head, varr_t * rules)
{
    void           *vp;
    bcdef_t        *bcd;

    if (head->users) {
        for (vp = varr_first(head->users); vp;
             vp = varr_next(head->users, vp)) {
            bcd = (bcdef_t *) vp;
            if (bcd->users || bcd->rules)
                rules = all_rule_users(bcd, rules);
        }
    }

    if (head->rules)
        rules = varr_or(rules, head->rules, 1);

    return rules;
}

/*
 * ---------------------------------------------------------------------- 
 */

static void
bcdef_delink(bcdef_t * bcd)
{
    bcexp_delink(bcd->exp);
}

/*
 * ---------------------------------------------------------------------- 
 */

/*
 * expects a spec of the form {...}{...}:.... 
 */

static spocp_result_t
bcond_eval(element_t * qp, element_t * rp, bcspec_t * bcond, octet_t * oct)
{
    octet_t         spec;
    element_t      *tmp, *xp = 0;
    int             sl, rl;
    spocp_result_t  rc = SPOCP_DENIED;
    cmd_param_t     cpt;

    octln(&spec, bcond->spec);

    while (spec.len && *spec.val == '{') {
        spec.len--;
        spec.val++;
        if ((sl = octchr(&spec, '}')) < 0)
            break;

        rl = spec.len - (sl + 1);
        spec.len = sl;

        if ((tmp = element_eval(&spec, qp, &rc)) == NULL) {
            tmp = element_new();
            tmp->type = SPOC_NULL;
        }
        xp = element_list_add(xp, tmp);

        spec.val += sl + 1;
        spec.len = rl;
    }

    if (*spec.val != ':')
        return SPOCP_SYNTAXERROR;
    spec.val++;
    spec.len--;

    cpt.q = qp;
    cpt.r = rp;
    cpt.x = xp;
    cpt.arg = &spec;
    cpt.pd = bcond->plugin->dyn;
    cpt.conf = bcond->plugin->conf;

    rc = bcond->plugin->test(&cpt, oct);

    if (xp)
        element_free(xp);

    return rc;
}

/*
 * ---------------------------------------------------------------------- 
 */

/*!
 * \brief Evaluates a boundary expression
 * \param qp The element representation of the query
 * \param rp The element representation of the matched rule 
 * \param bce The boundary expression
 * \param oa Place for the dynamic blobs
 * \return A spocp result code
 */
spocp_result_t
bcexp_eval(element_t * qp, element_t * rp, bcexp_t * bce, octarr_t ** oa)
{
    int             n, i;
    spocp_result_t  r = SPOCP_DENIED;
    octet_t         oct;
    octarr_t       *lo = 0, *po = 0;

    oct.size = oct.len = 0;

    switch (bce->type) {
    case SPEC:
        r = bcond_eval(qp, rp, bce->val.spec, &oct);
        if (r == SPOCP_SUCCESS && oct.len) {
            lo = octarr_add(0, octdup(&oct));
            octclr(&oct);
        }
        break;

    case NOT:
        r = bcexp_eval(qp, rp, bce->val.single, &lo);
        if (r == SPOCP_SUCCESS) {
            if (lo)
                octarr_free(lo);
            r = SPOCP_DENIED;
        } else if (r == SPOCP_DENIED)
            r = SPOCP_SUCCESS;
        break;

    case AND:
        n = varr_len(bce->val.arr);
        r = SPOCP_SUCCESS;
        for (i = 0; r == SPOCP_SUCCESS && i < n; i++) {
            r = bcexp_eval(qp, rp, (bcexp_t *) varr_nth(bce->val.arr, i), &po);
            if (r == SPOCP_SUCCESS && po)
                lo = octarr_extend(lo, po);
        }
        if (r != SPOCP_SUCCESS)
            octarr_free(lo);
        break;

    case OR:
        n = varr_len(bce->val.arr);
        r = SPOCP_DENIED;
        for (i = 0; r != SPOCP_SUCCESS && i < n; i++) {
            r = bcexp_eval(qp, rp, (bcexp_t *) varr_nth(bce->val.arr, i), &lo);
        }
        break;

    case REF:
        r = bcexp_eval(qp, rp, bce->val.ref->exp, &lo);
        break;
    }

    if (r == SPOCP_SUCCESS && lo)
        *oa = octarr_extend(*oa, lo);

    return r;
}

/*
 * If it's a reference to a boundary condition it should be of the form (ref
 * <name>) 
 */
static spocp_result_t
is_bcref(octet_t * o, octet_t * res)
{
    octet_t         lc;
    octet_t         op;
    spocp_result_t  r;

    if (*o->val == '(') {

        octln(&lc, o);
        lc.val++;
        lc.len--;

        if ((r = get_str(&lc, &op)) != SPOCP_SUCCESS)
            return r;
        if (oct2strcmp(&op, "ref") != 0)
            return SPOCP_SYNTAXERROR;
        if ((r = get_str(&lc, &op)) != SPOCP_SUCCESS)
            return r;
        if (!(*lc.val == ')' && lc.len == 1))
            return SPOCP_SYNTAXERROR;

        octln(res, &op);

        return SPOCP_SUCCESS;
    }

    return SPOCP_SYNTAXERROR;
}

static char    *
bcname_make(octet_t * name)
{
    size_t          len;
    char           *str;

    len = name->len + 6 + 1;
    str = (char *) Malloc(len);
    /*
     * sprintf( str, "BCOND:%s", name->val ) ; 
     */
    strcat(str, "BCOND:%s");
    strncpy(str + 6, name->val, name->len);
    str[6 + name->len] = '\0';

    return str;
}

/*
 * ---------------------------------------------------------------------- *
 * PUBLIC INTERFACES *
 * ---------------------------------------------------------------------- 
 */

/*! Is this a proper specification of a boundary condition ? 
 * \param spec A pointer to the boundary condition specification 
 * \return TRUE is true or FALSE if not 
 */
int
bcspec_is(octet_t * spec)
{
    int             n;

    if ((n = octchr(spec, ':')) < 0)
        return FALSE;

    for (n--; n >= 0 && DIRCHAR(spec->val[n]); n--);

    /*
     * should I check the XPath definitions ? 
     */

    if (n == -1)
        return TRUE;
    else
        return FALSE;
}

/*! \brief Adds a boundary condition definition to the list of others 
 * \param rbc A pointer to the first boundary condition in the list of stored
 *  specifications.
 * \param p Pointer to the linked list of registered plugins 
 * \param dbc Command parameters connected to the persistent store 
 * \param name The name of the boundary condition specification 
 * \param data The boundary condition specification 
 * \return A pointer to the internal representation of the boundary condition
 */
bcdef_t        *
bcdef_add(bcdef_t **rbc, plugin_t *p, dbackdef_t *dbc, octet_t *name,
        octet_t *data)
{
    bcexp_t     *bce;
    bcdef_t     *bcd, *bc;
    char        cname[45], *bcname;
    bcstree_t   *st;
    octet_t     tmp;

    if (*data->val == '(') {
        if ((st = parse_bcexp(data)) == 0)
            return 0;
    } else {
        st = (bcstree_t *) Calloc(1,sizeof(bcstree_t));
        octcpy(&st->val, data);
    }

    bcd = bcdef_new();

    bce = transv_stree(p, st, *rbc, bcd);

    bcstree_free(st);

    if (bce == 0) {
        bcdef_free(bcd);
        return 0;
    }

    if (bce->type != REF) {
        if (name == 0 || name->len == 0) {
            make_hash(cname, data);
            bcd->name = Strdup(cname);
        } else {
            bcd->name = oct2strdup(name, 0);
        }
    }

    /* If there is a persistent store */
    if (dbc && dbc->dback) {
        /*
         * create a bcond name that MUST differ from rule ids. Since
         * rule ids are SHA1 hashes, it consists of lower case letters 
         * a-f and numbers. So making the bcond name in the persistent 
         * store start with "BCOND:" will make it absolutely different. 
         */

        oct_assign(&tmp, bcd->name);
        bcname = bcname_make(&tmp);
        dback_save(dbc, bcname, data, 0, 0);
        Free(bcname);
    }

    bcd->exp = bce;

    if (bcd->name) {
        if (*rbc == 0)
            *rbc = bcd;
        else {
            for (bc = *rbc; bc->next; bc = bc->next);
            bc->next = bcd;
            bcd->prev = bc;
        }
    }

    return bcd;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief Remove a boundary condition from the internal database. A
 * boundary condition can not be removed if there is rules that uses it!
 * \param rbc A pointer to the first boundary condition in the list of stored
 *  specifications.
 * \param dbc Command parameters connected to the persistent store 
 * \param name The name of the boundary condition 
 * \return A result code, SPOCP_SUCCESS on success 
 */
spocp_result_t
bcdef_del(bcdef_t **rbc , dbackdef_t * dbc, octet_t * name)
{
    bcdef_t        *bcd;
    char           *bcname;

    bcd = bcdef_find(*rbc, name);

    if (bcd->users && varr_len(bcd->users) > 0)
        return SPOCP_UNWILLING;
    if (bcd->rules && varr_len(bcd->rules) > 0)
        return SPOCP_UNWILLING;


    if (dbc && dbc->dback){
        bcname = bcname_make(name);
        dback_delete(dbc, bcname);
        Free(bcname);
    }

    /*
     * this boundary conditions might have links to other links
     * which should then also be removed 
     */
    bcdef_delink(bcd);

    *rbc = bcdef_rm(*rbc, bcd);

    bcdef_free(bcd);

    return SPOCP_SUCCESS;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief Given a boundary condition specification, return a pointer to the
 * internal representation. That includes returning a pointer to a already
 * existing stored boundary condition if this specification is a reference.
 * Or if the specification is not a simple reference, store it and return a
 * pointer to where it is stored. If the specification is equal to the string
 * "NULL" then a NULL pointer is returned.
 * \param rbc A pointer to the first boundary condition in the list of stored
 *  specifications.
 * \param o The boundary conditions specification
 * \param p Pointer to the set of plugins that is present
 * \param dbc Command parameters connected to the persistent store
 * \param rc Where the result code of the * operation should be stored
 * \return A pointer to the internal representation 
 * or NULL if it does not exist, is faulty or references unknown boundary
 * conditions.  If the boundary condition specification is equal to "NULL",
 * NULL is also returned but together with the result code SPOCP_SUCCESS. 
 */
bcdef_t        *
bcdef_get(bcdef_t **rbc, plugin_t * p, dbackdef_t * dbc, octet_t * o,
      spocp_result_t * rc)
{
    bcdef_t        *bcd = 0;
    spocp_result_t  r;
    octet_t         br;

    if (oct2strcmp(o, "NULL") == 0)
        bcd = NULL;
    else if ((r = is_bcref(o, &br)) == SPOCP_SUCCESS) {
        bcd = bcdef_find(*rbc, &br);
        if (bcd == NULL)
            *rc = SPOCP_UNAVAILABLE;
    } else {
        bcd = bcdef_add(rbc, p, dbc, 0, o);
        if (bcd == 0)
            *rc = SPOCP_SYNTAXERROR;
    }

    return bcd;
}


/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief A niffty function that allows you to find all rules that are
 * dependent on a specific boundary condition. 
 * \param db A pointer to the internal database 
 * \param bcname The name of the boundary condition 
 * \return A array of pointers to the rules that uses, directly of indirectly, this
 * boundary condition 
 */
varr_t         *
bcond_users(bcdef_t *rbc, octet_t * bcname)
{
    bcdef_t        *bcd;

    bcd = bcdef_find(rbc, bcname);

    return all_rule_users(bcd, 0);
}

/*
 * ---------------------------------------------------------------------- 
 */
