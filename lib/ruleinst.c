/*
 *  ruleinst.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/7/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#include <config.h>

#include <string.h>

#include <ruleinst.h>
#include <octet.h>
#include <sha1.h>
#include <rdb.h>
#include <varr.h>
#include <macros.h>
#include <wrappers.h>
#include <log.h>

#ifdef HAVE_LIBCRYPTO
#include <openssl/sha.h>
#endif

unsigned int
ruleinst_uid(unsigned char *sha1sum, octet_t * rule, octet_t * blob, 
             char *bcname) 
{
#ifdef HAVE_LIBCRYPTO
    SHA_CTX             ctx;
#else
	struct sha1_context	ctx;
#endif
    
	unsigned int		l=20;
    
#ifdef HAVE_LIBCRYPTO
    SHA1_Init(&ctx);
    SHA1_Update(&ctx,(uint8 *) rule->val, rule->len);
    
	if (bcname)
		SHA1_Update(&ctx, (uint8 *) bcname, strlen(bcname));
	if (blob)
		SHA1_Update(&ctx, (uint8 *) blob->val, blob->len);
	SHA1_Final(sha1sum, &ctx);
#else
	sha1_starts(&ctx);
    
	sha1_update(&ctx, (uint8 *) rule->val, rule->len);
	if (bcname)
		sha1_update(&ctx, (uint8 *) bcname, strlen(bcname));
	if (blob)
		sha1_update(&ctx, (uint8 *) blob->val, blob->len);
    
	sha1_finish(&ctx, (unsigned char *) sha1sum);
#endif
    
	return l;
}

ruleinst_t *
ruleinst_new(octet_t * rule, octet_t * blob, char *bcname)
{
	ruleinst_t     *rip;
	unsigned char   sha1sum[21];
	unsigned char	*ucp;
	int				j;
	unsigned int	l;
    
	if (rule == 0 || rule->len == 0)
		return 0;
    
	rip = (ruleinst_t *) Calloc(1, sizeof(ruleinst_t));
    
    /* make a copy of the octet struct */
	rip->rule = octdup(rule);
    
	if (blob && blob->len)
		rip->blob = octdup(blob);
	else
		rip->blob = 0;
    
	memset(sha1sum,0,20);
	
	l = ruleinst_uid( sha1sum, rule, blob, bcname );
    
	for (j = 0, ucp = (unsigned char *) rip->uid; j < 20; j++, ucp += 2)
		sprintf((char *) ucp, "%02x", sha1sum[j]);
    
	DEBUG(SPOCP_DSTORE) traceLog(LOG_DEBUG,"New rule (%s)", rip->uid);
    
    /* unnecessary
	rip->ep = 0;
	rip->alias = 0;
     */
    
	return rip;
}

ruleinst_t     *
ruleinst_dup(ruleinst_t * ri)
{
	ruleinst_t     *new;
    
	if (ri == 0)
		return 0;
    
	if (ri->bcond)
		new = ruleinst_new(ri->rule, ri->blob, ri->bcond->name);
	else
		new = ruleinst_new(ri->rule, ri->blob, 0);
    
	new = (ruleinst_t *) Calloc(1, sizeof(ruleinst_t));
    
	/* Flawfinder: ignore */
	strcpy(new->uid, ri->uid);
    
	new->rule = octdup(ri->rule);
	/*
	 * if( ri->ep ) new->ep = element_dup( ri->ep ) ; 
	 */
	if (ri->alias)
		new->alias = ll_dup(ri->alias);
    
	return new;
}

void
ruleinst_free(ruleinst_t * rt)
{
	if (rt) {
		if (rt->rule)
			oct_free(rt->rule);
		if (rt->bcexp)
			oct_free(rt->bcexp);
		if (rt->blob)
			oct_free(rt->blob);
		if (rt->alias)
			ll_free(rt->alias);
		/*
		 * if( rt->ep ) element_rm( rt->ep ) ; 
		 */
        
		Free(rt);
	}
}

/*
 *! \brief
 * 	creates an output string that looks like this
 * 
 * path ruleid rule [ boundarycond [ blob ]]
 *
 * If no boundary condition is defined but a blob is, then
 * the boundary condition is represented with "NULL"
 * \param r Pointer to the ruleinstance
 * \param rs The name of the ruleset
 * \returns Pointer to a newly allocated octet containing the output string
 */
octet_t	*ruleinst_print(ruleinst_t * r, char *rs)
{
	octet_t	*oct;
	int	     l, lr, bc=0;
	char	    flen[1024];
    
    
	/* First the path */
	if (rs && strlen(rs) ) {
		lr = strlen(rs);
		snprintf(flen, 1024, "%d:%s", lr, rs);
	} else {
		strcpy(flen, "1:/");
		lr = 1;
	}
    
	/* calculate the total length of the whole string */
	/* starting point path, ruleid and rule only */
	l = r->rule->len + DIGITS(r->rule->len) + lr + DIGITS(lr) + 5 + 40 + 1;
    
	if (r->bcexp && r->bcexp->len) {
		l += r->bcexp->len + 1 + DIGITS(r->bcexp->len);
		bc = 2;
	}
    
	if (r->blob && r->blob->len) {
		l += r->blob->len + 1 + DIGITS(r->blob->len);
		if (bc == 0) {
			l += 6;
			bc = 1;
		}
	}
    
	oct = oct_new(l, 0);
    
	octcat(oct, flen, strlen(flen));
    
	octcat(oct, "40:", 3);
    
	octcat(oct, r->uid, 40);
    
	snprintf(flen, 1024, "%d:", (int) r->rule->len);
	octcat(oct, flen, strlen(flen));
    
	octcat(oct, r->rule->val, r->rule->len);
    
	if( bc == 2) {
		snprintf(flen, 1024, "%d:", (int) r->bcexp->len);
		octcat(oct, flen, strlen(flen));
		octcat(oct, r->bcexp->val, r->bcexp->len);
	}
	else if (bc==1)
		octcat( oct, "4:NULL", 6 );
    
	if (r->blob && r->blob->len) {
		snprintf(flen, 1024, "%d:", (int) r->blob->len);
		octcat(oct, flen, strlen(flen));
		octcat(oct, r->blob->val, r->blob->len);
	}
    
	return oct;
}

varr_t	 *
varr_ruleinst_add(varr_t * va, ruleinst_t * ri)
{
	return varr_add(va, (void *) ri);
}

/*
ruleinst_t     *
varr_ruleinst_pop(varr_t * va)
{
	return (ruleinst_t *) varr_pop(va);
}
*/

ruleinst_t     *
varr_ruleinst_nth(varr_t * va, int n)
{
	return (ruleinst_t *) varr_nth(va, n);
}
