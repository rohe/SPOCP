/*!
 * \file cache.c
 * \author Roland Hedberg <roalnd@catalogix.se>
 * \brief functions for set and retireving cached results
 */

/***************************************************************************
                          cache.c  -  description
                             -------------------

    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <time.h>
#include <string.h>

#include <spocp.h>
#include <plugin.h>
#include <varr.h>
#include <wrappers.h>

/*! The maximum number of results to cache, perhaps this should be configurable */
#define MAX_CACHED_VALUES 1024

/*! \brief Calculates the hash over an array of bytes
 * \param s The byte array
 * \param len The length of the byte array
 * \param init Seed
 * \return A number
*/
unsigned int lhash(unsigned char *s, unsigned int len, unsigned int init);
/*
 * ======================================================================= 
 */

static cacheval_t *
cacheval_new(unsigned int h, unsigned int ct, int r)
{
	cacheval_t     *cvp;
	time_t          t;

	time(&t);

	cvp = (cacheval_t *) Malloc(sizeof(cacheval_t));

	cvp->timeout = t + ct;
	cvp->hash = h;
	cvp->res = r;
	memset(&cvp->blob, 0, sizeof(octet_t));

	return cvp;
}

static void
cacheval_free(cacheval_t * cvp)
{
	if (cvp) {
		free(cvp);
	}
}

/*
 * ---------------------------------------------------------------------- 
 */

static cache_t *
cache_new(void)
{
	cache_t        *new;

	new = (cache_t *) Calloc(1, sizeof(cache_t));

#ifdef HAVE_LIBPTHREAD
	pthread_rdwr_init(&new->rw_lock);
#endif

	return new;
}

static cache_t *
cache_add(cache_t * c, void *v)
{
	if (c == 0)
		c = cache_new();

	c->va = varr_add(c->va, v);

	return c;
}

static cacheval_t *
cache_find(cache_t * c, octet_t * arg)
{
	unsigned int    hash;
	cacheval_t     *cvp;
	void           *vp;

	hash = lhash((unsigned char *) arg->val, arg->len, 0);
	/*
	 * traceLog( "Hash: %u", hash ) ; 
	 */

	for (vp = varr_first(c->va); vp; vp = varr_next(c->va, vp)) {
		cvp = (cacheval_t *) vp;
		/*
		 * traceLog( "Stored hash: %u (%d)", cvp->hash, cvp->res ) ; 
		 */
		if (cvp->hash == hash)
			return cvp;
	}

	return 0;
}

static void
cache_del(cache_t * c, cacheval_t * cvp)
{
	varr_rm(c->va, (void *) cvp);
	cacheval_free(cvp);
}

static void
cache_fifo_rm(cache_t * cp)
{
	cacheval_t     *cv;

	cv = (cacheval_t *) varr_fifo_pop(cp->va);
	cacheval_free(cv);
}

/*
 * ---------------------------------------------------------------------- 
 */

/*!
 * \brief Cache a result of the evaluation of a external reference *
 * \param cp A pointer to thye cached results
 * \param arg The argument, normally a external reference after variable
 * substitutions 
 * \param ct cache time 
 * \param r the result of evaluation of the external reference (1/0)
 * \param blob A 'dynamic' blob that should be return.
 * 
 * \return A pointer to the cache of cached results
 */

cache_t        *
cache_value(cache_t * cp, octet_t * arg, unsigned int ct, int r,
	    octet_t * blob)
{
	unsigned int    hash;
	cacheval_t     *cvp;

	hash = lhash((unsigned char *) arg->val, arg->len, 0);

	cvp = cacheval_new(hash, ct, r);

	if (cp == 0)
		cp = cache_new();

	/*
	 * max number of cachevalues should be configurable 
	 */
	if (cp->va && varr_len(cp->va) == MAX_CACHED_VALUES) {
		cache_fifo_rm(cp);	/* to make room, remove the oldest */
	}

	if (!cp)
		cp = cache_new();

	cache_add(cp, (void *) cvp);

	if (blob && blob->len) {
		cvp->blob.len = blob->len;
		cvp->blob.val = blob->val;
		cvp->blob.size = blob->size;
		blob->size = 0;	/* should be kept and not deleted upstream */
	}

	return cp;
}

/*!
 * \brief Checks whether there exists a cache value for this specific reference 
 * \param cp Pointer to a array where results are cached for this type of
 * reference 
 * \param arg The argument, normally a external reference after variable
 * substitutions
 * \param blob A 'dynamic' blob that should be cached.
 * \return CACHED | result(0/1) 
 */

int
cached(cache_t * cp, octet_t * arg, octet_t * blob)
{
	cacheval_t     *cvp;
	time_t          t;

	if (cp == 0)
		return 0;

	if ((cvp = cache_find(cp, arg)) == 0)
		return 0;

	time(&t);

	if ((time_t) cvp->timeout < t)
		return EXPIRED;

	if (blob) {
		blob->len = cvp->blob.len;
		blob->val = cvp->blob.val;
		blob->size = 0;	/* static data, shouldn't be deleted upstream */
	}

	return cvp->res;
}

/*!
 * \brief  remove a previously cached result
 * \param cp Pointer to a array where results are cached for this type of
 * reference
 * \param arg The argument, normally a external reference after variable
 * substitutions
 */

void
cached_rm(cache_t * cp, octet_t * arg)
{
	cacheval_t     *cvp;

	if ((cvp = cache_find(cp, arg)))
		return;

	cache_del(cp, cvp);
}
