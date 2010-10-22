
/***************************************************************************
                          hash.c  -  description
                             -------------------

    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <string.h>

#include <octet.h>
#include <hash.h>
#include <atom.h>
#include <verify.h>

#include <wrappers.h>
#include <macros.h>
#include <log.h>

/* #define AVLUS 1 */

static void     phash_resize(phash_t * ht);

unsigned int    prime[] = {
	3, 7, 13, 61, 121, 251, 509, 751, 1021, 1499, 2039, 3011, 4093,
	6151, 8191, 12241, 16381, 24709, 32749, 49139, 65521,
	98299, 131071, 196613, 262139, 393209, 524287, 1048573, 2097143,
	4194301, 8388593, 16777213, 33554393, 67108859,
	134217689, 268435399, 536870909, 1073741789,
	2147483647
};

unsigned int    pnrmax = (sizeof(prime) / sizeof(prime[0])) - 1;

phash_t        *
phash_new(unsigned int pnr, unsigned int density)
{
	phash_t        *newht = 0;
	int             lim;

	if (pnr > pnrmax)
		pnr = pnrmax;
	if (density < 1)
		density = 1;
	if (density > 100)
		density = 100;

	newht = (phash_t *) Calloc(1, sizeof(phash_t));

	newht->size = prime[pnr];
	newht->n = 0;
	newht->pnr = pnr;
	newht->density = density;
	newht->arr = (buck_t **) Calloc(newht->size, sizeof(buck_t *));

	lim = (newht->size * density) / 100;
	newht->limit = lim;

	return newht;
}

buck_t  *
buck_new(unsigned int key, octet_t * op)
{
	buck_t         *bp;

	bp = (buck_t *) Calloc(1,sizeof(buck_t));

	bp->hash = key;
	octcpy(&bp->val, op);

	return bp;
}

void 
buck_free( buck_t *b )
{
	if (b){
		octclr(&b->val);
		Free(b);
	}
}

static unsigned int _primary(phash_t * ht, unsigned int key)
{
    return key % ht->size;
}

static unsigned int _secondary(phash_t * ht, unsigned int key)
{
    /* must not be 0 */
    return (key % (prime[ht->pnr / 2] - 1)) + 1;
}

/*
 * If I can't place an item where it 'should' be, use an offset to find a
 * empty place 'closeby' in a deterministic way 
 */
buck_t         *
phash_insert(phash_t * ht, atom_t * ap)
{
	buck_t          **arr = ht->arr, *bp;
	unsigned int    hv1, hv2, shv, key;

    key = ap->hash;
    hv1 = _primary(ht, key);
    shv = hv2 = _secondary(ht, key);

    /* Is the place free or is the same value already stored there */
	while (arr[hv1] && octcmp(&arr[hv1]->val, &ap->val)) {

		hv1 = (hv1 + hv2) % ht->size;

		if (hv1 == shv) {
			phash_resize(ht);
			shv = hv1 = _primary(ht, key);
			hv2 = _secondary(ht, key);
			arr = ht->arr;
		}
	}

	if (arr[hv1] == 0) {
		bp = ht->arr[hv1] = buck_new(key, &ap->val);
		ht->n++;

		if (ht->n > ht->limit)
			phash_resize(ht);

		return bp;
	} else {
#ifdef AVLUS
		oct_print(LOG_DEBUG, "old value", &ap->val);
#endif
		return ht->arr[hv1];
	}
}

static buck_t  *
phash_insert_bucket(phash_t * ht, buck_t * bp)
{
	buck_t        **arr = ht->arr;
	unsigned int    hv1, hv2, shv, key;
    
    key = bp->hash;
    hv1 = _primary(ht, key);
    shv = hv2 = _secondary(ht, key);
    
	while (arr[hv1]) {

		hv1 = (hv1 + hv2) % ht->size;

		if (hv1 == shv) {
			phash_resize(ht);
			shv = hv1 = _primary(ht, key);
			hv2 = _secondary(ht, key);
			arr = ht->arr;
		}
	}

	ht->arr[hv1] = bp;
	ht->n++;

	if (ht->n > ht->limit)
		phash_resize(ht);

	return bp;
}

void
phash_print(phash_t * ht)
{
	int     i;
	buck_t  **ba;
	char    *tmp;

	ba = ht->arr;
	for (i = 0; i < (int) ht->size; i++)
		if (ba[i]) {
			tmp = oct2strdup(&ba[i]->val, '%');
			traceLog(LOG_INFO,"Hash[%d]: %s", i, tmp);
			Free(tmp);
		}
}

char *
phash_package(phash_t * ht) {
    char    str[1024];
    int     i;
	buck_t  **ba;
    
    memset(str, 0, 1024*sizeof(char));
	ba = ht->arr;
    for( i = 0; i < (int) ht->size; i++) {
        if(ba[i])
            str[i] = '^';
        else
            str[i] = '_';
    }
    
    return Strdup(str);
}
        
phash_t        *
phash_dup(phash_t * php, ruleinfo_t * ri)
{
	phash_t        *new;
	buck_t        **oldarr = php->arr;
	buck_t        **newarr;
	int             i;

	new = phash_new(php->pnr, php->density);
	newarr = new->arr;
	new->n = php->n;

	for (i = 0; i < (int) new->size; i++) {
		if (oldarr[i] == 0)
			continue;

		newarr[i] = buck_new(oldarr[i]->hash, &oldarr[i]->val);

		newarr[i]->refc = oldarr[i]->refc;
		newarr[i]->next = junc_dup(oldarr[i]->next, ri);
	}

	return new;
}

static void
phash_resize(phash_t * ht)
{
	buck_t        **newarr, **oldarr;
	unsigned int    i, oldsize, newsize;

	/*
	 * Log("=-=-=-=-=-=-=-=-=-=-=-=-=\n" ) ; print_phash( ht ) ; 
	 */

	oldsize = ht->size;
	newsize = prime[++(ht->pnr)];

	if (newsize == oldsize) {
		/*
		 * this will as a sideeffect shut down the server !! 
		 */
		FatalError("Reached maxsize of phash", 0, 0);
	}

	newarr = (buck_t **) Calloc(newsize, sizeof(buck_t *));

	oldarr = ht->arr;
	ht->arr = newarr;
	ht->size = newsize;
	ht->n = 0;		/* since hash_insert will count it up anyway */

	ht->limit = (ht->size * ht->density) / 100;

	/*
	 * reinsert the strings already placed in the old phash 
	 */

	for (i = 0; i < oldsize; i++)
		if (oldarr[i])
			phash_insert_bucket(ht, oldarr[i]);

	Free(oldarr);
	/*
	 * print_phash( ht ) ; 
	 */
}
/*
 * Returns the place in the hash array a specific bucket occupies ?
 */
static int
phash_bucket_index(phash_t * ht, buck_t * bp, size_t * rc)
{
	buck_t        **arr = ht->arr, *ap;
	unsigned int    hv1, hv2, shv, key;
    
    key = bp->hash;
    hv1 = _primary(ht, key);
    shv = hv2 = _secondary(ht, key);
    
	if (ht->n == 0)
		return 0;

	while (arr[hv1] != 0) {
		ap = arr[hv1];
		if (bp == ap) {
			*rc = hv1;
			return 1;
		}

		hv1 = (hv1 + hv2) % ht->size;
	}

	return 0;
}

buck_t         *
phash_search(phash_t * ht, atom_t * ap)
{
	buck_t        **arr = ht->arr, *bp;
	unsigned int    hv1, hv2, shv, key;
    
    key = ap->hash;
    hv1 = _primary(ht, key);
    shv = hv2 = _secondary(ht, key);

	if (ht->n == 0)
		return 0;

	while (arr[hv1] != 0) {
		bp = arr[hv1];
		if (bp->hash == key && octcmp(&bp->val, &ap->val) == 0)
			return bp;

		hv1 = (hv1 + hv2) % ht->size;
	}

	return 0;
}

/* Only called from phash_free, which means that everything should be removed */
static void
bucket_free(buck_t * bp)
{
	if (bp) {
		DEBUG(SPOCP_DSTORE) {
			oct_print(LOG_DEBUG, "removing bucket with value", &bp->val);
		}
		/*
	 	* bp->val.val shouldn't be touched since it's pointing to
	 	* something that is freed elsewhere 
	 	*/
		if (bp->next)
			junc_free(bp->next);

		buck_free(bp);
		DEBUG( SPOCP_DSTORE) {
			traceLog(LOG_DEBUG,"bucket removed");
		}
	}
}

void
phash_free(phash_t * ht)
{
	buck_t        **arr;
	unsigned int    i;

	if (ht) {
		DEBUG(SPOCP_DSTORE) {
			traceLog(LOG_DEBUG,"removing hash struct");
		}
		if (ht->arr) {
			arr = ht->arr;
			for (i = 0; i < ht->size; i++)
				if (arr[i] != 0)
					bucket_free(arr[i]);

			Free(ht->arr);
		}
		Free(ht);

		DEBUG(SPOCP_DSTORE) {
			traceLog(LOG_DEBUG,"hash struct removed");
		}
	}
}

/* removing values in one bucket means I have to recreate the hash struct
 * since it might look different with that specific bucket empty 
 */
void
bucket_rm(phash_t * ht, buck_t * bp)
{
	buck_t  **arr = ht->arr;
	buck_t  **newarr;
	size_t  n, size = ht->size;
	size_t  rc = 0;
	int     i;

    /* printf("[bucket_rm]\n"); */
	i = phash_bucket_index(ht, bp, &rc);
    /* printf("[bucket_rm] bucket index = %d\n", i); */
	DEBUG(SPOCP_DSTORE) 
        traceLog(LOG_DEBUG, "bucket index = %d", i);

	if (i == 0)
		return;

	Free(bp);
	ht->arr[rc] = 0;

    /* printf("[bucket_rm] Rearrange\n"); */
	/*
	 * have to rearrange the buckets, since otherwise I might get 'holes' 
	 */
	newarr = (buck_t **) Calloc(size, sizeof(buck_t *));

	ht->arr = newarr;
	ht->n = 0;		/* since hash_insert will count it up anyway */

	/*
	 * reinsert the strings already placed in the old phash 
	 */

	for (n = 0; n < size; n++)
		if (arr[n])
			phash_insert_bucket(ht, arr[n]);

	Free(arr);
}

/* 
 * returns 0 if not true else non-zero 
 */
int
phash_empty(phash_t * ht)
{
	return (ht->n == 0);
}

