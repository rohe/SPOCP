/***************************************************************************
                          struct.h  -  description
                             -------------------
    begin                : Mon Jun 3 2002
    copyright            : (C) 2002 by SPOCP
    email                : roland@catalogix.se
 ***************************************************************************/

#ifndef __STRUCT_H
#define __STRUCT_H

#include <config.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <regex.h>

#include <spocp.h>

#define FALSE          0
#define TRUE           1

#define SPOC_ATOM      0
#define SPOC_LIST      1
/* #define SPOC_AND       2 */
#define SPOC_PREFIX    3
#define SPOC_SUFFIX    4
#define SPOC_RANGE     5
#define SPOC_ENDOFLIST 6
#define SPOC_ENDOFRULE 7
#define SPOC_BCOND     8
#define SPOC_OR        9 /* not really a type of it's own */
#define SPOC_REPEAT   10
/* 
*/
#define SPOC_EXTREF   11

#define NTYPES        11

#define START          0

#define SPOC_DATE      0
#define SPOC_TIME      1
#define SPOC_ALPHA     2
#define SPOC_IPV4      3
#define SPOC_NUMERIC   4
#define SPOC_IPV6      5

#define DATATYPES      6 /* make sure this is correct */

#define TTIME          1

#define LT            0x10
#define GLE           0x20
#define GT            0x40

#define DEF_LINK       4

#define CACHED         2
#define EXPIRED        4

#define FORWARD        1
#define BACKWARD       2

#define QUERY    0x01
#define LIST     0x02
#define ADD      0x04
#define REMOVE   0x08
#define BEGIN    0x10
#define ALIAS    0x20
#define COPY     0x40

#define SUBTREE  0x04
#define ONELEVEL 0x02
#define BASE     0x01

/* ------------------------------------- *

typedef struct _octet {
  size_t      len ;
  size_t      size ;
  char        *val ;
} octet_t ;

 * ------------------------------------- */

struct _be_cpool ;

typedef void * item_t ;
typedef char * Key ;

/* free function */
typedef void (ffunc)( item_t vp ) ;

/* duplicate function */
typedef item_t (dfunc)( item_t a, item_t b ) ;

/* copy function */
typedef int (cfunc)( item_t dest, item_t src ) ;

/* compare function */
typedef int (cmpfn)( item_t a, item_t b ) ;

/* print function */
typedef char *(pfunc)( item_t vp ) ;

/* key function, normally a hashfunction */
typedef char *(kfunc)( item_t ) ;


/*
*/

typedef struct _atom {
  unsigned int  hash ;
  octet_t       val ;
} atom_t ;

struct _list ;
struct _range ;
struct _set ;

typedef struct _element {
  int   not ;
  int   type ;

  struct _element *next ;
/*  struct _element *set ; */
  struct _element *memberof ;

  union {
    atom_t           *atom ;
    struct _list     *list ;
    struct _set      *set ;
    atom_t           *prefix ;
    atom_t           *suffix ;
    struct _range    *range ;
    int               id ;
  } e ;

} element_t ;

/****** and structure **********************/

typedef struct _set {
  /* couple of terminated branches */
  int       n ;
  int       size ;
  element_t **element ;
} set_t ;


typedef struct _list {
  unsigned int hstrlist ; /* hash of the list string, of limit usage */
  element_t   *head ;
} list_t ;

typedef struct _subelem {
  int              direction ;
  int              list ;       /* yes = 1, no = 0 */
  element_t       *ep ;
  struct _subelem *next ; 
} subelem_t ;

typedef struct _parr {
  int    n ;
  int    size ;
  int   *refc ;
  void  **vect ;
  cmpfn *cf ;      /* compare function */
  ffunc *ff ;      /* free function */
  dfunc *df ;      /* duplicate function */
  pfunc *pf ;      /* print function */
} parr_t ;

typedef struct _node {
  void         *payload ;
  struct _node *next ;
  struct _node *prev ;
} node_t ;

typedef struct _ll {
  node_t  *head ;
  node_t  *tail ;
  cmpfn   *cf ;
  ffunc   *ff ;
  dfunc   *df ;
  pfunc   *pf ;
  int      n ;
} ll_t ;

typedef struct _boundary {
  int                 type ;

  union {
    long int          num ;
    struct in_addr    v4;
    struct in6_addr   v6;
    octet_t           val ;
  } v ;

} boundary_t ;

typedef struct _range
{
  boundary_t     lower ;
  boundary_t     upper ;
} range_t ;

/* ------------------------------------------ */

typedef struct _strarr {
  char **argv ;
  int    argc ;
  int    size ;
} strarr_t ;

/* ------------------------------------------ */

/*
typedef struct _keyval {
  octet_t  *key ;
  octet_t  *val ;
  struct _keyval *next ;
} keyval_t ;
*/
/* ------------------------------------------ */

typedef struct _rbnode {
  item_t item ;
  Key    v ;
  int    n ; 
  char   red ;
  struct _rbnode *l ;
  struct _rbnode *r ;
  struct _rbnode *p ;
} rbnode_t ;

typedef struct _rbt {
  cmpfn    *cf ;
  ffunc    *ff ;
  kfunc    *kf ;
  pfunc    *pf ;
  dfunc    *df ;
  rbnode_t *head ;
} rbt_t ;

/* ------------------------------------------ */

/* === global things === */

extern int   spocp_loglevel ;

#define SPOCP_MAXLINE       2048
#define SPOCP_INPUTBUFSIZE  4096

#endif
