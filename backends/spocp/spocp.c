/*
 * spocp backend 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPOCP_BACKEND 1

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <rvapi.h>
#include <spocpcli.h>

/*
 * arg is composed of the following parts spocphost;operation;arg
 * 
 * where arg are [path] sexp [ SP bcond [ SP blob ]]
 *
 * The native Spocp protocol is used 
 */

/*
extern int      spocpc_debug;
*/
befunc          spocp_test;

static int
P_spocp_close(void *vp)
{
	SPOCP          *spocp;

	spocp = (SPOCP *) vp;

	spocpc_send_logout(spocp);
	spocpc_close(spocp);
	free_spocp(spocp);

	return 1;
}

spocp_result_t
spocp_test(cmd_param_t * cpp, octet_t * blob)
{
	spocp_result_t  r = SPOCP_DENIED;

	octarr_t	*argv;
	octet_t		*host, *oper, *oct, *path, *query=0;
	octet_t		*rule=0, *info=0, *bcond=0;
	char		*server, *tmp;
	becon_t		*bc = 0;
	SPOCP		*spocp;
	pdyn_t		*dyn = cpp->pd;
	queres_t	qres ;

	if (cpp->arg == 0)
		return SPOCP_MISSING_ARG;

	tmp = oct2strdup( cpp->arg, 0);
	traceLog(LOG_DEBUG, "Argument \"%s\"", tmp );
	free(tmp);

	if ((oct = element_atom_sub(cpp->arg, cpp->x)) == 0)
		return SPOCP_SYNTAXERROR;

	tmp = oct2strdup( oct, 0);
	traceLog(LOG_DEBUG, "Argument after expension \"%s\"", tmp );
	free(tmp);

	argv = oct_split(oct, ';', 0, 0, 0);

	if (oct != cpp->arg)
		oct_free(oct);

	/* spocpc_debug = 0; */
	host = argv->arr[0];

	memset( &qres, 0, sizeof( queres_t )) ;

	if (dyn == 0 || (bc = becon_get(host, dyn->bcp)) == 0) {
		server = oct2strdup(host, 0);
		traceLog(LOG_INFO,"Spocp query to %s", server);

		spocp = spocpc_open( 0, server, 2);
		free(server);

		if (spocp == 0) {
			traceLog(LOG_NOTICE,"Couldn't open connection");
			r = SPOCP_UNAVAILABLE;
		} else if (dyn && dyn->size) {
			if (!dyn->bcp)
				dyn->bcp = becpool_new(dyn->size);
			bc = becon_push(host, &P_spocp_close, (void *) spocp,
					dyn->bcp);
		}
	} else {
		spocp = (SPOCP *) bc->con;
		/*
		 * how to check that the server is still there ? Except
		 * sending a dummy query ? 
		 */
		if ((r =
		     spocpc_str_send_query(spocp, "", "(5:XxXxX)",
				       &qres)) == SPOCPC_OK) {
			if (qres.rescode == SPOCP_UNAVAILABLE )
				if (spocpc_reopen(spocp, 1) == FALSE)
					r = SPOCP_UNAVAILABLE;
		}
	}

	if (r == SPOCP_DENIED) {
		/*
		 * means there is not actual query, just checking if the SPOCP 
		 * server is up and running 
		 */
		if (argv->n == 1)
			r = SPOCP_SUCCESS;
		else {
			traceLog(LOG_DEBUG,"filedesc: %d", spocp->fd);

			oper = argv->arr[1];
			path = argv->arr[2];

			if (oct2strcmp( oper, "QUERY") == 0 ) {
				query = sexp_normalize_oct(argv->arr[3]);
				if (query)
					r = spocpc_send_query(spocp, path, query,
					       &qres);
				else
					r = SPOCPC_SYNTAX_ERROR;
			}
			else if (oct2strcmp( oper, "ADD") == 0 ) {
				rule = argv->arr[3];
				if (argv->n >= 4 ) 
					bcond = argv->arr[4];
				if (argv->n == 5 ) 
					info = argv->arr[5];

				r = spocpc_send_add(spocp, path, rule, bcond,
				    info, &qres);
			}

			if (r == SPOCPC_OK ) {
				if (qres.rescode == SPOCP_SUCCESS ) {
					r = SPOCP_SUCCESS ;
					if (qres.blob ) {
						/* just pick the first */
						octmove(blob, qres.blob->arr[0]);	
						octarr_free(qres.blob);
					}
				}
			}

			traceLog(LOG_DEBUG,"Spocp returned:%d", (int) r);
		}
	}

	if (bc)
		becon_return(bc);
	else if (spocp)
		P_spocp_close((void *) spocp);

	octarr_free(argv);

	return r;
}

plugin_t        spocp_module = {
	SPOCP20_PLUGIN_STUFF,
	spocp_test,
	NULL,
	NULL,
	NULL
};
