/***************************************************************************
                         pam_spocp.c  -  pam access module 
                             -------------------

    begin                : Wed Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
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

/*================================================================================*/

int tls;
int withid;

char *subject = 0;
char *certificate = 0;
char *privatekey = 0;
char *calist = 0;
char *passwd = 0;

static int
spocp_list(SPOCP * spocp, char *file)
{
	char	resp[BUFSIZ];
	int	res = 0;
	ssize_t	n;
	FILE	*fp;
	int	rc = SPOCPC_OK;
	queres_t	qres;

	memset(resp, 0, BUFSIZ);
	memset(&qres, 0, sizeof(queres_t));

	if ((spocpc_open(spocp, 0, 3)) == 0) {
		return 0;
	}

	if (file == 0)
		fp = stdout;
	else if ((fp = fopen(file, "w")) == NULL) {
		traceLog("Couldn't open %s for writing", file);
		return 0;
	}
#ifdef HAVE_SSL
	if (tls) {
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
	    (spocpc_send_subject(spocp, "subject", &qres) != SPOCPC_OK ||
		qres.rescode != SPOCP_SUCCESS))
		exit(1);

	if ( spocpc_writen(spocp, "6:4:LIST", 8) != 8 ) return -1;

	while (rc == SPOCPC_OK &&
	    (n = spocpc_readn(spocp, resp, BUFSIZ)) > 0) {
		rc = spocpc_parse_and_print_list(resp, n, fp, withid);
	}

	if (fp != stdout)
		fclose(fp);

	spocpc_send_logout(spocp);
	spocpc_close(spocp);
	free_spocp(spocp);

	return res;
}

int
main(int argc, char **argv)
{
	char *file = 0;
	char *server = 0;
	char i;
	SPOCP *spocp = 0;

	spocpc_debug = 0;
	tls = 0;

	while ((i = getopt(argc, argv, "dht:wc:C:P:p:a:f:s:")) != EOF) {
		switch (i) {

		case 'f':
			file = strdup(optarg);
			break;

		case 'd':
			spocpc_debug = 1;
			break;

		case 's':
			server = strdup(optarg);
			break;

		case 'a':
			subject = strdup(optarg);
			break;

		case 'c':
			certificate = 0;
			break;

		case 'p':
			privatekey = 0;
			break;

		case 'C':
			calist = 0;
			break;

		case 'P':
			passwd = 0;
			break;

		case 't':
			tls = atoi(optarg);
			break;

		case 'w':
			withid = 1;
			break;

		case 'h':
		default:
			fprintf(stderr,
			    "Usage: %s [-f rulefile] [-d] [-t tlsoption] [h] [-a subject]\n",
			    argv[0]);
			fprintf(stderr,
			    "\t[-s server] [-p privatekeyfile] [-c certificatefile]\n");
			fprintf(stderr, "\t[-P password] [-C calistFile]\n");
			exit(0);
		}
	}

	if (server == 0)
		exit(1);

	spocp = spocpc_init(server);

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

	if (spocp_list(spocp, file) == 0)
		return 1;

	return 0;
}
