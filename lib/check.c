/*! \file lib/check.c \author Roland Hedberg <roland@catalogix.se> 
 */

#include <string.h>

#include <db0.h>
#include <wrappers.h>
#include <func.h>

/* #define AVLUS 1 */

static checked_t *
checked_new( ruleinst_t *ri, spocp_result_t rc, octet_t *blob)
{
	checked_t *cr;

	cr = (checked_t *)Calloc(1, sizeof( checked_t ));
#ifdef AVLUS
	traceLog( LOG_DEBUG, "checked_new %p", (void *) cr );
#endif
	cr->ri = ri;
	cr->rc = rc;
	cr->blob = blob;

	return cr;
}

void
checked_free( checked_t *c )
{
	if (c) {
#ifdef AVLUS
		traceLog( LOG_DEBUG, "checked_free %p", (void *) c );
#endif
		if (c->blob)
			oct_free(c->blob);
		if (c->next)
			checked_free(c->next);
		Free(c);
	}
}

checked_t *
checked_rule( ruleinst_t *ri, checked_t **cr)
{
	checked_t *c;

	for (c = *cr; c ; c = c->next )
		if ( c->ri == ri )
			return c;

	return 0;
}

spocp_result_t
checked_res( ruleinst_t *ri, checked_t **cr)
{
	checked_t *c;

	for (c = *cr; c ; c = c->next )
		if ( c->ri == ri )
			return c->rc;
	return -1;
}

void
add_checked( ruleinst_t *ri, spocp_result_t rc, octet_t *blob, checked_t **cr)
{
	checked_t *c, *nc;

	c = checked_new( ri,rc,blob );

	if( *cr == 0 )
		*cr = c;
	else {
		for( nc = *cr; nc->next ; nc = nc->next );
		nc->next = c ;
	}
}

void
checked_print( checked_t *c )
{
	traceLog(LOG_DEBUG,"--Checked--");
	oct_print(LOG_DEBUG,"rule", c->ri->rule );
        traceLog(LOG_DEBUG,"rc=%d", c->rc );
	if ( c->blob )	
		oct_print(LOG_DEBUG,"blob", c->blob );
}

