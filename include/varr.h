#ifndef __JARR_H_
#define __JARR_H_
#include <basefn.h>
#include <db0.h>

typedef struct _varr {
	size_t          n;
	size_t          size;
	void          **arr;
} varr_t;

/*
 * ---------------------------------------------------------------------- 
 */

varr_t         *varr_new(size_t n);
varr_t         *varr_add(varr_t * va, void *v);
void            varr_free(varr_t * va);
int             varr_find(varr_t * va, void *v);
unsigned int    varr_len(varr_t * va);
varr_t         *varr_or(varr_t * a, varr_t * b, int noninv);
varr_t         *varr_and(varr_t * a, varr_t * b);
void           *varr_pop(varr_t * va);
void           *varr_fifo_pop(varr_t * va);
void           *varr_nth(varr_t * va, int n);
void           *varr_first_common(varr_t * va, varr_t * b);
void           *varr_rm(varr_t * va, void *v);
void           *varr_first(varr_t * va);
void           *varr_next(varr_t * va, void *v);
varr_t         *varr_dup(varr_t * va, dfunc * df);

void 		varr_rm_dup( varr_t *va, cmpfn *cf, ffunc *ff);
/*
 * -----
 * 
 * varr_t *varr_junc_add( varr_t *va, junc_t *ju ) ; junc_t *varr_junc_pop(
 * varr_t *va ) ; junc_t *varr_junc_nth( varr_t *va, int n ) ; junc_t
 * *varr_junc_common( varr_t *va, varr_t *b ) ; junc_t *varr_junc_rm( varr_t
 * *va, junc_t *v ) ;
 * 
 * -----
 * 
 * varr_t *varr_ruleinst_add( varr_t *va, ruleinst_t *ju ) ; ruleinst_t
 * *varr_ruleinst_pop( varr_t *va ) ; ruleinst_t *varr_ruleinst_nth( varr_t
 * *va, int n ) ;
 * 
 */

#endif
