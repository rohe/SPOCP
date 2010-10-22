/*
 *  hash.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/8/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef _HASH_H
#define _HASH_H

#include <octet.h>
#include <basefn.h>
#include <ruleinfo.h>
#include <branch.h>


buck_t  *buck_new(unsigned int key, octet_t * op);
void    buck_free( buck_t *b );
void    bucket_rm(phash_t * ht, buck_t * bp);

phash_t	*phash_new(unsigned int pnr, unsigned int density);
buck_t	*phash_insert(phash_t * ht, atom_t * ep);
buck_t	*phash_search(phash_t * ht, atom_t * ap);
int     phash_empty(phash_t * hp);
void	phash_free(phash_t * hp);
void	phash_rm_bucket(phash_t * hp, buck_t * bp);
void	phash_print(phash_t * ht);
char    *phash_package(phash_t * ht);

phash_t	*phash_dup(phash_t * php, ruleinfo_t * ri);

#endif