
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

#include <config.h>

#include <func.h>
#include <db0.h>
#include <wrappers.h>
#include <macros.h>

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

static buck_t  *
buck_new(unsigned int key, octet_t * op)
{
	buck_t         *bp;

	bp = (buck_t *) Malloc(sizeof(buck_t));

	bp->hash = key;
	bp->val.size = 0;
	octcpy(&bp->val, op);
	bp->refc = 0;
	bp->next = 0;

	return bp;
}

/*
 * If I can't place an item where it 'should' be, use an offset to find a
 * empty place 'closeby' in a deterministic way 
 */
buck_t         *
phash_insert(phash_t * ht, atom_t * ap, unsigned int key)
{
	unsigned int    hv1 = key % ht->size;
	unsigned int    hv2 = (key % (prime[ht->pnr / 2] - 1)) + 1;	/* must 
									 * not 
									 * be
									 * 0 */
	unsigned int    shv = hv1;
	buck_t        **arr = ht->arr, *bp;

	while (arr[hv1] && octcmp(&arr[hv1]->val, &ap->val)) {

		hv1 = (hv1 + hv2) % ht->size;

		if (hv1 == shv) {
			phash_resize(ht);
			shv = hv1 = key % ht->size;
			hv2 = (key % (prime[ht->pnr / 2] - 1)) + 1;	/* must 
									 * not 
									 * be
									 * 0 */
			arr = ht->arr;
		}
	}

	if (arr[hv1] == 0) {
		bp = ht->arr[hv1] = buck_new(key, &ap->val);
		ht->n++;

		if (ht->n > ht->limit)
			phash_resize(ht);

		return bp;
	} else
		return ht->arr[hv1];
}

static buck_t  *
phash_insert_bucket(phash_t * ht, buck_t * bp)
{
	unsigned int    key = bp->hash;
	unsigned int    hv1 = key % ht->size;
	unsigned int    hv2 = (key % (prime[ht->pnr / 2] - 1)) + 1;	/* must 
									 * not 
									 * be
									 * 0 */
	unsigned int    shv = hv1;
	buck_t        **arr = ht->arr;

	while (arr[hv1]) {

		hv1 = (hv1 + hv2) % ht->size;

		if (hv1 == shv) {
			phash_resize(ht);
			shv = hv1 = key % ht->size;
			hv2 = (key % (prime[ht->pnr / 2] - 1)) + 1;	/* must 
									 * not 
									 * be
									 * 0 */
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
	int             i;
	buck_t        **ba;
	char           *tmp;

	ba = ht->arr;
	for (i = 0; i < (int) ht->size; i++)
		if (ba[i]) {
			tmp = oct2strdup(&ba[i]->val, '%');
			traceLog(LOG_INFO,"Hash[%d]: %s", i, tmp);
			free(tmp);
		}
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

	free(oldarr);
	/*
	 * print_phash( ht ) ; 
	 */
}

static int
phash_bucket_index(phash_t * ht, buck_t * bp, size_t * rc)
{
	unsigned int    hv1 = bp->hash % ht->size;
	unsigned int    hv2 = (bp->hash % (prime[ht->pnr / 2] - 1)) + 1;	/* must 
										 * not 
										 * be 
										 * 0 
										 */
	buck_t        **arr = ht->arr, *ap;

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
phash_search(phash_t * ht, atom_t * ap, unsigned int key)
{
	unsigned int    hv1 = key % ht->size;
	unsigned int    hv2 = (key % (prime[ht->pnr / 2] - 1)) + 1;	/* must 
									 * not 
									 * be
									 * 0 */
	buck_t        **arr = ht->arr, *bp;

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

static void
bucket_free(buck_t * bp)
{
	if (bp) {
		DEBUG(SPOCP_DSTORE) {
			char           *tmp;

			tmp = oct2strdup(&bp->val, '\\');
			traceLog(LOG_DEBUG,"removing bucket with value [%s]", tmp);
			free(tmp);
		}
		/*
		 * bp->val.val shouldn't be touched since it's pointing to
		 * something that is freed elsewhere 
		 */
		if (bp->next)
			junc_free(bp->next);

		free(bp);
	}
}

void
phash_free(phash_t * ht)
{
	buck_t        **arr;
	unsigned int    i;

	if (ht) {
		if (ht->arr) {
			arr = ht->arr;
			for (i = 0; i < ht->size; i++)
				if (arr[i] != 0)
					bucket_free(arr[i]);

			free(ht->arr);
		}
		free(ht);
	}
}

void
bucket_rm(phash_t * ht, buck_t * bp)
{
	buck_t        **arr = ht->arr;
	buck_t        **newarr;
	size_t          n, size = ht->size;
	size_t          rc;
	int             i;

	i = phash_bucket_index(ht, bp, &rc);

	if (i == 0)
		return;

	free(bp);
	ht->arr[rc] = 0;

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

	free(arr);
}

int
phash_index(phash_t * ht)
{
	return ht->n;
}

varr_t         *
get_all_atom_followers(branch_t * bp, varr_t * in)
{
	phash_t        *ht = bp->val.atom;
	buck_t        **arr = ht->arr;
	unsigned int    i;

	for (i = 0; i < ht->size; i++)
		if (arr[i])
			in = varr_junc_add(in, arr[i]->next);

	return in;
}

varr_t         *
prefix2atoms_match(char *prefix, phash_t * ht, varr_t * pa)
{
	buck_t        **arr = ht->arr;
	unsigned int    i, len = strlen(prefix);

	for (i = 0; i < ht->size; i++)
		if (arr[i] && strncmp(arr[i]->val.val, prefix, len) == 0)
			pa = varr_junc_add(pa, arr[i]->next);

	return pa;
}

varr_t         *
suffix2atoms_match(char *suffix, phash_t * ht, varr_t * pa)
{
	buck_t        **arr = ht->arr;
	unsigned int    i, len = strlen(suffix), l;
	char           *s;

	for (i = 0; i < ht->size; i++) {
		if (arr[i]) {
			s = arr[i]->val.val;

			l = strlen(s);
			if (l >= len) {
				if (strncmp(&s[l - len], suffix, len) == 0)
					pa = varr_junc_add(pa, arr[i]->next);
			}
		}
	}

	return pa;
}

varr_t         *
range2atoms_match(range_t * rp, phash_t * ht, varr_t * pa)
{
	buck_t        **arr = ht->arr;
	int             r, i, dtype = rp->lower.type & 0x07;
	int             ll = rp->lower.type & 0xF0;
	int             ul = rp->upper.type & 0xF0;
	octet_t        *op, *lo = 0, *uo = 0;
	long            li, lv = 0, uv = 0;

	struct in_addr *v4l, *v4u, ia;
#ifdef USE_IPV6
	struct in6_addr *v6l, *v6u, i6a;
#endif

	switch (dtype) {
	case SPOC_NUMERIC:
	case SPOC_TIME:
		lv = rp->lower.v.num;
		uv = rp->upper.v.num;
		break;

	case SPOC_ALPHA:
	case SPOC_DATE:
		lo = &rp->lower.v.val;
		uo = &rp->upper.v.val;
		break;

	case SPOC_IPV4:
		v4l = &rp->lower.v.v4;
		v4u = &rp->upper.v.v4;
		break;

#ifdef USE_IPV6
	case SPOC_IPV6:
		v6l = &rp->lower.v.v6;
		v6u = &rp->upper.v.v6;
		break;
#endif
	}

	for (i = 0; i < (int) ht->size; i++) {
		if (arr[i]) {
			op = &arr[i]->val;
			r = 0;

			switch (dtype) {
			case SPOC_NUMERIC:
			case SPOC_TIME:
				if (is_numeric(op, &li) == SPOCP_SUCCESS) {
					/*
					 * no upper or lower limits 
					 */
					if (ll == 0 && ul == 0)
						r = 1;
					/*
					 * no upper limits 
					 */
					else if (ul == 0) {
						if ((ll == GLE && li >= lv)
						    || (ll = GT && li > lv))
							r = 1;
					}
					/*
					 * no upper limits 
					 */
					else if (ll == 0) {
						if ((ul == GLE && li <= uv)
						    || (ul = LT && li < uv))
							r = 1;
					}
					/*
					 * upper and lower limits 
					 */
					else {
						if (((ll == GLE && li >= lv)
						     || (ll = GT && li > lv))
						    && ((ul == GLE && li <= uv)
							|| (ul = LT
							    && li < uv)))
							r = 1;
					}
				}
				break;

			case SPOC_DATE:
				if (is_date(op) != SPOCP_SUCCESS)
					break;
				/*
				 * otherwise fall through 
				 */
			case SPOC_ALPHA:
				/*
				 * no upper or lower limits 
				 */
				if (lo == 0 && uo == 0)
					r = 1;
				/*
				 * no upper limits 
				 */
				else if (uo == 0) {
					if ((ll == GLE && octcmp(op, lo) >= 0)
					    || (ll = GT && octcmp(op, lo) > 0))
						r = 1;
				}
				/*
				 * no lower limits 
				 */
				else if (lo == 0) {
					if ((ul == GLE && octcmp(op, uo) <= 0)
					    || (ul = LT && octcmp(op, uo) < 0))
						r = 1;
				}
				/*
				 * upper and lower limits 
				 */
				else {
					if (((ll == GLE && octcmp(op, lo) >= 0)
					     || (ll = GT
						 && octcmp(op, lo) > 0))
					    &&
					    ((ul == GLE && octcmp(op, uo) <= 0)
					     || (ul = LT
						 && octcmp(op, uo) < 0)))
						r = 1;
				}
				break;

			case SPOC_IPV4:
				if (is_ipv4(op, &ia) != SPOCP_SUCCESS)
					break;
				/*
				 * MISSING CODE 
				 */
				break;

#ifdef USE_IPV6
			case SPOC_IPV6:
				if (is_ipv6(op, &i6a) != SPOCP_SUCCESS)
					break;
				/*
				 * MISSING CODE 
				 */
				break;
#endif
			}

			if (r)
				pa = varr_junc_add(pa, arr[i]->next);
		}
	}

	return pa;
}
