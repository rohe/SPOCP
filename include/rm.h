/*
 *  rm.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 2/4/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __RM_H
#define __RM_H

#include <octet.h>
#include <ruleinst.h>
#include <element.h>
#include <branch.h>

spocp_result_t  rule_rm(junc_t * jp, octet_t * rule, ruleinst_t * rt);
int             element_rm(junc_t * jp, element_t * ep, ruleinst_t * rt);

#endif
