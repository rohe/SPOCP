#include "config.h"
#include "db0.h"

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

	for ( i = 0 ; i < lev ; i++)
		indent[i] = ' ';
	indent[i] = '\0' ;

	if (jp->item[SPOC_SET]) {
		traceLog(LOG_DEBUG,"%sSET", indent);
	}
	if (jp->item[SPOC_ATOM]) {
		traceLog(LOG_DEBUG,"%sATOM", indent);
		atom_print_r( lev+1, jp->item[SPOC_ATOM]->val.atom);
	}
	if (jp->item[SPOC_LIST]) {
		traceLog(LOG_DEBUG,"%sLIST", indent);
		junc_print_r( lev+1, jp->item[SPOC_LIST]->val.list );
	}
	if (jp->item[SPOC_PREFIX]) {
		traceLog(LOG_DEBUG,"%sPREFIX", indent);
	}
	if (jp->item[SPOC_SUFFIX]) {
		traceLog(LOG_DEBUG,"%sSUFFIX", indent);
	}
	if (jp->item[SPOC_RANGE]) {
		traceLog(LOG_DEBUG,"%sRANGE", indent);
	}
	if (jp->item[SPOC_ENDOFLIST]) {
		traceLog(LOG_DEBUG,"%sENDOFLIST", indent);
		b = jp->item[SPOC_ENDOFLIST];
		if (b->val.next)
			junc_print_r( lev+1, b->val.next );
	}
	if (jp->item[SPOC_ENDOFRULE]) {
		spocp_index_t *id;
		int i;

		traceLog(LOG_DEBUG,"%sENDOFRULE", indent);
		id = jp->item[7]->val.id;
		for (i = 0; i < id->n; i++) 
			traceLog(LOG_DEBUG,"%s Rule: %s", indent,id->arr[i]->uid);
	}
	if (jp->item[SPOC_ANY])
		traceLog(LOG_DEBUG,"%sANY", indent);

}
/* }}}
   */


