/***************************************************************************
                          db0.h  -  description
                             -------------------
    begin                : Mon Jul 15 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

  COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#ifndef __DB0_H__
#define __DB0_H__

#include <config.h>

#include <struct.h>
#include <plugin.h>

#define SHA1HASHLEN 40

struct _branch ;
struct _ruleinstance ;

/*********** where all the barnches starts *************/

typedef struct {
  struct _branch *item[NTYPES] ;
} junc_t ;

#define ARRFIND(a,c) ( (a)->item[(c)] )

/***********************************************/

typedef struct _index {
  int size ;
  int n ;
  struct _ruleinstance **arr ;
} index_t ;

/*********** for prefixes ****************/

typedef struct _ssn {
  unsigned char  ch ;
  struct _ssn   *left ;
  struct _ssn   *down ;
  struct _ssn   *right ;
  junc_t        *next ;
  unsigned int   refc ; /* reference counter */
} ssn_t ;

/*********** for ranges ******************/

typedef struct _rnode {
  int            ss ;
  unsigned int   refc ;

  boundary_t    *item ;

  struct _rnode *left ;
  struct _rnode *right ;
  struct _rnode *up ;

  parr_t        *gt ;
  parr_t        *lt ;
  parr_t        *eq ;
  parr_t        *tot ;
} rnode_t ;

typedef struct _link
{
  rnode_t  *other ;
  junc_t   *next ;
} link_t ;

typedef struct _limit {
  rnode_t   *limit ;
} limit_t ;

/****** for ranges *********************/

typedef struct _slnode {
  boundary_t      *item ;
  unsigned int     refc ;
  struct _slnode **next ;
  int              sz ;
  parr_t          *junc ;
} slnode_t ;


typedef struct _slist {
  slnode_t *head ;
  slnode_t *tail ;
  int       n ;
  int       lgn ;
  int       lgnmax ;
  int       type ;
} slist_t ;

/****** for atoms **********************/

typedef struct _bucket {
  octet_t           val ;
  unsigned long int hash ;
  unsigned int      refc ;
  junc_t            *next ;
} buck_t ;

typedef struct _phash {
  unsigned char pnr ;
  unsigned char density ;
  unsigned int  size ;
  unsigned int  n ;
  unsigned int  limit ;
  buck_t      **arr ;
} phash_t ;

/****** caching **********************/

typedef struct {
  unsigned int  hash ;
  octet_t      *arg ;
  octet_t       blob ;
  unsigned int  timeout ;
  int           res ;
} cacheval_t ;

/****** for boundary conditions **********************/

typedef struct _ref {
  plugin_t        *plugin ;
  octet_t          bc ;
  octet_t          arg ;
  ll_t            *cachedval ;
  unsigned int    ctime ;
  unsigned int    refc ;
  int             inv ;
  junc_t          *next ;

#ifdef HAVE_LIBPTHREAD
  pthread_mutex_t mutex ;
#endif

  struct _ref     *nextref ;
} ref_t ;

typedef struct _backend {
  char        *name ;
  befunc      *func ;
  beinitfn    *init ;
  cachetime_t *ct ;
} backend_t ;

typedef struct _erset {
  size_t        n ;
  size_t        size ;
  ref_t         **ref ;
  junc_t        *next ;
  struct _erset *other ;
} erset_t ;

/****** branches **********************/

typedef struct _branch {
  int      type ;
  int      count ;
  junc_t  *parent ;

  union {
    erset_t  *set ;
    phash_t  *atom ;
    junc_t   *list ;
    ssn_t    *prefix ;
    ssn_t    *suffix ; 
    slist_t  *range[DATATYPES] ;
    index_t  *id ;
  } val ;

} branch_t ;

/*********** ACL info  ****************/

#define ACL_COMP   0  /* always on */
#define ACL_READ   1
#define ACL_ADD    2
#define ACL_DELETE 4
#define ACL_CHECK  8

/*------------------------------------*/
/* who can do what with which rule    */

typedef struct _rule_aci {
  subelem_t            *resource ;
  unsigned int         action ;   /* bitmap with permissions */
  element_t            *subject ;
  struct _ruleinstance *ri ;      /* links back and forth */
} raci_t ;

/*********** rule info ****************/

typedef struct _ruleinstance {
  char       uid[SHA1HASHLEN+1] ; /* place for sha1 hash */
  octet_t    *rule ;
  octet_t    *blob ;
  element_t  *ep ;     /* only used for acis */
  raci_t     *raci ;   /* only used if this rule is ACI */
  ll_t       *alias ;
  ll_t       *aci ;    /* pointers to access control information */
} ruleinst_t ;

typedef struct _ruleinfo {
  rbt_t   *rules ;     /* red/black tree */
} ruleinfo_t ;

/*********** The database *************/

typedef struct _db {
  junc_t      *jp ;
  ruleinfo_t  *ri ;
  junc_t      *acit ;
  ruleinfo_t  *raci ;
  becpool_t   *bcp ;
  plugin_t    *plugins ;
} db_t ;

#endif
