#include <locl.h>

/* keeps operations within the same transaction together */

opstack_t *
opstack_new( int op, ruleset_t *rs, octarr_t *arg)
{
	opstack_t *os;
	
	os = (opstack_t *)Calloc(1, sizeof( opstack_t));

	os->oper = op;
	os->rs = rs;
	os->oparg = octarr_dup(arg);
	os->rollback = 0;
	return os;
}

void
opstack_free( opstack_t *osn )
{
	if (osn) {
		if (osn->oparg)
			octarr_free( osn->oparg );
		if (osn->rollback)
			octarr_free( osn->rollback);
		if (osn->next)
			opstack_free( osn->next );

		Free( osn );
	}
}

opstack_t *
opstack_push( opstack_t *a, opstack_t *b)
{
	if (a) {
		while (a->next) a = a->next;

		a->next = b;
		return a;
	}
	else
		return b;
}

/*
spocp_result_t
opstack_commit( opstack_t *head )
{
	
}
*/

