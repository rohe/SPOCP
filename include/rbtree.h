typedef char * item_t ;

typedef unsigned int Key ;

typedef Key (kfunc)( item_t a ) ;
typedef void (ffunc)( item_t a ) ;
typedef item_t (dfunc)( item_t a ) ;
typedef int (cmpfn)( Key a, Key b ) ;

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
