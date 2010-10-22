/*
 *  bcondfunc.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/26/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __BCONDFUNC_H
#define __BCONDFUNC_H

#include <octet.h>
#include <bcond.h>
#include <plugin.h>
#include <check.h>
#include <dback.h>
#include <ruleinst.h>
#include <element.h>


bcdef_t         *bcdef_add(bcdef_t **rbc, plugin_t *p, dbackdef_t *dbc, 
                           octet_t *name, octet_t *data);
spocp_result_t  bcdef_del(bcdef_t **rbc , dbackdef_t * dbc, octet_t * name);
bcdef_t         *bcdef_get(bcdef_t **rbc, plugin_t * p, dbackdef_t * dbc, 
                           octet_t * o, spocp_result_t * rc);
spocp_result_t  bcond_check(element_t * ep, ruleinst_t *ri, octarr_t ** oa, 
                            checked_t **cr);
varr_t          *bcond_users(bcdef_t *rbc, octet_t * bcname);
spocp_result_t  bcexp_eval(element_t * qp, element_t * rp, bcexp_t * bce, 
                           octarr_t ** oa);
int             bcspec_is(octet_t * spec);

#endif