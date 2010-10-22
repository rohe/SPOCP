/*
 *  cache.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/4/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __CACHE_H_
#define __CACHE_H_

#include <config.h>

#include <time.h>
#if defined HAVE_LIBPTHREAD || defined HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include <octet.h>
#include <varr.h>
#include <rdwr.h>

/*! \brief Struct for keeping cached values */ 
typedef struct {
	/*! A hash of the query that produced this cachevalue */
	unsigned int    hash;
	/*! A 'dynamic' blob to be returned */
	octet_t         blob;
	/*! When this cache becomes invalid */
	unsigned int    timeout;
	/*! The cached result */
	int             res;
} cacheval_t;

/*! \brief Information about how long things should be cached */
typedef struct cachetime_t {
	/*! The cache time */
	time_t          limit;
	/*! A pattern that can be applied to a query to see if this information should
	 *  be used when caching */
	octet_t         pattern;
	/*! The next piece of caching information */
	struct cachetime_t *next;
} cachetime_t;

/*! \brief A set of cached results */
typedef struct {
	/*! an array in which to store the cached results */
	varr_t          *va;
#if defined HAVE_LIBPTHREAD || defined HAVE_PTHREAD_H
	/*! A read/write lock on the cache */
	pthread_rdwr_t  rw_lock;
#endif
} cache_t;

/*
 * ------------------------------------ 
 */

time_t          cachetime_set(octet_t * str, cachetime_t * ct);
cachetime_t    *cachetime_new(octet_t * s);
void            cachetime_free(cachetime_t * s);
void            cache_free(cache_t *cp );
cache_t        *cache_value(cache_t *, octet_t *, unsigned int, int,
                            octet_t *);
int             cached(cache_t * cp, octet_t * arg, octet_t * blob);
void            cached_rm(cache_t * cp, octet_t * arg);

#endif
