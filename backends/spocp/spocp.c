/*
 * spocp backend 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <rvapi.h>
#include <spocpcli.h>

/*
 * arg is composed of the following parts spocphost:path:rule
 * 
 * The native Spocp protocol is used 
 */

extern int      debug;
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

	octarr_t       *argv;
	octet_t        *oct;
	char           *path, *server, *query;
	becon_t        *bc = 0;
	SPOCP          *spocp;
	pdyn_t         *dyn = cpp->pd;
	queres_t       qres ;

	if (cpp->arg == 0)
		return SPOCP_MISSING_ARG;

	if ((oct = element_atom_sub(cpp->arg, cpp->x)) == 0)
		return SPOCP_SYNTAXERROR;

	argv = oct_split(oct, ';', 0, 0, 0);

	if (oct != cpp->arg)
		oct_free(oct);

	debug = 0;
	oct = argv->arr[0];

	memset( &qres, 0, sizeof( queres_t )) ;

	if (dyn == 0 || (bc = becon_get(oct, dyn->bcp)) == 0) {
		server = oct2strdup(oct, 0);
		traceLog("Spocp query to %s", server);

		spocp = spocpc_open( 0, server, 2);
		free(server);

		if (spocp == 0) {
			traceLog("Couldn't open connection");
			r = SPOCP_UNAVAILABLE;
		} else if (dyn && dyn->size) {
			if (!dyn->bcp)
				dyn->bcp = becpool_new(dyn->size);
			bc = becon_push(oct, &P_spocp_close, (void *) spocp,
					dyn->bcp);
		}
	} else {
		spocp = (SPOCP *) bc->con;
		/*
		 * how to check that the server is still there ? Except
		 * sending a dummy query ? 
		 */
		if ((r =
		     spocpc_send_query(spocp, "", "(5:XxXxX)",
				       &qres)) == SPOCPC_OK) {
			if( qres.rescode == SPOCP_UNAVAILABLE )
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
			traceLog("filedesc: %d", spocp->fd);

			path = oct2strdup(argv->arr[1], 0);
			query = oct2strdup(argv->arr[2], 0);

			if ((r =
			     spocpc_send_query(spocp, path, query,
					       &qres)) == SPOCPC_OK ) {
				if (qres.rescode == SPOCP_SUCCESS ) {
					r = SPOCP_SUCCESS ;
					if( qres.blob ) {
						/* just pick the first */
						octmove(blob, qres.blob->arr[0]);	
						octarr_free(qres.blob);
					}
				}
			}

			traceLog("Spocp returned:%d", (int) r);

			free(path);
			free(query);
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
	NULL
};
