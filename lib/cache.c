
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
#include <struct.h>
#include <plugin.h>
#include <varr.h>
#include <db0.h>
#include <func.h>
#include <wrappers.h>

#define MAX_CACHED_VALUES 1024

/*
 * ======================================================================= 
 */

/**********************************************************
*               Hash an array of strings                  *
**********************************************************/
/*
 * Arguments: argc number of strings argc the vector containing the strings
 * 
 * Returns: unsigned integer representing the total hashvalue 
 */
/*
 * static unsigned int hash_arr( int argc, char **argv ) { int i ; unsigned
 * int hashv = 0 ;
 * 
 * for( i = 0 ; i < argc ; i++ ) hashv = lhash( (unsigned char *) argv[i],
 * strlen( argv[i] ), hashv ) ;
 * 
 * return hashv ; } 
 */

/**********************************************************
*          Create a new structure for cached values       *
**********************************************************/
/*
 * Arguments: h the hashvalue calculated for arg ct cache time r the result of 
 * evaluation of the external reference (1/0)
 * 
 * Returns: a cacheval struct with everything filled in 
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
 * static void *cacheval_dup( void *vp) { cacheval_t *new, *old ;
 * 
 * old = ( cacheval_t * ) vp ;
 * 
 * new = cacheval_new( old->hash, 0, old->res ) ; new->timeout = old->timeout
 * ;
 * 
 * return (void *) new ; } 
 */

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

/***********************************************************
* Cache a result of the evaluation of a external reference *
***********************************************************/
/*
 * Arguments: harr The pointer to a array to which the result should be added
 * arg The argument, normally a external reference after variable
 * substitutions ct cache time r the result of evaluation of the external
 * reference (1/0)
 * 
 * Returns: TRUE if OK 
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

/************************************************************************
* Checks whether there exists a cache value for this specific reference *
************************************************************************/
/*
 * Arguments: gp Pointer to a array where results are cached for this type of
 * reference arg The argument, normally a external reference after variable
 * substitutions
 * 
 * Returns: CACHED | result(0/1) 
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

/************************************************************
*          remove a previously cached result              *
************************************************************/
/*
 * Arguments: gp Pointer to a array where results are cached for this type of
 * reference arg The argument, normally a external reference after variable
 * substitutions ct Number of seconds this result can be cached r The result
 * to be cached
 * 
 * Returns: TRUE if OK 
 */

void
cached_rm(cache_t * cp, octet_t * arg)
{
	cacheval_t     *cvp;

	if ((cvp = cache_find(cp, arg)))
		return;

	cache_del(cp, cvp);
}
