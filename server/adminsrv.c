#include <stdio.h>

#include <spocp.h>
#include <struct.h>
#include <locl.h>

#define  SPOCP_NEW(X,y) ((X) = Calloc(y, sizeof(*(X))))

typedef struct _rule {
	octet_t		*path;
	octet_t		*rule;
	octet_t		*blob;
	char		*strrule;
	element_t	*ep;
	bcdef_t		*bcond;
	struct _rule	*next;
	struct _rule	*prev;
} rule_t ;

srv_t	srv;
dbcmd_t	*dbc = 0;

int 	debug = 0;
int	extended = 0;

int elementcmp( element_t *a, element_t *b, octet_t *str );

/* ---------------------------------------------------------------------- */

static rule_t
*rule_new( octet_t *path )
{
	rule_t *r;

	r = ( rule_t * ) Calloc ( 1, sizeof( rule_t ));
	if( path ) r->path = octdup( path );

	return r;
}

static void
rule_free( rule_t *r )
{
	if (r) {
		if (r->path) oct_free( r->path );
		if (r->rule) oct_free( r->rule );
		if (r->blob) oct_free( r->blob );
		if (r->ep) element_free( r->ep);
		Free(r);
	}
}

/* ---------------------------------------------------------------------- */

static int
as_read_rules( char *file, rule_t **rule)
{
	FILE           *fp;
	char           *sp, *tmp;
	int             n = 0, f = 0;
	octet_t         oct, *op;
	spocp_result_t  rc = SPOCP_SUCCESS;
	spocp_charbuf_t	buf;
	spocp_chunk_t	*chunk = 0, *ck;
	spocp_ruledef_t	rdef;
	rule_t		*top = 0, *nr;
	element_t	*ep;
	bcdef_t		*bcd;
				

	if ((fp = fopen(file, "r")) == 0) {
		LOG(SPOCP_EMERG) traceLog(LOG_EMERG,"couldn't open rule file \"%s\"",
					  file);
		memset( &oct, 0, sizeof( octet_t ));
		sp = getcwd(oct.val, oct.size);
		LOG(SPOCP_EMERG) traceLog(LOG_INFO,"I'm in \"%s\"", sp);
		Free(oct.val);
		return -1;
	}

	/*
	 * The default ruleset should already be set 
	 */

	buf.fp = fp;
	buf.str = (char *) Calloc ( BUFSIZ, sizeof( char ));
	buf.size = BUFSIZ;
	buf.start = buf.str ; 

	if (get_more(&buf) == 0) return 0;

	/*
	 * have to escape CR since fgets stops reading when it hits a newline
	 * NUL also has to be escaped since I have problem otherwise finding
	 * the length of the 'string'. '\' hex hex is probably going to be the 
	 * choice 
	 */
	while (rc == SPOCP_SUCCESS && ( chunk = get_object( &buf, 0 )) != 0 ) {
		if (oct2strcmp(chunk->val, ";include ") == 0) {	/* include
								 * file */
			ck = chunk->next;
			tmp = oct2strdup( ck->val, 0 ) ;
			LOG(SPOCP_DEBUG)
				traceLog(LOG_DEBUG,"include directive \"%s\"",
				    tmp);
			if ((rc = as_read_rules(tmp, rule)) < 0) {
				traceLog(LOG_ERR,"Include problem");
			}
		}
		else if (*chunk->val->val == '/' || *chunk->val->val == '(') {


			if (*chunk->val->val == '/') {
				nr = rule_new( chunk->val ) ;
				ck = chunk->next;
			}
			else  {
				nr = rule_new( NULL );
				ck = chunk;
			}

			ruledef_return( &rdef, ck ) ;
			if( rdef.rule ) {
				octet_t loc;

				op = chunk2sexp( rdef.rule ) ;
				octln( &loc, op);
				rc = element_get(&loc, &ep);

				if (rc != SPOCP_SUCCESS) {
					chunk_free( chunk );
					rule_free( nr );
					continue;
				}

				nr->rule = op;
				nr->strrule = oct2strdup( op, '%' );
				nr->ep = ep;
			}
			
			if( rdef.bcond) {
				op = chunk2sexp( rdef.bcond ) ;
				bcd = bcdef_get(srv.root->db, srv.plugin, dbc, op, &rc);
				if (bcd == NULL && rc != SPOCP_SUCCESS) {
					chunk_free( chunk );
					rule_free( nr );
					continue;
				}
				nr->bcond = bcd;
			}
			
			if( rdef.blob )
				nr->blob = octdup(rdef.blob->val) ;
			
			nr->next = top;
			if (top)
				top->prev = nr;

			top = nr;
			n++;
		}
       		else {
			octet_t *key, *val, *to = 0;

			key = chunk->val ;
			if( *chunk->next->next->val->val == '(' ) {
				ruledef_return( &rdef, chunk->next->next ) ;
				if( rdef.rule) {
					val = to = chunk2sexp( rdef.rule ) ;
				}
				else {
					printf("Error in bconddef\n");
					f++;
					chunk_free( chunk ) ;
					continue ;
				}
			}
			else 
				val = chunk->next->next->val ;

			if (bcdef_add(srv.root->db, srv.plugin, dbc, key, val) == 0 )
				rc = SPOCP_LOCAL_ERROR;

			if (to) oct_free( to );
		}

		chunk_free( chunk ) ;
	}

	if (rc != SPOCP_SUCCESS)
		return -1;
	else {
		LOG(SPOCP_INFO) traceLog(LOG_INFO,"Stored %d rules, failed %d", n, f);
		*rule = top;
		return n;
	}
}

/* ---------------------------------------------------------------------- */

int main( int argc, char **argv )
{
	element_t	*ep = 0;
	int		n;
	rule_t		*top = 0, *r;
	spocp_charbuf_t scb;
	spocp_chunk_t	*sc = 0;
	octet_t		*op,loc, *ext = 0;
	spocp_result_t	rc, sr = SPOCP_SUCCESS;
	char 		*cnfg = 0, str[1024], answ[32];
	int		i, runbc = 0, m, c = 1;
	octarr_t	*info = 0;

	memset( &srv, 0, sizeof( srv_t ));
	memset( &answ, 0, sizeof(answ));

	while ((i = getopt(argc, argv, "hbxf:d:")) != EOF) {
		switch (i) {

		case 'f':
			cnfg = Strdup(optarg);
			break;

		case 'd':
			spocp_debug = debug = atoi(optarg);
			if (debug < 0)
				debug = 0;
			break;

		case 'b':
			runbc = 1;
			break;

		case 'x':
			extended = 1;
			break;

		case 'h':
		default:
			fprintf(stderr,
				"Usage: %s [-f configfile] [-b] [-x] [-d debuglevel]\n",
				argv[0]);
			exit(0);
		}
	}

	if (srv.root == 0) 
/*
		srv.root = (ruleset_t *)Calloc(1, sizeof(ruleset_t));
*/
		SPOCP_NEW( srv.root, 1);

	if (cnfg == 0 || read_config(cnfg, &srv) == 0) {
		printf( "Error reading rules from %s\n", cnfg);
		exit(1) ;
	}

	if (srv.rulefile == 0 || (n = as_read_rules( srv.rulefile, &top)) < 0 ) {
		printf( "Error reading rules from %s\n", srv.rulefile);
		exit(1) ;
	}

	if (debug)
		spocp_open_log(0, debug);

	if (n == 0) {
		printf( "Didn't find any rules to use\n");
		exit(0);
	}

	printf( "Read %d rules\n", n );
	
	if (optind == argc) {
		scb.fp = stdin;
		scb.str = (char *) Calloc ( BUFSIZ, sizeof( char ));
		scb.size = BUFSIZ;
		scb.start = scb.str ; 

		if (get_more(&scb) == 0) return 0;
	}
	else {
		scb.str = argv[optind];
		scb.start = scb.str;
		scb.size = 0;
		scb.fp = 0;
	}

	if (extended) 
		ext = oct_new( 1024, 0);

	while ((sc = get_object( &scb, 0 )) != NULL) {
		op = chunk2sexp( sc ) ;
		if( oct2strcpy( op, str, 1024, '%') > 0 ) {
			printf("%s", str);

			printf("\n");
			if (extended)
				printf("\n");
		}

		octln( &loc, op);
		if(( rc = element_get(&loc, &ep)) == SPOCP_SUCCESS) {
			for ( r = top, m = 0, c = 0 ; r ; r = r->next, c++ ) { 
				if (extended) {
					*ext->val = '\0';
					ext->len = 0;
				}
	
				if ( elementcmp( ep, r->ep, ext ) == 0 ) {
					if (runbc && r->bcond ) {
						sr = bcexp_eval(ep, r->ep,
						    r->bcond->exp, &info);
						if (sr==SPOCP_SUCCESS)
							sprintf(answ,
							    ", bcond: OK");
						else 
							sprintf(answ,
							    ", bcond: DENIED (%d)",
							    (int) sr);
					}
					printf( "(%d) Matched rule: %s%s\n",
					    c, r->strrule, answ);
					m++;
				}
				else if( extended ) {
					if (oct2strcpy( ext, str, 1024, '%' ) >0)
						printf("(%d)     [%s] of [%s]\n",
						   c, str, r->strrule);
				}
			}

			if (!m)
				printf( " NO\n" );

			element_free( ep );
		}
		oct_free( op );
		chunk_free( sc );
	}


	exit(0);
}


