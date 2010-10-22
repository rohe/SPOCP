#include "config.h"
#include "db0.h"
#include <log.h>

void junc_print_r(int lev, junc_t * jp);

/* {{{ void atom_print_r( int lev, junc_t *jp)
   */
static void
atom_print_r(int lev, phash_t *ht)
{
	int     i;
	buck_t  **ba;
	char	indent[32];

	for ( i = 0 ; i < lev ; i++)
		indent[i] = ' ';
	indent[i] = '\0' ;
	
	ba = ht->arr;
	for (i = 0; i < (int) ht->size; i++) {
		if (ba[i]) {
			oct_print(LOG_DEBUG,indent, &ba[i]->val);
			junc_print_r( lev+1, ba[i]->next );
		}
	}
}
/* }}}
   */

/* {{{ void junc_print_r( int lev, junc_t *jp)
   */
void
junc_print_r(int lev, junc_t * jp)
{
	char		indent[32];
	int			i;
	branch_t	*b;
    ruleinst_t  *ri;

	for ( i = 0 ; i < lev ; i++)
		indent[i] = ' ';
	indent[i] = '\0' ;

	if (jp->branch[SPOC_SET]) {
		traceLog(LOG_DEBUG,"%sSET", indent);
	}
	if (jp->branch[SPOC_ATOM]) {
		traceLog(LOG_DEBUG,"%sATOM", indent);
		atom_print_r( lev+1, jp->branch[SPOC_ATOM]->val.atom);
	}
	if (jp->branch[SPOC_LIST]) {
		traceLog(LOG_DEBUG,"%sLIST", indent);
		junc_print_r( lev+1, jp->branch[SPOC_LIST]->val.list );
	}
	if (jp->branch[SPOC_PREFIX]) {
		traceLog(LOG_DEBUG,"%sPREFIX", indent);
	}
	if (jp->branch[SPOC_SUFFIX]) {
		traceLog(LOG_DEBUG,"%sSUFFIX", indent);
	}
	if (jp->branch[SPOC_RANGE]) {
		traceLog(LOG_DEBUG,"%sRANGE", indent);
	}
	if (jp->branch[SPOC_ENDOFLIST]) {
		traceLog(LOG_DEBUG,"%sENDOFLIST", indent);
		b = jp->branch[SPOC_ENDOFLIST];
		if (b->val.next)
			junc_print_r( lev+1, b->val.next );
	}
	if (jp->branch[SPOC_ENDOFRULE]) {
		traceLog(LOG_DEBUG,"%sENDOFRULE", indent);
		ri = jp->branch[7]->val.ri;
        traceLog(LOG_DEBUG,"%s Rule: %d", indent, ri->uid);
	}
	if (jp->branch[SPOC_ANY])
		traceLog(LOG_DEBUG,"%sANY", indent);

}
/* }}}
   */


