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

/* server.c */

int server( FILE *fp, junc_t *cdb, int can ) ;

/* match.c */

junc_t *atom2atom_match( atom_t *ap, phash_t *pp ) ;
parr_t *atom2prefix_match( atom_t *ap, ssn_t *pp ) ;
parr_t *atom2suffix_match( atom_t *ap, ssn_t *pp ) ;
parr_t *atom2range_match( atom_t *ap, slist_t *slp, int vtype, spocp_result_t *rc ) ;
parr_t *prefix2prefix_match( atom_t *prefa, ssn_t *prefix ) ;
parr_t *suffix2suffix_match( atom_t *prefa, ssn_t *suffix ) ;
parr_t *range2range_match( range_t *ra, slist_t *slp ) ;

/*
parr_t *atom_match( junc_t *db, element_t *ep, parr_t *pap, becpool_t *, spocp_result_t *rc ) ;

element_t *skip_list_tails( element_t *ep ) ;
junc_t    *ending( junc_t *ap, element_t *ep, parr_t *pap, becpool_t *bcp ) ;
void      *do_extref( erset_t *erp, element_t *ep, parr_t *pap, becpool_t * ) ;
junc_t    *do_branches( parr_t *avp, element_t *ep, parr_t *pap, becpool_t *bcp ) ;
*/

junc_t *element_match_r(
  junc_t *, element_t *, parr_t *, becpool_t *, spocp_result_t *, octnode_t ** ) ;

/* input.c */

int             get_len( octet_t *oct ) ;
spocp_result_t  get_str( octet_t *oct, octet_t *str ) ;
char           *sexp_to_canonical( char *sexp ) ;
int             ipv6cmp( struct in6_addr *ia1, struct in6_addr *ia2 ) ;

spocp_result_t  element_get( octet_t *oct, element_t **epp ) ;
element_t      *element_new( void ) ;

atom_t         *atom_new( octet_t *op ) ;
set_t          *set_new( int size ) ;

void            hms2int( octet_t *op, long int *li ) ;
void            to_gmt( octet_t *s, octet_t *t ) ;


/* io.c */

size_t  Writen( int fd, size_t n, char *str ) ;
ssize_t Readn( int fd, size_t max, char *str ) ;

/* verify */

spocp_result_t is_numeric( octet_t *op, long *l ) ;
spocp_result_t numstr( char *s, long *l ) ;
spocp_result_t is_ipv4( octet_t *op, struct in_addr *ia ) ;
spocp_result_t is_ipv6( octet_t *op, struct in6_addr *ia ) ;
spocp_result_t is_date( octet_t *op ) ;
spocp_result_t is_time( octet_t *op ) ;
spocp_result_t is_extref( octet_t *op ) ;
spocp_result_t is_ipv4_s( char *ip, struct in_addr *ia ) ;
spocp_result_t is_ipv6_s( char *ip, struct in6_addr *ia ) ;

/* hashfunc.c */

unsigned int lhash( unsigned char *s, unsigned int len, unsigned int init ) ;

/* string.c */

int      sexp_len( octet_t *sexp ) ;

strarr_t *strarr_new( int size ) ;
strarr_t *strarr_add( strarr_t *sa, char *value ) ;
void      strarr_free( strarr_t *sa ) ;

int      str_expand( char *src, keyval_t *kvp, char *dest, size_t size ) ;

char *sexp_print( char *sexp, unsigned int *bsize, char *fmt, ... ) ;

/* free.c */

void atom_free( atom_t *ap ) ;
void list_free( list_t *lp ) ;
void range_free( range_t *rp ) ;
void branch_free( branch_t *bp ) ;
void boundary_free( boundary_t *bp ) ;
void set_free( set_t *, int ) ;
void element_free( element_t *ep ) ;
void junc_free( junc_t *dbv ) ;
void ref_free( ref_t *ref )  ;

/* element.c */

int        element_cmp( element_t *ea, element_t *eb ) ;

/*
element_t *first_in_tree( element_t *ep ) ;
element_t *next_in_tree( element_t *ep ) ;
element_t *get_first( element_t *ep, element_t **pep ) ;
element_t *get_next( element_t *ep, element_t **pep ) ;
*/

element_t *find_list( element_t *ep, char *tag ) ;
parr_t    *get_simple_lists( element_t *ep, parr_t *ap ) ;

/* hash.c */

phash_t *phash_new( unsigned int pnr, unsigned int density ) ;
buck_t  *phash_insert( phash_t *ht, atom_t *ep, unsigned int key ) ;
buck_t  *phash_insert_bucket( phash_t *ht, buck_t *bp ) ;
void     phash_resize( phash_t *ht ) ;
buck_t  *phash_search( phash_t *ht, atom_t *ap, unsigned int key ) ;
void     phash_free( phash_t *hp )  ;
void     phash_rm_bucket( phash_t *hp, buck_t *bp ) ;
int      phash_index( phash_t *hp ) ;
void     phash_print( phash_t *ht ) ;

phash_t  *phash_dup( phash_t *php, ruleinfo_t *ri ) ;

parr_t  *get_all_atom_followers( branch_t *bp, parr_t *in ) ;
parr_t *prefix2atoms_match( char *prefix, phash_t *ht, parr_t *pa ) ;
parr_t *suffix2atoms_match( char *suffix, phash_t *ht, parr_t *pa ) ;
parr_t *range2atoms_match( range_t *rp, phash_t *ht, parr_t *pa ) ;

void    bucket_rm( phash_t *ht, buck_t *bp ) ;

/* db0.c */

junc_t     *junc_new( void ) ;
junc_t     *junc_dup( junc_t *jp, ruleinfo_t *ri ) ;
db_t       *db_new( void ) ;

junc_t     *branch_add( junc_t *ap, branch_t *dbp ) ;
branch_t   *brancharr_find( junc_t *arr, int type ) ;
void       brancharr_free( junc_t *ap ) ;
int        free_rule( ruleinfo_t *ri, char *uid ) ;
void       free_all_rules( ruleinfo_t *ri ) ;


spocp_result_t  add_right( db_t **db, octet_t **arg, spocp_req_info_t *sri, ruleinst_t **ri ) ;

octet_t        *rulename_print(  ruleinst_t *r, char *rs ) ;
octet_t        *get_blob( ruleinst_t *ri ) ;
ruleinst_t     *get_rule( ruleinfo_t  *ri, char *uid ) ;
spocp_result_t  get_all_rules( db_t *db, octarr_t *oa, spocp_req_info_t *sri, char *rs ) ;
parr_t         *get_rec_all_rules( junc_t *jp, parr_t *in ) ;
parr_t         *get_all_bcond_followers( branch_t *bp, parr_t *in ) ;
int            nrules( ruleinfo_t *ri ) ;
int            rules( db_t *db ) ;

ruleinst_t *save_rule( db_t *db, octet_t *rule, octet_t *blob ) ;
ruleinst_t *aci_save( db_t *db, octet_t *aci ) ;

ruleinst_t *ruleinst_new( octet_t *rule, octet_t *blob ) ;
void       ruleinst_free( ruleinst_t *rt ) ;

ruleinfo_t *ruleinfo_dup( ruleinfo_t *old ) ;
int        ruleinfo_print( ruleinfo_t *r ) ;

void       *read_rules( void *vp, char *file, int *rc, keyval_t **globals ) ;

element_t  *element_dup( element_t *ep, element_t *set, element_t *memberof ) ;
junc_t     *element_add( plugin_t *pl, junc_t *dvp, element_t *ep, ruleinst_t *rt ) ;

raci_t     *saci_new( void ) ;


raci_t     *raci_new( void ) ;
int        P_raci_print( void *vp ) ;
void       *P_raci_dup( void *vp ) ;
void       P_raci_free( void *vp ) ;

/* index.c */

index_t *index_new( int size ) ;
void     index_free( index_t *id ) ;
index_t *index_dup( index_t *id, ruleinfo_t *ri ) ;
index_t *index_add( index_t *id, ruleinst_t *ri ) ;
int      index_rm( index_t *id, ruleinst_t *ri ) ;


/* tree.c */

/*
  link_t    *create_link( rnode_t *rn1, rnode_t *rn2 ) ;
  link_t    *find_link( rnode_t *rn1, rnode_t *rn2 ) ;
  rnode_t   *tree_add( limit_t *lp, boundary_t *bp ) ;
  int        cmp_boundary( boundary_t *b1p, boundary_t *b2p ) ;
  rnode_t   *tree_find( rnode_t *head, boundary_t *bp, int type ) ;
*/

/* subelement.c */

subelem_t *subelement_dup( subelem_t *se ) ;
int       to_subelements( octet_t *arg, octarr_t *oa ) ;
subelem_t *octarr2subelem( octarr_t *oa, int dir ) ;


/* ssn.c */

junc_t *ssn_insert( ssn_t **top, char *str, int direction ) ;
parr_t *ssn_match( ssn_t *pssn, char *sp, int direction ) ;
junc_t *ssn_delete( ssn_t **top, char *sp, int direction ) ;
ssn_t  *ssn_dup( ssn_t *, ruleinfo_t *ri ) ;
parr_t *ssn_lte_match( ssn_t *pssn, char *sp, int direction, parr_t *res ) ;
parr_t *get_all_ssn_followers( branch_t *bp, int type, parr_t *pa ) ;

/* backend.c */

octet_t  *var_sub( octet_t *def, parr_t *lap ) ;
erset_t  *ref_add( erset_t *erp, ref_t *rp ) ;
int        ref_eq( ref_t *r1, ref_t *r2 ) ;
erset_t  *erset_new( size_t size ) ;
erset_t  *erset_dup( erset_t *old, ruleinfo_t *ri ) ;

int       backend_display( void ) ;
backend_t *backend_get( octet_t *type ) ;

int      do_ref( ref_t *rp, parr_t *pap, becpool_t *bcp, octet_t *blob ) ;
erset_t *er_cmp( erset_t *es, element_t *ep ) ;

/* parr.c */

parr_t *parr_new( int size, cmpfn *cf, ffunc *ff, dfunc *df, pfunc *pf ) ;

parr_t *parr_add( parr_t *ap, item_t vp ) ;
parr_t *parr_add_nondup( parr_t *ap, item_t vp ) ;

parr_t *parr_dup( parr_t *ap, item_t ref ) ;

parr_t *parr_join( parr_t *to, parr_t *from ) ;
item_t  parr_common( parr_t *avp1, parr_t *avp2 ) ;
parr_t *parr_or( parr_t *a1, parr_t *a2 ) ;
parr_t *parr_xor( parr_t *a1, parr_t *a2 ) ;
parr_t *parr_and( parr_t *a1, parr_t *a2 ) ;

void    parr_free( parr_t *ap ) ;

item_t  parr_get_item_by_index( parr_t *gp, int i ) ;
int     parr_get_index( parr_t *gp, item_t vp ) ;

int     parr_items( parr_t *gp ) ;

item_t  parr_find( parr_t *ap, item_t val ) ;
int     parr_find_index( parr_t *ap, item_t val ) ;

void    parr_rm_index( parr_t **gpp, int i ) ;
void    parr_rm_item( parr_t **gpp, item_t vp ) ;

void    parr_dec_refcount( parr_t **gpp, int delta ) ;

void    parr_nullterm( parr_t *pp ) ;

parr_t *ll2parr( ll_t *ll ) ;

void   P_null_free( item_t vp ) ;

int    P_strcasecmp( item_t a, item_t b ) ;
int    P_strcmp( item_t a, item_t b ) ;
int    P_pcmp( item_t a, item_t b );
item_t P_strdup( item_t vp );
char   *P_strdup_char( item_t vp );
item_t P_pcpy( item_t i, item_t j );
void   P_free( item_t vp ) ;


/* cache.c */

int     cache_replace_value( ll_t *gp, octet_t *arg, unsigned int ct, int r, octet_t *blob ) ;
int     cached( ll_t *gp, octet_t *arg, octet_t *blob ) ;
int     cache_value( ll_t **harr, octet_t *arg, unsigned int cachetime, int r, octet_t *blob ) ;
void    cacheval_free( void * vp ) ;
void   *cacheval_dup( void *vp) ;

/* print.c */

void    spocp_print( junc_t *dv, int level, int indent ) ;

/* del.c */

/*junc_t *element_rm( junc_t *ap, element_t *ep, int id ) ;*/
spocp_result_t rm_rule( junc_t *ap, octet_t *rule, ruleinst_t *rt ) ;

/* slist.c */

slist_t   *sl_init( int max ) ;
slnode_t  *sl_find( slist_t *slp, boundary_t *item ) ;
parr_t    *sl_match( slist_t *slp, boundary_t *item ) ;
parr_t    *sl_range_match( slist_t *slp, range_t *rp ) ;
junc_t    *sl_range_add( slist_t *slp, range_t *rp ) ;
junc_t    *sl_range_rm( slist_t *slp, range_t *rp, int *rc ) ;
parr_t    *sl_range_lte_match( slist_t *slp, range_t *rp, parr_t *pa ) ;
void       sl_free_slist( slist_t *slp ) ;
slist_t   *sl_dup( slist_t *old, ruleinfo_t *ri ) ; 

parr_t    *get_all_range_followers( branch_t *bp, parr_t *pa ) ;

subelem_t *subelem_new( void ) ;
void       subelem_free( subelem_t *sep ) ;

void        boundary_print( boundary_t *bp ) ;
boundary_t *boundary_dup( boundary_t *bp ) ;
int         boundary_xcmp( boundary_t *b1p, boundary_t *b2p ) ;

void       junc_print( int lev, junc_t *jp ) ;

/* list.c */

spocp_result_t get_matching_rules( db_t *, octet_t *, octarr_t *, spocp_req_info_t *, char * ) ;

/* erset.c */

parr_t  *erset_match( erset_t *erp, element_t *ep, parr_t *pap, becpool_t *b ) ;
erset_t *erset_dup( erset_t *old, ruleinfo_t *ri ) ;
erset_t *erset_new( size_t size ) ;
void     erset_free( erset_t *es ) ;

/* ll.c */

ll_t *ll_new( cmpfn *cf, ffunc *ff, dfunc *df, pfunc *pf ) ;
ll_t *ll_push( ll_t *lp, void *vp, int nodup ) ;
void *ll_pop( ll_t *lp ) ;
void  ll_free( ll_t *lp ) ;
ll_t *ll_dup( ll_t *lp ) ;
void *ll_find( ll_t *lp, void *pattern ) ;
int   ll_rm_item( ll_t *lp, void *pattern, int all ) ;
void  ll_rm_payload( ll_t *lp, void *pl ) ;
int   ll_print( ll_t *lp ) ;

ll_t  *parr2ll( parr_t *pp ) ;

/* dup.c */

/* aci.c */

ruleinst_t *allowing_rule( junc_t *ap ) ;

spocp_result_t allowed(
  junc_t *ap, element_t *ep, parr_t *gap, becpool_t *b, octnode_t **on ) ;

spocp_result_t aci_add( db_t **db, octet_t *arg, spocp_req_info_t *sri ) ;
spocp_result_t allowed_by_aci( db_t *db, octet_t **arg, spocp_req_info_t *sri, char *act ) ;

/* rbtree.c */

rbt_t  *rbt_init( cmpfn *cf, ffunc *ff, kfunc *kf, dfunc *df, pfunc *pf ) ;
int    rbt_insert( rbt_t *bst, item_t item ) ;
int    rbt_remove( rbt_t *rh, Key k ) ;
item_t rbt_search( rbt_t *bst, Key k ) ;
rbt_t  *rbt_dup( rbt_t *org, int dyn ) ;
void   rbt_free( rbt_t *rbt ) ;
int    rbt_items( rbt_t *rbt ) ;
void   rbt_print( rbt_t *rbt ) ;

parr_t *rbt2parr( rbt_t *rh ) ;

/* octnode.c */
octnode_t *octnode_add( octnode_t *on, octet_t *oct, int link ) ;
void       octnode_free( octnode_t *on ) ;
octnode_t *octnode_join( octnode_t *a, octnode_t *b) ;
int        octnode_cmp( octnode_t *a, octnode_t *b ) ;

#endif
