
/***************************************************************************
                          func.h  -  description
                             -------------------
    begin                : Mon Jun 3 2002
    copyright            : (C) 2002 by SPOCP
    email                : roland@catalogix.se
 ***************************************************************************/

#ifndef _FUNC_H
#define _FUNC_H

#include <config.h>

#include <sys/time.h>
#include <struct.h>
#include <db0.h>
#include <spocp.h>
#include <varr.h>
#include <plugin.h>
#include <dback.h>

/*
 * server.c 
 */

int             server(FILE * fp, junc_t * cdb, int can);

/*
 * match.c 
 */

junc_t         *atom2atom_match(atom_t * ap, phash_t * pp);
varr_t         *atom2prefix_match(atom_t * ap, ssn_t * pp);
varr_t         *atom2suffix_match(atom_t * ap, ssn_t * pp);
varr_t         *atom2range_match(atom_t * ap, slist_t * slp, int vtype,
				 spocp_result_t * rc);
varr_t         *prefix2prefix_match(atom_t * prefa, ssn_t * prefix);
varr_t         *suffix2suffix_match(atom_t * prefa, ssn_t * suffix);
varr_t         *range2range_match(range_t * ra, slist_t * slp);

/*
 * parr_t *atom_match( junc_t *db, element_t *ep, spocp_result_t *rc ) ;
 * 
 * element_t *skip_list_tails( element_t *ep ) ; junc_t *ending( junc_t *ap,
 * element_t *ep ) ; void *do_extref( erset_t *erp, element_t *ep ) ; junc_t
 * *do_branches( parr_t *avp, element_t *ep ) ; 
 */

junc_t         *element_match_r(junc_t *, element_t *, comparam_t *);

/*
 * input.c 
 */

int             ipv4cmp(struct in_addr *ia1, struct in_addr *ia2);
int             ipv6cmp(struct in6_addr *ia1, struct in6_addr *ia2);

spocp_result_t  element_get(octet_t * oct, element_t ** epp);
element_t      *element_new(void);

atom_t         *atom_new(octet_t * op);

void            hms2int(octet_t * op, long int *li);
void            to_gmt(octet_t * s, octet_t * t);

void            set_memberof(varr_t * va, element_t * group);

/*
 * io.c 
 */

size_t          Writen(int fd, size_t n, char *str);
ssize_t         Readn(int fd, size_t max, char *str);

/*
 * verify 
 */

spocp_result_t  is_numeric(octet_t * op, long *l);
spocp_result_t  numstr(char *s, long *l);
spocp_result_t  is_ipv4(octet_t * op, struct in_addr *ia);
spocp_result_t  is_ipv6(octet_t * op, struct in6_addr *ia);
spocp_result_t  is_date(octet_t * op);
spocp_result_t  is_time(octet_t * op);
spocp_result_t  is_extref(octet_t * op);
spocp_result_t  is_ipv4_s(char *ip, struct in_addr *ia);
spocp_result_t  is_ipv6_s(char *ip, struct in6_addr *ia);

/*
 * hashfunc.c 
 */

unsigned int    lhash(unsigned char *s, unsigned int len, unsigned int init);

/*
 * string.c 
 */

strarr_t       *strarr_new(int size);
strarr_t       *strarr_add(strarr_t * sa, char *value);
void            strarr_free(strarr_t * sa);

void            oct_assign(octet_t * oct, char *s);

char           *lstrndup(char *s, int len);

/*
 * free.c 
 */

void            atom_free(atom_t * ap);
void            list_free(list_t * lp);
void            range_free(range_t * rp);
void            branch_free(branch_t * bp);
void            boundary_free(boundary_t * bp);
void            element_free(element_t * ep);
void            junc_free(junc_t * dbv);
/*
 * void ref_free( ref_t *ref ) ; 
 */

/*
 * element.c 
 */

element_t      *find_list(element_t * ep, char *tag);
varr_t         *get_simple_lists(element_t * ep, varr_t * ap);

/*
 * hash.c 
 */

phash_t        *phash_new(unsigned int pnr, unsigned int density);
buck_t         *phash_insert(phash_t * ht, atom_t * ep, unsigned int key);
buck_t         *phash_search(phash_t * ht, atom_t * ap, unsigned int key);
void            phash_free(phash_t * hp);
void            phash_rm_bucket(phash_t * hp, buck_t * bp);
int             phash_index(phash_t * hp);
void            phash_print(phash_t * ht);

phash_t        *phash_dup(phash_t * php, ruleinfo_t * ri);

varr_t         *get_all_atom_followers(branch_t * bp, varr_t * in);
varr_t         *prefix2atoms_match(char *prefix, phash_t * ht, varr_t * pa);
varr_t         *suffix2atoms_match(char *suffix, phash_t * ht, varr_t * pa);
varr_t         *range2atoms_match(range_t * rp, phash_t * ht, varr_t * pa);

void            bucket_rm(phash_t * ht, buck_t * bp);

/*
 * db0.c 
 */

junc_t         *junc_new(void);
junc_t         *junc_dup(junc_t * jp, ruleinfo_t * ri);
db_t           *db_new(void);
void		db_clr( db_t *db);
void		db_free( db_t *db);


junc_t         *branch_add(junc_t * ap, branch_t * dbp);
branch_t       *brancharr_find(junc_t * arr, int type);
void            brancharr_free(junc_t * ap);
int             free_rule(ruleinfo_t * ri, char *uid);
void            free_all_rules(ruleinfo_t * ri);


spocp_result_t
 add_right(db_t ** db, dbcmd_t * dbc, octarr_t * oa, ruleinst_t ** ri,
	   bcdef_t * bcd);

octet_t        *ruleinst_print(ruleinst_t * r, char *rs);
octet_t        *get_blob(ruleinst_t * ri);
ruleinst_t     *get_rule(ruleinfo_t * ri, octet_t *uid);
spocp_result_t  get_all_rules(db_t * db, octarr_t * oa, char *rs);
varr_t         *get_rec_all_rules(junc_t * jp, varr_t * in);
varr_t         *get_all_bcond_followers(branch_t * bp, varr_t * in);
int             nrules(ruleinfo_t * ri);
int             rules(db_t * db);

void            ruleinst_free(ruleinst_t * rt);

ruleinfo_t     *ruleinfo_new(void);
void            ruleinfo_free(ruleinfo_t *);
ruleinfo_t     *ruleinfo_dup(ruleinfo_t * old);
int             ruleinfo_print(ruleinfo_t * r);
ruleinst_t     *ruleinst_find_by_uid(rbt_t * rules, char *uid);

element_t      *element_dup(element_t * ep, element_t * memberof);
element_t      *element_list_add(element_t * le, element_t * e);
element_t      *element_set_add(element_t * le, element_t * e);

element_t	*element_new_atom( octet_t *);
element_t	*element_new_list( element_t *);
element_t	*element_new_set( varr_t *);

/*
 * raci_t *saci_new( void ) ;
 * 
 * 
 * raci_t *raci_new( void ) ; int P_raci_print( void *vp ) ; void *P_raci_dup( 
 * void *vp ) ; void P_raci_free( void *vp ) ; 
 */

/*
 * index.c 
 */

spocp_index_t        *index_new(int size);
void            index_free(spocp_index_t * id);
spocp_index_t        *index_dup(spocp_index_t * id, ruleinfo_t * ri);
spocp_index_t        *index_add(spocp_index_t * id, ruleinst_t * ri);
int             index_rm(spocp_index_t * id, ruleinst_t * ri);


/*
 * tree.c 
 */

/*
 * link_t *create_link( rnode_t *rn1, rnode_t *rn2 ) ; link_t *find_link(
 * rnode_t *rn1, rnode_t *rn2 ) ; rnode_t *tree_add( limit_t *lp, boundary_t
 * *bp ) ; int cmp_boundary( boundary_t *b1p, boundary_t *b2p ) ; rnode_t
 * *tree_find( rnode_t *head, boundary_t *bp, int type ) ; 
 */

/*
 * subelement.c 
 */

subelem_t      *subelement_dup(subelem_t * se);
int             to_subelements(octet_t * arg, octarr_t * oa);
subelem_t      *octarr2subelem(octarr_t * oa, int dir);


/*
 * ssn.c 
 */

junc_t         *ssn_insert(ssn_t ** top, char *str, int direction);
varr_t         *ssn_match(ssn_t * pssn, char *sp, int direction);
junc_t         *ssn_delete(ssn_t ** top, char *sp, int direction);
ssn_t          *ssn_dup(ssn_t *, ruleinfo_t * ri);
varr_t         *ssn_lte_match(ssn_t * pssn, char *sp, int direction,
			      varr_t * res);
varr_t         *get_all_ssn_followers(branch_t * bp, int type, varr_t * pa);
void            ssn_free(ssn_t *);

/*
 * parr.c 
 */

parr_t         *parr_new(int size, cmpfn * cf, ffunc * ff, dfunc * df,
			 pfunc * pf);

parr_t         *parr_add(parr_t * ap, item_t vp);
parr_t         *parr_add_nondup(parr_t * ap, item_t vp);

parr_t         *parr_dup(parr_t * ap, item_t ref);

parr_t         *parr_join(parr_t * to, parr_t * from, int dup);
item_t          parr_common(parr_t * avp1, parr_t * avp2);
parr_t         *parr_or(parr_t * a1, parr_t * a2);
parr_t         *parr_xor(parr_t * a1, parr_t * a2, int dup);
parr_t         *parr_and(parr_t * a1, parr_t * a2);

void            parr_free(parr_t * ap);

item_t          parr_nth(parr_t * gp, int i);
int             parr_get_index(parr_t * gp, item_t vp);

int             parr_items(parr_t * gp);

item_t          parr_find(parr_t * ap, item_t val);
int             parr_find_index(parr_t * ap, item_t val);

void            parr_rm_index(parr_t ** gpp, int i);
void            parr_rm_item(parr_t ** gpp, item_t vp);

void            parr_dec_refcount(parr_t ** gpp, int delta);

void            parr_nullterm(parr_t * pp);

parr_t         *ll2parr(ll_t * ll);

void            P_null_free(item_t vp);

int             P_strcasecmp(item_t a, item_t b);
int             P_strcmp(item_t a, item_t b);
int             P_pcmp(item_t a, item_t b);
item_t          P_strdup(item_t vp);
char           *P_strdup_char(item_t vp);
item_t          P_pcpy(item_t i, item_t j);
void            P_free(item_t vp);


/*
 * -- 
 */

void            spocp_print(junc_t * dv, int level, int indent);

/*
 * del.c 
 */

/*
 * junc_t *element_rm( junc_t *ap, element_t *ep, int id ) ;
 */
spocp_result_t  rule_rm(junc_t * ap, octet_t * rule, ruleinst_t * rt);

/*
 * slist.c 
 */

slist_t        *sl_init(int max);
slnode_t       *sl_find(slist_t * slp, boundary_t * item);
varr_t         *sl_match(slist_t * slp, boundary_t * item);
varr_t         *sl_range_match(slist_t * slp, range_t * rp);
junc_t         *sl_range_add(slist_t * slp, range_t * rp);
junc_t         *sl_range_rm(slist_t * slp, range_t * rp, int *rc);
varr_t         *sl_range_lte_match(slist_t * slp, range_t * rp, varr_t * pa);
void            sl_free_slist(slist_t * slp);
slist_t        *sl_dup(slist_t * old, ruleinfo_t * ri);

varr_t         *get_all_range_followers(branch_t * bp, varr_t * pa);

subelem_t      *subelem_new(void);
void            subelem_free(subelem_t * sep);

void            boundary_print(boundary_t * bp);
boundary_t     *boundary_dup(boundary_t * bp);
int             boundary_xcmp(boundary_t * b1p, boundary_t * b2p);

void            junc_print(int lev, junc_t * jp);

/*
 * list.c 
 */

spocp_result_t  get_matching_rules(db_t *, octarr_t *, octarr_t *, char *);

/*
 * ll.c 
 */

ll_t           *ll_new(cmpfn * cf, ffunc * ff, dfunc * df, pfunc * pf);
ll_t           *ll_push(ll_t * lp, void *vp, int nodup);
void           *ll_pop(ll_t * lp);
void            ll_free(ll_t * lp);
ll_t           *ll_dup(ll_t * lp);
void           *ll_find(ll_t * lp, void *pattern);
int             ll_rm_item(ll_t * lp, void *pattern, int all);
void            ll_rm_payload(ll_t * lp, void *pl);
int             ll_print(ll_t * lp);

ll_t           *parr2ll(parr_t * pp);

/*
 * dup.c 
 */

/*
 * aci.c 
 */

ruleinst_t     *allowing_rule(junc_t * ap);
spocp_result_t  allowed(junc_t * ap, comparam_t * comp);

/*
 * -- varr.c --- 
 */

varr_t         *varr_junc_add(varr_t * va, junc_t * ju);
junc_t         *varr_junc_pop(varr_t * va);
junc_t         *varr_junc_nth(varr_t * va, int n);
junc_t         *varr_junc_common(varr_t * va, varr_t * b);
junc_t         *varr_junc_rm(varr_t * va, junc_t * v);

/*
 * ----- 
 */

varr_t         *varr_ruleinst_add(varr_t * va, ruleinst_t * ju);
ruleinst_t     *varr_ruleinst_pop(varr_t * va);
ruleinst_t     *varr_ruleinst_nth(varr_t * va, int n);

/*
 * ----- 
 */

int             bcspec_is(octet_t * spec);

bcdef_t        *bcdef_add(db_t *, plugin_t *, dbcmd_t *, octet_t *, octet_t *);
spocp_result_t  bcdef_del(db_t * db, dbcmd_t * dbc, octet_t * name);
spocp_result_t  bcdef_replace(db_t *, plugin_t *, dbcmd_t *, octet_t *, octet_t *);
bcdef_t        *bcdef_get(db_t *, plugin_t *, dbcmd_t *, octet_t *, spocp_result_t *);
void		bcdef_free(bcdef_t * bcd);


spocp_result_t	bcexp_eval(element_t *, element_t *, bcexp_t *, octarr_t **);
spocp_result_t	bcond_check(element_t *, spocp_index_t *, octarr_t **);
varr_t		*bcond_users(db_t *, octet_t *);

/*
 * --- sllist.c --- 
 */

slnode_t       *sl_new(boundary_t * item, int n, slnode_t * tail);
void            sl_free_node(slnode_t * slp);
void            sl_free_slist(slist_t * slp);
int             sl_rand(slist_t * slp);
void            sl_rec_insert(slnode_t * old, slnode_t * new, int n);
slnode_t       *sl_rec_delete(slnode_t * node, boundary_t * item, int n);
slnode_t       *sl_rec_find(slnode_t * node, boundary_t * item, int n,
			    int *flag);
slnode_t       *sl_delete(slist_t * slp, boundary_t * item);
void            sl_print(slist_t * slp);
slnode_t       *sl_insert(slist_t * slp, boundary_t * item);
slnode_t       *sln_dup(slnode_t * old);

/*
 * ll.c 
 */

node_t         *node_new(void *vp);
ll_t           *ll_new(cmpfn * cf, ffunc * ff, dfunc * df, pfunc * pf);
void            ll_free(ll_t * lp);
ll_t           *ll_push(ll_t * lp, void *vp, int nodup);
void           *ll_pop(ll_t * lp);
node_t         *ll_first(ll_t * lp);
node_t         *ll_next(ll_t * lp, node_t * np);
void           *ll_first_p(ll_t * lp);
void           *ll_next_p(ll_t * lp, void *prev);
void            ll_rm_link(ll_t * lp, node_t * np);

/*
 * element.c
 */

int		element_print( octet_t *oct, element_t *ep);
char		*element2str( element_t *ep);
element_t	*element_reduce( element_t *ep ) ;

/*
 * sum.c
 */

element_t	*get_indexes( junc_t *jp );

#endif
