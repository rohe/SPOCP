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

#ifndef FALSE
#define FALSE          0
#endif
#ifndef TRUE
#define TRUE           1
#endif

#define SPOC_ATOM      0
#define SPOC_LIST      1
#define SPOC_SET       2 
#define SPOC_PREFIX    3
#define SPOC_SUFFIX    4
#define SPOC_RANGE     5
#define SPOC_ENDOFLIST 6
#define SPOC_ENDOFRULE 7
#define SPOC_ANY       8 
/* --- */
#define SPOC_ARRAY     9 /* only used in and around backends */
#define SPOC_NULL     10 /* NULL element, necessary placeholder */
/*
#define SPOC_OR        9 
#define SPOC_REPEAT   10
#define SPOC_EXTREF   11
*/

#define NTYPES         9

/* ---------------------------------------------------------------------- */

#define SPOC_DATE      0
#define SPOC_TIME      1
#define SPOC_ALPHA     2
#define SPOC_IPV4      3
#define SPOC_NUMERIC   4
#define SPOC_IPV6      5

#define DATATYPES      6 /* make sure this is correct */

#define LT          0x10
#define GLE         0x20
#define GT          0x40

#define EXPIRED     1
#define CACHED      2 

/*
#define LINK        0x10

#define START          0

#define DEF_LINK       4
#define TTIME          1

#define FORWARD        1
#define BACKWARD       2

#define QUERY       0x01
#define LIST        0x02
#define ADD         0x04
#define REMOVE      0x08
#define BEGIN       0x10
#define ALIAS       0x20
#define COPY        0x40

#define SUBTREE     0x04
#define ONELEVEL    0x02
#define BASE        0x01
*/

/* ---------------------------------------------------------------------- */

typedef struct _atom {
  unsigned int  hash ;
  octet_t       val ;
} atom_t ;

struct _list ;
struct _range ;

typedef struct _element {
  int   not ;
  int   type ;

  struct _element *next ;
  struct _element *memberof ; /* points to the set or list root */

  union {
    atom_t           *atom ; /* suffix and prefix is also placed here */
    struct _list     *list ;
    struct _varr     *set ;
    struct _range    *range ;
    int               id ;
  } e ;

} element_t ;

/****** and structure **********************/

typedef struct _list {
  unsigned int hstrlist ; /* hash of the list string, of limit usage */
  element_t   *head ;
} list_t ;

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

/* === global things === */

extern int   spocp_loglevel ;

#define SPOCP_MAXLINE       2048
#define SPOCP_INPUTBUFSIZE  4096

#endif
