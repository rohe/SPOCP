#include <stdlib.h>
#include <string.h>
#include <db0.h>
#include <wrappers.h>
#include <bsdrb.h>

struct rule {
	ruleinst_t			*ri;
	RB_ENTRY( rule )	rule_tree;
} ;


RB_HEAD( rule_head, rule ) ;

static int rulecmp( struct rule *a, struct rule *b)
{
	return strcmp( a->ri->uid, b->ri->uid);
}

RB_PROTOTYPE( rule_head, rule, rule_tree, rulecmp)
RB_GENERATE( rule_head, rule, rule_tree, rulecmp)

/*
RB_INITIALIZE( &rules );
*/


static struct rule *
new_rule( void )
{
	struct rule *r ;
	r = ( struct rule *) Calloc (1, sizeof( struct rule ));
	return r;
}

void *
bsdrb_init()
{
	struct rule_head *rule_root;

	rule_root = (struct rule_head *) Calloc(1, sizeof( struct rule_head ));

	RB_INIT( rule_root ); 

	return (void *) rule_root;
}

ruleinst_t *
bsdrb_find( void *vp, char *k) 
{
	struct rule_head	*rh = (struct rule_head *) vp ;
	ruleinst_t			ri;
	struct rule			key, *r;

	strcpy(ri.uid,k);
	key.ri = &ri;

	r = RB_FIND( rule_head, rh, &key );

	return r ? r->ri : NULL;
}

int
bsdrb_insert( void *vp, ruleinst_t *ri )
{
	struct rule_head	*rh = (struct rule_head *) vp ;
	struct rule			*ir, *r;

	ir = new_rule();
	ir->ri = ri;

	traceLog( SPOCP_DEBUG, "bsdrb inserting key %s to head %p", ri->uid, vp );

	r = RB_INSERT( rule_head, rh, ir );

	return r ? 0 : 1;
}

int
bsdrb_remove( void *vp, char *k )
{
	struct rule_head	*rh = (struct rule_head *) vp ;
	ruleinst_t		ri;
	struct rule		key, *r;

	traceLog( SPOCP_DEBUG, "bsdrb removing key %s from head %p", k, vp );

	strcpy(ri.uid,k);
	key.ri = &ri;

	r = RB_FIND( rule_head, rh, &key );
	if (r)
		r = RB_REMOVE( rule_head, rh, r );

	if (r) {
		traceLog( SPOCP_DEBUG, "bsdrb removal was successfull => %p", r );
		/*free( r );*/
		return 1;
	}
	else {
		traceLog( SPOCP_DEBUG, "bsdrb removal was unsuccessfull" );
		return 0;
	}
}

int bsdrb_empty( void *vp )
{
	struct rule_head	*rh = (struct rule_head *) vp ;

	return RB_EMPTY( rh ) ? 1 : 0 ;
}

varr_t *
bsdrb_all( void *vp )
{
	struct rule_head	*rh = (struct rule_head *) vp ;
	struct rule			*np;
	varr_t				*va = 0;

    for (np = RB_MIN(rule_head, rh ); np != NULL; np = RB_NEXT(rule_head, &rule_root, np)) {
		va = varr_add( va, np->ri );
	}

	/*
	if( va )
		traceLog( SPOCP_DEBUG, "Found %d rules", va->n );
	else
		traceLog( SPOCP_DEBUG, "No rules remaining" );
		*/

	return va;
}

void 
bsdrb_free( void *vp )
{
	struct rule_head	*rh = (struct rule_head *) vp ;
	struct rule			*var, *nxt ;

	for (var = RB_MIN(rule_head, rh ); var != NULL; var = nxt) {
		nxt = RB_NEXT(rule_head, rh, var);
		RB_REMOVE(rule_head, rh, var);
		free(var);
	}
}

