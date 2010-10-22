
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

static ruleset_t    *one_level(octet_t * name, ruleset_t * rs);

/* #define AVLUS 1 */

/*
 * ---------------------------------------------------------------------- 
 */

/*!
 * \brief Splits a path into it's components. A path is a set of
 *  strings divided by a '/' character.
 * \param path An octet struct containing the path as value
 * \param pathlen A pointer to an int that will contain the length of the path.
 * \return An octarr struct with the parts.
 */

octarr_t       *
path_split(octet_t *path, int *pathlen)
{
    char           *sp, *ep;
    octarr_t       *res;
    octet_t        *o, tmp;

    octln(&tmp, path);

    while (*tmp.val == '/')
        tmp.val++, tmp.len--;

    for (sp = tmp.val; *sp == '/' || DIRCHAR(*sp); sp++);

    tmp.len = sp - tmp.val;

    *pathlen = sp - path->val;

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

/*!
 * \brief Creates a new instance of a ruleset struct, assigning it a name.
 * \param name The name that should be assigned to the struct.
 * \return A newly created instance of a ruleset struct.
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

/*!
 * \brief Used for logging, will print the names of all the rule sets
 *  in a tree.
 * \param rs The root of the tree
 * \param level The distance from the root
 */

void
ruleset_tree( ruleset_t *rs, int level )
{
    if (rs) {
        traceLog(LOG_INFO, "(%d) Name: %s", level, rs->name);
        if (rs->left)
            ruleset_tree( rs->left, level );
        if (rs->right)
            ruleset_tree( rs->right, level );
        if (rs->down)
            ruleset_tree( rs->down, level+1 );
    }
}

/*!
 * \brief Remove a tree of rule sets
 * \param rs Pointer to the root of the rule set tree
 */
void
remove_ruleset_tree(ruleset_t * rs)
{
    if (rs) {
        if (rs->down)
            remove_ruleset_tree(rs->down);

        if ( rs->name)
            Free(rs->name);
        if (rs->db) 
            db_free( rs->db );

        Free(rs);
    }
}

/*!
 * \brief Search through all the rulesets on this level to see if this one
 * already exists.
 * \param name The name of the rule set in an actet struct
 * \param rs Pointer to the root for the level.
 * \return Pointer to the rule set struct or NULL if it doesn't exist.
 */
static ruleset_t      *
one_level(octet_t * name, ruleset_t * rs)
{
    int             c;

    if (rs == 0)
        return 0;

#ifdef AVLUS
    traceLog(LOG_DEBUG,"one_level: [%s](%d),[%s]", name->val,
         name->len, rs->name);
#endif
    c = oct2strcmp(name, rs->name);

    if (c < 0) {
        while( c < 0 ) {
            if (!rs->left)
                break;
            rs = rs->left;
            c = oct2strcmp(name, rs->name);
        }
    }
    else {
        while( c > 0 ) {
            if (!rs->right)
                break;
            rs = rs->right;
            c = oct2strcmp(name, rs->name);
        }
    }

    if (c == 0)
        return rs;
    else
        return 0;
}

/*
 * returns pointer to the ruleset if it exists otherwise NULL
 */
ruleset_t *
ruleset_find(octet_t * name, ruleset_t * rs)
{
    octet_t         loc;
    ruleset_t      *r, *nr = 0;
    octarr_t       *oa = 0;
    int             i, pathlen;

    if (rs == 0) {
        traceLog(LOG_INFO,"ruleset_find() no starting point");
        return 0;
    }

    /*
     * the root 
     */
    if (name == 0 || name->len == 0
        || (*name->val == '/' && name->len == 1)) {
        for (r = rs; r->up; r = r->up);
        return r;
    }
    /*
     * server part 
     */
    else if (name->len >= 2 && *name->val == '/'
         && *(name->val + 1) == '/') {
        for (r = rs; r->up; r = r->up);

        oct_assign(&loc,"//");

        if ((nr = one_level(&loc, r)) == 0) {
            return NULL;
        }

        if (nr && name->len == 2) {
            oct_step(name, 2);
            return nr;
        }

        octln(&loc, name);
        oct_step(&loc, 2);
    }
    /*
     * absolute path 
     */
    else if (*name->val == '/') {
        /*traceLog(LOG_INFO,"Absolute path");*/
        for (nr = rs; nr->up; nr = nr->up);
        octln(&loc, name);
        oct_step(&loc, 1);
    } else 
        return NULL;    /* don't do relative */

    oa = path_split(&loc, &pathlen);

#ifdef AVLUS
    if (oa)
        traceLog(LOG_INFO,"%d levels in path", oa->n );
#endif

    for (i = 0; oa && i < oa->n; i++) {
        r = nr->down;
        if (r == 0) {
            octarr_free(oa);
            return NULL;
        }

        if ((nr = one_level(oa->arr[i], r)) == 0)
            break;
    }

    octarr_free(oa);

    if (nr == 0) 
        return NULL;

    name->val = loc.val + pathlen;
    name->len = loc.len - pathlen;

#ifdef AVLUS
    traceLog(LOG_INFO,"Found \"%s\"", nr->name);
#endif

    return nr;
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

#ifdef AVLUS    
    traceLog(LOG_DEBUG,"add_level: [%s](%d),[%s]", name->val,
                 name->len, rs->name);
#endif
    c = oct2strcmp(name, rs->name);

    if (c < 0) {
        while( c < 0 ) {
            /* Far left */
#ifdef AVLUS
            traceLog(LOG_DEBUG,"Left of %s", rs->name);
#endif

            if (!rs->left) {
                rs->left = new;
                new->right = rs;
                rs = new;
                c = 0;
            }
            else {
                rs = rs->left;
                c = oct2strcmp(name, rs->name);
            }
        }
        
        if ( c > 0 ) {
            new->right = rs->right;
            new->left = rs;
            new->right->left = new;
            rs->right = new;    
            rs = new;
        }
    } else if( c > 0) {
        while( c > 0 ) {
#ifdef AVLUS
            traceLog(LOG_DEBUG,"Right of %s", rs->name);
#endif
            /* Far right */
            if (!rs->right) {
                rs->right = new;
                new->left = rs;
                rs = new;
                c = 0;
            }
            else {
                rs = rs->right;
                c = oct2strcmp(name, rs->name);
            }
        }
        
        if ( c < 0 ) {
            new->left = rs->left;
            new->right = rs;
            new->left->right = new;
            rs->left = new; 
            rs = new;
        }
    } 

    return rs;
}

/*
 * ---------------------------------------------------------------------- 
 */

ruleset_t      *
ruleset_create(octet_t * name, ruleset_t **root)
{
    ruleset_t      *r, *nr;
    octet_t         loc;
    octarr_t       *oa = 0;
    int             i, pathlen = 0;

#ifdef AVLUS
    oct_print(LOG_INFO,"Ruleset create", name);
#endif

    if (*root == 0) {
        *root = ruleset_new(0);

        if (name == 0 || name->len == 0
            || (name->len == 1 && *name->val == '/'))
            return *root;
    } /* else
           traceLog(LOG_INFO, "Got some kind of tree"); 
           */


    if( name == 0 || name->len == 0 ) 
        return *root ;

    r = *root;

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

    octarr_free(oa);

    name->val = loc.val + pathlen;
    name->len = loc.len - pathlen;

    return r;
}

ruleset_t   *
ruleset_add(ruleset_t **root, char *path, octet_t *op)
{
    octet_t     oct;
    ruleset_t   *rs;
    
    if (path)
        oct_assign(&oct, path);
    else {
        octln( &oct, op);
    }

    if ((rs = ruleset_create(&oct, root)) == 0)
        return NULL;
    
    rs->db = db_new();
    
    return rs;
}

/*
 * ---------------------------------------------------------------------- 
 */

/*!
 * \brief Returns the pathname of the rule set.
 * \param rs Pointer to the rule set struct
 * \param buf Pointer to a string buffer where the pathname should be
 *  placed.
 * \param buflen The length of the string buffer.
 * \return Operation status.
 */

spocp_result_t
ruleset_pathname(ruleset_t * rs, char *buf, int buflen)
{
    int         len = 0, n=0;
    ruleset_t   *rp;
    char        *arr[64];

    memset(arr, 0, 64*sizeof(char *));
    for (rp = rs; rp; rp = rp->up) {
        len++;
        len += strlen(rp->name) + 1;
        arr[n++] = rp->name;
    }
    len++;

    if (len > buflen)
        return SPOCP_OPERATIONSERROR;

    *buf = 0;

    for (n--; n >= 0 ; n--) {
        if (arr[n][0] == '/' && arr[n][1] == '/' && arr[n][2] == '\0') 
            strcat(buf, "/");
        else if (arr[n][0] == '\0') {
            continue;
        }
        else {
            /* Flawfinder: ignore */
            strcat(buf, "/");
            /* Flawfinder: ignore */
            strcat(buf, arr[n]);
        }
    }

    return SPOCP_SUCCESS;
}





