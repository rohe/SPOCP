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

#include "spocpcli.h"

int tls;

char *subject = 0;
char *certificate = 0;
char *privatekey = 0;
char *calist = 0;
char *passwd = 0;

/*--------------------------------------------------------------------------------*

  format of the file
  line = [path 1*SP] s-expr [SP "=>" SP bcondexp]

  path         = '/' [ 1*pathpart ]
  pathpart     = 1*dirchar '/'
  dirchar      = %x30-39 / %x41-5A / %x61-7A / '-' / '_'

  s-expr       = "(" bytestring *s-part ")"
 
  bcondexp     = s-expr

 *--------------------------------------------------------------------------------*/

static int
spocp_add(SPOCP * spocp, char *file, char *subject)
{
	char resp[BUFSIZ], line[BUFSIZ], *cp, *path = 0, *rule, *bcexp;
	FILE		*fp;
	spocp_result_t	rc;
	queres_t	qres;

	memset(resp, 0, BUFSIZ);
	memset(&qres, 0, sizeof(queres_t));

	if ((spocp = spocpc_open(spocp, 0, 3)) == 0) {
		return 0;
	}

	if (file == 0)
		fp = stdin;
	else if ((fp = fopen(file, "r")) == NULL) {
		traceLog("Couldn't open %s", file);
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

	while (fgets(line, BUFSIZ, fp)) {

		if (*line == '#')
			continue;

		cp = &line[strlen(line) - 1];
		while (*cp == '\r' || *cp == '\n' || *cp == ' ')
			*cp-- = '\0';

		if (*line == 0)
			continue;	/* empty line */

		if (*line != '(') {
			path = line;
			rule = index(path, ' ');
			while( *rule == ' ' || *rule == '\t' ) *rule++ = '\0';
		} else {
			rule = line;
			path = 0;
		}

		if(( bcexp = strstr( rule, " => ")) != 0 ) {
			*bcexp = '\0' ;
			bcexp += 4 ;
		}
		
		memset(&qres, 0, sizeof(queres_t));

		if ((rc =
			spocpc_str_send_add(spocp, path, line, bcexp, 0,
			    &qres)) != SPOCPC_OK)
			printf("COMMUNICATION PROBLEM [%d]\n", rc);

		if (qres.rescode != SPOCP_SUCCESS)
			printf("SPOCP PROBLEM [%d]\n", qres.rescode);
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
