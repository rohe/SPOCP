/*
 *  list.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 2/3/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __LIST_H
#define __LIST_H

#include <octet.h>
#include <db0.h>
#include <subelem.h>

varr_t          *adm_list(junc_t *, subelem_t *, spocp_result_t *);
subelem_t       *pattern_parse(octarr_t * pattern);
spocp_result_t  get_matching_rules(db_t *, octarr_t *, octarr_t **, char *);

#endif
