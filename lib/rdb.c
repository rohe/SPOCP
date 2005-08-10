/* layer of indirection */

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>

#include <struct.h>
#include <wrappers.h>
#include <db0.h>
#include <func.h>
#include "rdb.h"
#include "bsdrb.h"

void *rdb_new(cmpfn * cf, ffunc * ff, kfunc * kf, dfunc * df, pfunc * pf)
{
	return bsdrb_init();
}

void rdb_free( void *r )
{
	;
}

int rdb_rules( void *r )
{
	if ( r && !bsdrb_empty(r))
		return 1;
	else
		return 0;
}

int rdb_insert( void *r, item_t val)
{
	return bsdrb_insert( r, (ruleinst_t *) val );
}

int rdb_remove( void *r, Key k)
{
	return bsdrb_remove( r, k ) ;
}

void *rdb_search( void *r, Key k)
{
	return (void *) bsdrb_find( r, k);
}

void *rdb_dup( void *r, int dyn )
{ 
	return 0;
}

/* returns a array of pointers to ruleinst_t structs */
varr_t *rdb_all( void *r )
{
	return bsdrb_all(r);
}

void rdb_print( void *r )
{
	int		i;
	varr_t	*va;

	va = rdb_all( r );

	if (va) {
		for( i = 0; i < va->n; i++ )
			traceLog( LOG_DEBUG, "RDB %s", ((ruleinst_t *) va->arr[i])->uid);

		varr_free(va );
	}
	else
			traceLog( LOG_DEBUG, "RDB EMPTY");
}

