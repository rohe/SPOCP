/***************************************************************************
                         spocpput.c  -  sendind rules to a Spocp server
                             -------------------

    begin                : Wed Oct 1 2003
    copyright            : (C) 2002 by Stockholm University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 **************************************************************************/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

#include <spocp.h>
#include "spocpcli.h"

int tls;

char *subject = 0;
char *certificate = 0;
char *privatekey = 0;
char *calist = 0;
char *passwd = 0;

/*--------------------------------------------------------------------------------*/

typedef struct _ruledef {
	spocp_chunk_t	*rule;
	spocp_chunk_t	*bcond;
	spocp_chunk_t	*blob;
} spocp_ruledef_t;

/*--------------------------------------------------------------------------------*

  format of the file, same as for the server rule file

 *--------------------------------------------------------------------------------*/

static spocp_chunk_t        *
get_object(spocp_charbuf_t * ib, spocp_chunk_t * head)
{
	spocp_chunk_t        *np = 0, *pp;
	char		c;

	if (head == 0)
		head = pp = get_chunk(ib);
	else
		for (pp = head; pp->next; pp = pp->next);

	if (pp == 0)
		return 0;

	/* It's a S-expression */
	if (*pp->val->val == '/' || oct2strcmp(pp->val, "(") == 0) {

		/* starts with a path specification */
		if( *pp->val->val == '/' )
			pp = chunk_add( pp, get_chunk( ib )) ;

		np = get_sexp(ib, pp);

		/*
		 * So do I have the whole S-expression ? 
		 */
		if (np == 0) {
			traceLog(LOG_ERR, "Error in rulefile" ) ;
			chunk_free(head);
			return 0;
		}

		for (; pp->next; pp = pp->next);

		c = charbuf_peek(ib);

		if( c == '=' ) {
			np = get_chunk(ib);
			if (oct2strcmp(np->val, "=>") == 0) { /* boundary condition */
				pp = chunk_add(pp, np);
	
				np = get_chunk(ib);

				if (oct2strcmp(np->val, "(") == 0) 
					pp = get_sexp( ib, chunk_add(pp, np));

				if (pp == 0) {
					traceLog(LOG_ERR, "Error in rulefile" ) ;
					chunk_free(head);
					return 0;
				}

				for (; pp->next; pp = pp->next);

				if( charbuf_peek(ib) == '=' ) 
					np = get_chunk(ib);
			}

			if (oct2strcmp(np->val, "==") == 0) { /* blob */
				pp = chunk_add(pp, np);
				pp = chunk_add( pp, get_chunk( ib ));
			}
		}
	} else if ( oct2strcmp(pp->val, ";include" ) == 0 ) {
		pp = chunk_add( pp, get_chunk( ib )) ;
	} else {	/* should be a boundary condition definition,
			 * has three or more chunks
			 */
		pp = chunk_add( pp, get_chunk( ib ));
		if( pp && oct2strcmp(pp->val, ":=") == 0 ) {
			pp = chunk_add( pp, get_chunk( ib ));
			if( oct2strcmp( pp->val, "(" ) == 0 ) {
				pp = get_sexp( ib, pp);
			}
		}
		else {
			traceLog(LOG_ERR,"Error in rulefile") ;
			chunk_free(head);
			return 0;
		}
	}

	if (pp)
		return head;
	else {
		chunk_free( head );
		return 0;
	}
}

/*--------------------------------------------------------------------------------*/

static void
ruledef_return( spocp_ruledef_t *rd, spocp_chunk_t *c )
{
	spocp_chunk_t *ck = c;
	int	p = 0;

	rd->rule = c ;

	do {
		if( oct2strcmp( ck->val, "(" ) == 0 )
			p++;
		else if( oct2strcmp( ck->val, ")" ) == 0 )
			p--;

		ck = ck->next;
	} while( p ) ;
 
        if( ck && oct2strcmp( ck->val, "=>") == 0 ) {
		ck = ck->next;
		rd->bcond = ck;
		do {
			if( oct2strcmp( ck->val, "(" ) == 0 )
				p++;
			else if( oct2strcmp( ck->val, ")" ) == 0 )
				p--;

			ck = ck->next;
		} while( p );
	}
	else
		rd->bcond = 0;

        if( ck && oct2strcmp( ck->val, "==") == 0 ) {
		rd->blob = ck->next;
	}
	else
		rd->blob = 0;
}

/*--------------------------------------------------------------------------------*/


static int
spocp_add(SPOCP * spocp, char *file, char *subject)
{
	char		resp[BUFSIZ];
	FILE		*fp;
	spocp_result_t	rc = SPOCP_SUCCESS;
	queres_t	qres;
	octet_t		*rule, *bcond, *blob, *path;
	spocp_chunk_t	*chunk, *ck;
	spocp_charbuf_t	*buf;
	spocp_ruledef_t	rdef;

	memset(resp, 0, BUFSIZ);
	memset(&qres, 0, sizeof(queres_t));

	if ((spocp = spocpc_open(spocp, 0, 3)) == 0) {
		return 0;
	}

	if (file == 0)
		fp = stdin;
	else if ((fp = fopen(file, "r")) == NULL) {
		traceLog(LOG_INFO,"Couldn't open %s", file);
		exit(1);
	}
#ifdef HAVE_SSL
	if (spocp->tls) {
		if ((spocpc_attempt_tls(spocp, &qres) == SPOCPC_OK) &&
		    qres.rescode == SPOCP_SSL_START) {
			if (spocpc_start_tls(spocp) == 0)
				exit(1);
		} else
			exit(1);
	}
	memset(&qres, 0, sizeof(queres_t));
#endif

	if (subject &&
	    (spocpc_str_send_subject(spocp, "subject", &qres) != SPOCPC_OK ||
		qres.rescode != SPOCP_SUCCESS))
		exit(1);

	/* lines should be [path] 1*SP rule *SP "=>" *SP bcondexp */

	path = 0;
	bcond = 0;
	blob = 0;
	rule = 0;

	buf = charbuf_new( fp, BUFSIZ );

	if (get_more(buf) == 0) return 0;

	while (rc == SPOCP_SUCCESS && ( chunk = get_object( buf, 0 )) != 0 ) {
	
		if (*chunk->val->val == '/' || *chunk->val->val == '(') {
			if (*chunk->val->val == '/') {
				path = chunk->val;
				ck = chunk->next;
			}
			else 
				ck = chunk;

			ruledef_return( &rdef, ck ) ;
			if( rdef.rule ) 
				rule = chunk2sexp( rdef.rule ) ;
			
			if( rdef.bcond) 
				bcond = chunk2sexp( rdef.bcond ) ;
			
			if( rdef.blob ) {
				if (!rdef.bcond)
					bcond =  str2oct("NULL", 0) ;

				blob = rdef.blob->val ;
			}
			
			memset(&qres, 0, sizeof(queres_t));

			if ((rc =
				spocpc_send_add(spocp, path, rule, bcond, blob,
				    &qres)) != SPOCPC_OK)
				printf("COMMUNICATION PROBLEM [%d]\n", rc);

			if (qres.rescode != SPOCP_SUCCESS)
				printf("SPOCP PROBLEM [%d]\n", qres.rescode);

			if( rule ) {
				oct_free( rule );
				rule = 0;
			}
			if( bcond ) {
				oct_free( bcond );
				bcond = 0;
			}
			path = blob = 0;
		}
	}


	memset(&qres, 0, sizeof(queres_t));

	spocpc_send_logout(spocp);
	spocpc_close(spocp);
	free_spocp(spocp);

	return 0;
}

int
main(int argc, char **argv)
{
	char *file = 0, *subject = 0;
	char *server;
	char i;
	SPOCP *spocp;

	spocpc_debug = 0;
	server = 0;

	while ((i = getopt(argc, argv, "dht:a:c:s:f:p:l:P:")) != EOF) {
		switch (i) {
		case 'c':
			certificate = strdup(optarg);
			break;

		case 'p':
			privatekey = strdup(optarg);
			break;

		case 'P':
			passwd = strdup(optarg);
			break;

		case 'l':
			calist = strdup(optarg);
			break;

		case 'f':
			file = strdup(optarg);
			break;

		case 'd':
			spocpc_debug = 1;
			break;

		case 't':
			tls = atoi(optarg);
			break;

		case 'a':
			subject = strdup(optarg);
			break;

		case 's':
			server = strdup(optarg);
			break;

		case 'h':
		default:
			fprintf(stderr,
			    "Usage: %s -s server -f rulefile [-d] [-a subject] or\n",
			    argv[0]);
			fprintf(stderr, "\t%s -t -s server -f rulefile [-d] ",
			    argv[0]);
			fprintf(stderr,
			    "-c certifcate -l calist -p privatekey [-P passwd]");
			exit(0);
		}
	}

	if (server == 0)
		exit(1);

	spocp = spocpc_init(server, 0, 0);

#ifdef HAVE_SSL
	if (tls) {
		spocpc_use_tls(spocp);
		spocpc_tls_use_cert(spocp, certificate);
		spocpc_tls_use_calist(spocp, calist);
		spocpc_tls_use_key(spocp, privatekey);
		spocpc_tls_use_passwd(spocp, passwd);

		if (tls & DEMAND)
			spocpc_tls_set_demand_server_cert(spocp);
		if (tls & VERIFY)
			spocpc_tls_set_verify_server_cert(spocp);
	}
#else
	{
		fprintf(stderr, "Can't use TLS/SSL, ignoring\n");
		tls = 0;
	}
#endif

	spocp_add(spocp, file, subject);

	return 0;
}
