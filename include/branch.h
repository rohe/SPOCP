/*
 *  branch.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/8/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */


#ifndef __BRANCH_H_
#define __BRANCH_H_

#include <config.h>

#include <rdb.h>
#include <ll.h>
#include <varr.h>
#include <element.h>
#include <basefn.h>
#include <dset.h>
#include <ruleinfo.h>
#include <skiplist.h>
#include <index.h>
#include <range.h>
#include <plugin.h>

struct _branch;

/*
 * ------------------------------------------ 
 */

#define FORWARD     1
#define BACKWARD	2


/*********** where all the branches starts *************/

typedef struct _junc {
	int             dynamic;
	int             refc;
	struct _branch  *branch[NTYPES];
} junc_t;

#define ARRFIND(a,c) ( (a) ? (a)->branch[(c)] : NULL )

/*********** for prefixes ****************/

typedef struct _ssn {
	unsigned char	ch;
	struct _ssn     *left;
	struct _ssn     *down;
	struct _ssn     *right;
	junc_t          *next;
	unsigned int	refc;	/* reference counter */
} ssn_t;

/******* for atoms **********************/

typedef struct _bucket {
	octet_t             val;
	unsigned long int   hash;
	unsigned int        refc;
	junc_t              *next;
} buck_t;

typedef struct _phash {
	unsigned int	refc;
	unsigned char	pnr;
	unsigned char	density;
	unsigned int	size;
	unsigned int	n;
	unsigned int	limit;
	buck_t          **arr;
} phash_t;


/****** branches **********************/

typedef struct _branch {
	int                 type;
	int                 count;
	junc_t              *parent;
    
	union {
		dset_t          *set;
		phash_t         *atom;
		junc_t          *list;
		junc_t          *next;
		ssn_t           *prefix;
		ssn_t           *suffix;
		slist_t         *range[DATATYPES];
		/*spocp_index_t	*id;*/
        ruleinst_t      *ri;
	} val;
    
} branch_t;

junc_t      *junc_new(void);
junc_t      *junc_dup(junc_t * jp, ruleinfo_t * ri);
void        junc_free(junc_t * juncp);

junc_t      *varr_junc_pop(varr_t * va);
junc_t      *varr_junc_nth(varr_t * va, int n);
junc_t      *varr_junc_common(varr_t * va, varr_t * b);
junc_t      *varr_junc_rm(varr_t * va, junc_t * v);
varr_t      *varr_junc_add(varr_t * va, junc_t * ju);

junc_t      *add_branch(junc_t * ap, branch_t * dbp);
junc_t      *add_element(plugin_t * pl, junc_t * dvp, element_t * ep,
                         ruleinst_t * ri, int n);

branch_t    *branch_new(int type);
char        *set_item_list(junc_t * dv);
junc_t      *add_atom(branch_t * bp, atom_t * ap);
junc_t      *extref_add(branch_t * bp, atom_t * ap);
junc_t      *list_end(junc_t * arr);
junc_t      *add_list(plugin_t * pl, branch_t * bp, list_t * lp, 
                      ruleinst_t * ri);
junc_t      *add_range(branch_t * bp, range_t * rp);
junc_t      *add_prefix(branch_t * bp, atom_t * ap);
junc_t      *rule_close(junc_t * ap, ruleinst_t * ri);
junc_t      *add_set( branch_t *bp, element_t *ep, plugin_t *pl, 
                     junc_t *jp, ruleinst_t *rt);
junc_t      *add_suffix(branch_t * bp, atom_t * ap);
junc_t      *add_any(branch_t * bp);
void		branch_free(branch_t * bp);

ruleinst_t  *ruleinst_new(octet_t * rule, octet_t * blob, char *bcname);
ruleinst_t  *ruleinst_dup(ruleinst_t * ri);
void        ruleinst_free(ruleinst_t * ri);
junc_t      *rule_end(junc_t * ap, ruleinst_t * ri);

void        junc_print(int lev, junc_t * jp);

varr_t      *get_all_range_followers(branch_t * bp, varr_t * va);

/* ----------- rangestore.c ------------------- */
junc_t      *bsl_rm_range(slist_t *slp, range_t *rp, spocp_result_t *sr);

/*
junc_t   *add_next(plugin_t *, junc_t *, element_t *, ruleinst_t *);
*/

#endif
