/*
 *  match.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 2/2/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __MATCH_H
#define __MATCH_H

#include <atom.h>
#include <ssn.h>
#include <element.h>
#include <branch.h>
#include <ruleinst.h>
#include <db0.h>
#include <resset.h>

junc_t      *atom2atom_match(atom_t * ap, phash_t * pp);
varr_t      *atom2prefix_match(atom_t * ap, ssn_t * pp);
varr_t      *atom2suffix_match(atom_t * ap, ssn_t * pp);
varr_t      *prefix2prefix_match(atom_t * prefa, ssn_t * prefix);
varr_t      *suffix2suffix_match(atom_t * prefa, ssn_t * suffix);

resset_t    *element_match_r(junc_t * db, element_t * ep, comparam_t * comp);

/* ---------------- aci.c --------------------- */

ruleinst_t      *allowing_rule(junc_t *jp);

spocp_result_t  allowed(junc_t * jp, comparam_t *comp, resset_t **rspp);

#endif