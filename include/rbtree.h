#ifndef __RBTREE_H_
#define __RBTREE_H_

#include <struct.h>
#include <basefn.h>
#include <varr.h>

struct _varr ;

#define hl h->l 
#define hr h->r
#define hll h->l->l
#define hrr h->r->r

typedef struct _rbnode {
  item_t item ;
  Key    v ;
  int    n ; 
  char   red ;
  struct _rbnode *l ;
  struct _rbnode *r ;
  struct _rbnode *p ;
} rbnode_t ;

typedef struct _rbhead {
  cmpfn    *cf ;
  ffunc    *ff ;
  kfunc    *kf ;
  rbnode_t *root ;
} rbhead_t ;

typedef struct _rbt {
  cmpfn    *cf ;
  ffunc    *ff ;
  kfunc    *kf ;
  pfunc    *pf ;
  dfunc    *df ;
  rbnode_t *head ;
} rbt_t ;

rbt_t  *rbt_init( cmpfn *cf, ffunc *ff, kfunc *kf, dfunc *df, pfunc *pf ) ;
int    rbt_insert( rbt_t *bst, item_t item ) ;
int    rbt_remove( rbt_t *rh, Key k ) ;
item_t rbt_search( rbt_t *bst, Key k ) ;
rbt_t  *rbt_dup( rbt_t *org, int dyn ) ;
void   rbt_free( rbt_t *rbt ) ;
int    rbt_items( rbt_t *rbt ) ;
void   rbt_print( rbt_t *rbt ) ;

struct _varr *rbt2varr( rbt_t *rh ) ;

#endif

