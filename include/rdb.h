#ifndef __RDB_H_
#define __RDB_H_

/* layer of indirection */

#include <struct.h>
#include <basefn.h>
#include <varr.h>
#include <db0.h>

/*
cmpfn   *cf;		* compare function *
ffunc   *ff;		* free function *
kfunc   *kf;		* the function that calculates the key from the value *
pfunc   *pf;		* print function for the value *
dfunc   *df;		* duplicate function *
*/


void	rdb_print( void *r );
void	*rdb_new(cmpfn * cf, ffunc * ff, kfunc * kf, dfunc * df, pfunc * pf);
void	rdb_free( void *r );
int		rdb_rules( void *r );
int		rdb_insert( void *r, item_t val);
int		rdb_remove( void *r, Key k);
void	*rdb_search( void *r, Key k);
void	*rdb_dup( void *r, int dyn );
struct _varr *rdb_all( void *r );

#endif
