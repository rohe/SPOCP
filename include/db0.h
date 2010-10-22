
/***************************************************************************
				db0.h	-  description
					-------------------
	begin		 : Mon Jul 15 2002
	copyright		 : (C) 2002 by Umeå University, Sweden
	email		 : roland@catalogix.se

	COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
	of this package for details.

 ***************************************************************************/

#ifndef __DB0_H__
#define __DB0_H__

#include <config.h>

#include <element.h>
#include <plugin.h>
#include <basefn.h>
#include <rdb.h>
#include <ll.h>
#include <varr.h>
/*#include <rbtree.h> */
#include <dback.h>
#include <ruleinst.h>
#include <branch.h>
#include <result.h>
#include <check.h>

/*
 * ------------------------------------------ 
 */

#define FORWARD     1
#define BACKWARD	2

/*
 * ------------------------------------------ 
 */

typedef struct _strarr {
	char	**argv;
	int		argc;
	int		size;
} strarr_t;

/*
 * -------------------------------------------------- 
 */


typedef struct _com_param {
	element_t       *head;
	octarr_t        **blob;
	spocp_result_t	rc;
	int             all;
	int             nobe;
	checked_t       **cr;
} comparam_t;


/*********** The database *************/

typedef struct _db {
	junc_t		*jp;
	ruleinfo_t	*ri;
/*  junc_t		*acit; */
	plugin_t	*plugins;
/*	bcdef_t		*bcdef; */
	dback_t		*dback;
} db_t;

db_t            *db_new(void);
ruleinst_t      *save_rule(db_t * db, dbackdef_t * dbc, octet_t * rule, 
                           octet_t * blob, char *bcondname);
int             rules(db_t * db);
spocp_result_t  get_all_rules(db_t * db, octarr_t ** oa, char *rs);
void            db_clr( db_t *db);
void            db_free( db_t *db);
spocp_result_t  store_right(db_t * db, element_t * ep, ruleinst_t * rt);
spocp_result_t  add_right(db_t ** db, dbackdef_t * dbc, octarr_t * oa, 
                          ruleinst_t ** ri, bcdef_t * bcd);

#endif
