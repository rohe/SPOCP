/*! \file src/clilib.c 
 * \author Roland Hedberg <roland@catalogix.se>
 * \brief The Spocp client library
 */
/***************************************************************************
                         common.c  -  common stuff to all spocp clients
                             -------------------

    begin                : Wed Oct 5 2003
    copyright            : (C) 2003 by Stockholm University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 **************************************************************************/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdarg.h>
#include <syslog.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <ctype.h>
#include <sys/utsname.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>

#include <spocp.h>
#include <wrappers.h>
#include <spocpcli.h>

#if!defined(MAXHOSTNAMELEN) || (MAXHOSTNAMELEN < 64)
#undef MAXHOSTNAMELEN
/*! The max size of a FQDN */
#define MAXHOSTNAMELEN 256
#endif

/*! Should not have to define this here */
char	*strndup(const char *old, size_t sz);

/*! Local shothand alias */
typedef struct sockaddr SA;

/*! 
 * \var prbuf
 * \brief global variable to hold strings to be printed
 */
char prbuf[BUFSIZ];

int spocpc_str_send_query(SPOCP *, char *, char *, queres_t *);
int spocpc_str_send_delete(SPOCP *, char *, char *, queres_t *);
int spocpc_str_send_subject(SPOCP *, char *, queres_t *);
int spocpc_str_send_add(SPOCP *, char *, char *, char *, char *, queres_t *);

/*--------------------------------------------------------------------------------*/

/*!
 * \brief Translation from result codes as written on-the-wire and 
 *   resultcodes used in the program
 */
spocp_rescode_t spocp_rescode[] = {
	{SPOCP_WORKING, "100"},

	{SPOCP_SUCCESS, "200"},
	{SPOCP_MULTI, "201"},
	{SPOCP_DENIED, "202"},
	{SPOCP_CLOSE, "203"},
	{SPOCP_SSL_START, "205"},

	{SPOCP_SASLBINDINPROGRESS, "301"},

	{SPOCP_BUSY, "400"},
	{SPOCP_TIMEOUT, "401"},
	{SPOCP_TIMELIMITEXCEEDED, "402"},
	{SPOCP_REDIRECT, "403"},

	{SPOCP_SYNTAXERROR, "500"},
	{SPOCP_MISSING_ARG, "501"},
	{SPOCP_MISSING_CHAR, "502"},
	{SPOCP_PROTOCOLERROR, "503"},
	{SPOCP_UNKNOWNCOMMAND, "504"},
	{SPOCP_PARAM_ERROR, "505"},
	{SPOCP_SSL_ERR, "506"},
	{SPOCP_UNKNOWN_TYPE, "507"},

	{SPOCP_SIZELIMITEXCEEDED, "511"},
	{SPOCP_OPERATIONSERROR, "512"},
	{SPOCP_UNAVAILABLE, "513"},
	{SPOCP_INFO_UNAVAIL, "514"},
	{SPOCP_NOT_SUPPORTED, "515"},
	{SPOCP_SSLCON_EXIST, "516"},
	{SPOCP_OTHER, "517"},
	{SPOCP_CERT_ERR, "518"},
	{SPOCP_UNWILLING, "519"},
	{SPOCP_EXISTS, "520"},
	{0, 0}
};

/*=================================================================================*/
/*                 SSL/TLS stuff                                                   */
/*=================================================================================*/

#ifdef HAVE_SSL

#include <openssl/lhash.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/x509.h>

typedef struct randstuff
{
	time_t t;
	pid_t p;
} randstuff_t;

/*---------------------------------------------------------------------------*/

static void
print_tls_errors(void)
{
	char errorstr[1024];
	unsigned long e;

	if ((e = ERR_get_error()) != 0) {
		ERR_error_string_n(e, errorstr, 1024);

		fprintf(stderr, "%s\n", errorstr);
	}
}

/*---------------------------------------------------------------------------*/

static int
password_cb(char *buf, int num, int rwflag, void *userdata)
{
	if (num < (int) strlen((char *) userdata) + 1) {
		if (spocpc_debug)
			traceLog("Not big enough place for the password (%d)",
			    num);
		return (0);
	}

	strcpy(buf, userdata);
	return (strlen(userdata));
}

/*---------------------------------------------------------------------------*/
/* hardly necessary ? */

static void
add_entropy(char *file)
{
	randstuff_t r;
	r.t = time(NULL);
	r.p = getpid();

	RAND_seed((unsigned char *) (&r), sizeof(r));
	RAND_load_file(file, 1024 * 1024);
}

/*---------------------------------------------------------------------------*/

static int
verify_cb(int verify_ok, X509_STORE_CTX * x509ctx)
{
	X509_NAME *xn;
	static char txt[256];

	xn = X509_get_subject_name(x509ctx->current_cert);
	X509_NAME_oneline(xn, txt, sizeof(txt));

	if (verify_ok == 0) {
		traceLog("SSL verify error: depth=%d error=%s cert=%s",
		    x509ctx->error_depth,
		    X509_verify_cert_error_string(x509ctx->error), txt);
		return 0;
	}

	/* the check against allowed clients are done elsewhere */

	if (spocpc_debug)
		traceLog("SSL authenticated peer: %s\n", txt);

	return 1;		/* accept */
}

/*---------------------------------------------------------------------------*/
/*!
 * Initializes all the SSL stuff for a SPOCP session, that is creates 
 * the context and loads it with the locations of the certificate, privatekey
 * and caList files. If password is given that will also be loaded through the
 * locally defined password callback function 
 * \param spocp A pointer to the SPOCP session information block
 * \return 1 on sucess, 0 if something went wrong
*/
static int
tls_init(SPOCP * spocp)
{
	/* uses strdup for consistency */
	spocp->sslEntropyFile = strdup( "/var/log/messages" );

	SSL_library_init();
	SSL_load_error_strings();	/* basic set up */

	if (!RAND_status()) {
		if (spocpc_debug)
			traceLog("Have to find more entropy\n");

		/* get some entropy */

		add_entropy(spocp->sslEntropyFile);

		if (!RAND_status()) {
			if (spocpc_debug)
				traceLog
				    ("Failed to find enough entropy on your system");
			return 0;
		}
	}

	/* Create a TLS context */

	if (!(spocp->ctx = SSL_CTX_new(SSLv23_client_method()))) {
		return 0;
	}

	/* Set up certificates and keys */

	if (spocp->certificate &&
	    !SSL_CTX_use_certificate_chain_file(spocp->ctx,
		spocp->certificate)) {
		if (spocpc_debug)
			traceLog("Failed to load certificate chain file");
		SSL_CTX_free(spocp->ctx);
		spocp->ctx = 0;
		return 0;
	}

	if (spocp->passwd) {
		SSL_CTX_set_default_passwd_cb(spocp->ctx, password_cb);
		SSL_CTX_set_default_passwd_cb_userdata(spocp->ctx,
		    (void *) spocp->passwd);
	}

	if (!SSL_CTX_use_PrivateKey_file(spocp->ctx, spocp->privatekey,
		SSL_FILETYPE_PEM)) {
		if (spocpc_debug)
			traceLog("Failed to load private key file, passwd=%s",
			    spocp->passwd);
		SSL_CTX_free(spocp->ctx);
		spocp->ctx = 0;
		return 0;
	}

	if (spocp->calist
	    && !SSL_CTX_load_verify_locations(spocp->ctx, spocp->calist, 0)) {
		traceLog("Failed to load ca file");
		SSL_CTX_free(spocp->ctx);
		spocp->ctx = 0;
		return 0;
	}

	SSL_CTX_set_verify(spocp->ctx, SSL_VERIFY_PEER, verify_cb);

	SSL_CTX_set_verify_depth(spocp->ctx, 5);

	return 1;
}

/*---------------------------------------------------------------------------*/
/* Check that the common name matches the host name */

static int
check_cert_chain(SSL * ssl, char *host)
{
	X509 *peer;
	X509_NAME *xn;
	char hostname[256], buf[256], *name = 0, *pp = 0;
	int r;

	if (SSL_get_verify_result(ssl) != X509_V_OK) {
		traceLog("Certificate doesn't verify");
		return 0;
	}

	/*Check the cert chain. The chain length
	 * is automatically checked by OpenSSL when we
	 * set the verify depth in the ctx */

	/*Check the common name */
	peer = SSL_get_peer_certificate(ssl);
	xn = X509_get_subject_name(peer);

	name = X509_NAME_oneline(xn, buf, sizeof(buf));
	if (spocpc_debug)
		traceLog("Peers subject name: %s", name);

	pp = index(host, ':');
	/* do I need to reset this, no */
	if (pp)
		*pp = 0;

	r = X509_NAME_get_text_by_NID(xn, NID_commonName, hostname, 256);
	if (spocpc_debug)
		if (spocpc_debug)
			traceLog
			    ("Peers hostname: %s, connections hostname: %s",
			    hostname, host);

/*
  if (strcasecmp(peer_CN,host)) {
    traceLog("Common name doesn't match host name");
    return 0 ;
  }
*/

	X509_free(peer);

	return 1;
}

/*---------------------------------------------------------------------------*/

/*!
 * Loads the Spocp session with information pertaining to the use of a SSL/TLS
 * connection.
 * \param spocp A pointer to the SPOCP session
 * \param certificate the certificate file 
 * \param privatekey  the private key file
 * \param calist a file containing the CAs we have choosen to believe in
 * \param passwd the password that can unlock the private key.
 * \return none
 */


void
spocpc_tls_use_cert(SPOCP * spocp, char *str)
{
	if (str)
		spocp->certificate = strdup(str);
	else
		spocp->certificate = 0;
}

void
spocpc_tls_use_calist(SPOCP * spocp, char *str)
{
	if (str)
		spocp->calist = strdup(str);
	else
		spocp->calist = 0;
}

void
spocpc_tls_use_key(SPOCP * spocp, char *str)
{
	if (str)
		spocp->privatekey = strdup(str);
	else
		spocp->privatekey = 0;
}

void
spocpc_tls_use_passwd(SPOCP * spocp, char *str)
{
	if (str)
		spocp->passwd = strdup(str);
	else
		spocp->passwd = 0;
}

void
spocpc_tls_set_demand_server_cert(SPOCP * spocp)
{
	spocp->servercert = TRUE;
}

void
spocpc_tls_set_verify_server_cert(SPOCP * spocp)
{
	spocp->verify_ok = TRUE;
}

void
spocpc_use_tls(SPOCP * spocp)
{
	spocp->tls = TRUE;
}

/*---------------------------------------------------------------------------*/

int
spocpc_start_tls(SPOCP * spocp)
{
	char sslhost[MAXHOSTNAMELEN];
	int r;

	if (tls_init(spocp) == 0)
		return 0;

	if (spocpc_debug)
		traceLog("TLS init done");

	/* Connect the SSL/TLS socket */
	if ((spocp->ssl = SSL_new(spocp->ctx)) == 0)
		return 0;

	if (SSL_set_fd(spocp->ssl, spocp->fd) != 1) {
		traceLog("starttls: Error setting fd\n");
		SSL_free(spocp->ssl);
		return 0;
	}

	SSL_set_mode(spocp->ssl, SSL_MODE_AUTO_RETRY);

	if (spocpc_debug)
		traceLog("SSL_connect");

	/* Initiates the TLS handshake with a server */
	if ((r = SSL_connect(spocp->ssl)) <= 0) {
		traceLog("SSL connect failed %d", r);
		print_tls_errors();
		SSL_free(spocp->ssl);
		return 0;
	}

	if (*(*spocp->srv) == '/' || strcmp(*spocp->srv, "localhost") == 0) {
		gethostname(sslhost, MAXHOSTNAMELEN);
		if (check_cert_chain(spocp->ssl, sslhost) == 0) {
			SSL_free(spocp->ssl);
			return 0;
		}
	} else if (check_cert_chain(spocp->ssl, *spocp->srv) == 0) {
		SSL_free(spocp->ssl);
		return 0;
	}

	spocp->contype = SSL_TLS;

	if (spocpc_debug)
		traceLog("STARTTLS done");

	return 1;
}

/*--------------------------------------------------------------------------------*/
/*
static int
ssl_socket_readn(SSL * ssl, char *buf, size_t len)
{
	return SSL_read(ssl, buf, len);
}
*/
/*--------------------------------------------------------------------------------*/
/*
static int
ssl_socket_writen(SSL * ssl, char *buf, size_t len)
{
	return SSL_write(ssl, buf, len);
}
*/
#endif

/*=================================================================================*/

/*!
 * Closes the connection to a SPOCP server
 * If it is a SSL/TLS protected connection SSL/TLS is shutdown before the 
 * filedescriptor is closed. SSL/TLS connected structures are clear out.
 * \param spocp The Spocp session
 * \return nothing
 */

void
spocpc_close(SPOCP * spocp)
{

#ifdef HAVE_SSL

	if (spocp->contype == SSL_TLS) {
		SSL_shutdown(spocp->ssl);
		spocp->ssl = 0;
		SSL_CTX_free(spocp->ctx);
		spocp->ctx = 0;
	}
#endif

	close(spocp->fd);
	spocp->fd = 0;

	spocp->contype = UNCONNECTED;
}

/*--------------------------------------------------------------------------------*/

static void
spocpc_clr(SPOCP * s)
{
	if (s) {
		if (s->srv)
			free(s->srv);

#ifdef HAVE_SSL
		if (s->sslEntropyFile)
			free(s->sslEntropyFile);
		if (s->certificate)
			free(s->certificate);
		if (s->privatekey)
			free(s->privatekey);
		if (s->calist)
			free(s->calist);
		if (s->passwd)
			free(s->passwd);
#endif
	}
}

/*!
 * Clears a SPOCP session structure and frees the structure itself
 * \param s The Spocp session
 * \return nothing 
 */
void
free_spocp(SPOCP * s)
{
	if (s) {
		spocpc_clr(s);
		free(s);
	}
}

/*--------------------------------------------------------------------------------*/

static int
sexp_get_len(char **str, int *rc)
{
	char *ep, *sp = *str;
	long l;

	/* strtol skips SP (0x20) and allows a leading '-'/'+' so
	 * I have to check that first, since those are not allowed in
	 * this context */

	if (*sp == ' ' || *sp == '-' || *sp == '+') {
		*rc = SPOCPC_SYNTAXERROR;
		return -1;
	}

	l = strtol(sp, &ep, 10);

	/* no number read */
	if (ep == sp) {
		*rc = SPOCPC_SYNTAXERROR;
		return -1;
	}

	/* ep points to the first non digit character or '\0',
	 * should be ':' */

	if (*ep != ':') {
		*rc = SPOCPC_SYNTAXERROR;
		return -1;
	}

	*str = ++ep;

	return (int) l;
}
/*--------------------------------------------------------------------------------*/

static int
skip_length(char **sexp, int n, int *rc)
{
	char *sp = *sexp;
	int l;

	if ((l = sexp_get_len(&sp, rc)) < 0)
		return 0;

	n -= (sp - *sexp);
	if (l > n) {
		*rc = SPOCPC_MISSING_CHAR;
		return 0;
	}

	*sexp = sp;
	return l;
}

/*--------------------------------------------------------------------------------*/

static char *
sexp_get_next_element(char *sexp, int n, int *rc)
{
	char *sp = sexp;
	int l;

	if ((l = sexp_get_len(&sp, rc)) < 0)
		return 0;

	n -= (sp - sexp);
	if (l > n) {
		*rc = SPOCPC_MISSING_CHAR;
		return 0;
	} else if (l < n)
		return sp + l;
	else
		return 0;
}

/*--------------------------------------------------------------------------------*/

static int
spocpc_sexp_elements(char *resp, int n, octet_t line[], int size, int *rc)
{
	char *next, *sp;
	int l, i;

	if (spocpc_debug)
		traceLog("spocpc_sexp_elements");

	sp = resp;
	i = 0;
	*rc = SPOCPC_OK;

	do {
		line[i].val = sp;

		if (spocpc_debug)
			traceLog("n [%d]", n);
		next = sexp_get_next_element(sp, n, rc);
		if (spocpc_debug)
			traceLog("next (%d)", *rc);

		if (next) {
			l = next - sp;
			n = (n - l);
		} else {
			if (spocpc_debug)
				traceLog("No next");
			if (*rc != SPOCPC_OK)
				return -1;
			l = n;
			n = 0;
		}

		line[i++].len = l;
		sp = next;
	} while (n && next && i < size);

	if (i == size)
		return -1;

	line[i].len = 0;

	return i;
}

/*--------------------------------------------------------------------------------*/

static int
sexp_get_response(octet_t * buf, octet_t * code, octet_t * info)
{
	size_t l, d, n;
	char *sp, *cp;
	int rc;

	code->val = buf->val;
	code->len = buf->len;

	n = buf->len;
	sp = code->val;

	if ((l = sexp_get_len(&sp, &rc)) < 0)
		return rc;

	d = (sp - buf->val);
	if (l != (n - d))
		return 0;

	if ((l = sexp_get_len(&sp, &rc)) < 0)
		return rc;

	n -= (sp - buf->val);

	if (l == 3 && l < n) {
		code->len = l;
		code->val = sp;

		n -= 3;
		if (n) {	/* expects some info too */
			/* skip the length field */
			sp += 3;
			cp = sp;	/* to know where I've been */

			/* is this really a major fault ? */
			if ((l = sexp_get_len(&sp, &rc)) < 0)
				return rc;

			info->val = sp;
			info->len = n - (sp - cp);
		} else
			info->len = 0;

		return SPOCPC_OK;
	}

	return SPOCPC_SYNTAXERROR;
}

static int
spocpc_connect(char *srv, int nsec)
{
	int port, res = 0, sockfd = 0, val;
	struct sockaddr_un sun;
	struct sockaddr_in sin;
	struct in_addr **pptr;
	struct hostent *hp;
	char *cp, *host, buf[256];


	if (srv == 0) return -1;

	if (*srv == '/') {	/* unix domain socket */

		sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);

		memset(&sun, 0, sizeof(sun));
		sun.sun_family = AF_LOCAL;

		strcpy(sun.sun_path, srv);

		res = connect(sockfd, (SA *) & sun, sizeof(sun));

		if (res != 0) {
			traceLog("connect error [%s] \"%s\"\n", srv,
			    strerror_r(errno, buf, 256));
			return 0;
		}
	} else {		/* IP socket */
		int one = 1;

		host = strdup(srv);
		if ((cp = index(host, ':')) == 0)
			return 0;	/* no default port */
		*cp++ = '\0';
		port = atoi(cp);

		sockfd = socket(AF_INET, SOCK_STREAM, 0);

		if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &one,
			sizeof(int)) < 0) {
			traceLog("Unable to disable nagle: %.100s",
			    strerror(errno));
			return 0;
		}

		val = fcntl(sockfd, F_GETFL, 0);
		if (val == -1) {
			traceLog
			    ("Unable to get file descriptor flags on fd=%d: %s",
			    sockfd, strerror(errno));
			close(sockfd);
			return 0;
		}

		if (fcntl(sockfd, F_SETFL, val | O_NONBLOCK) == -1) {
			traceLog("Unable to set socket nonblock on fd=%d: %s",
			    sockfd, strerror(errno));
			close(sockfd);
			return 0;
		}

		if (strcmp(host, "localhost") == 0) {

			memset(&sin, 0, sizeof(sin));
			sin.sin_family = AF_INET;
			sin.sin_port = htons(port);
			sin.sin_addr.s_addr = inet_addr("127.0.0.1");

			res = connect(sockfd, (SA *) & sin, sizeof(sin));
		} else {
			/* Use getaddrinfo and freeaddrinfo instead ?? */
			if ((hp = gethostbyname(host)) == NULL) {
				traceLog("DNS problem with %s\n", host);
				return 0;
			}

			pptr = (struct in_addr **) hp->h_addr_list;

			for (; *pptr; pptr++) {
				bzero(&sin, sizeof(sin));
				sin.sin_family = AF_INET;
				sin.sin_port = htons(port);
				memcpy(&sin.sin_addr, *pptr,
				    sizeof(struct in_addr));

				if ((res =
					connect(sockfd, (SA *) & sin,
					    sizeof(sin))) == -1) {
					if (errno == EINPROGRESS)
						break;
				}
			}

			if (!(res == 0 || errno == EINPROGRESS)) {
				traceLog
				    ("Was not able to connect to %s at %d\n",
				    host, port);
				perror("connect");
				return 0;
			}
		}

		if (res != 0) {
			fd_set rset, wset;
			struct timeval tv;
			int error, len;

			FD_ZERO(&rset);
			FD_SET(sockfd, &rset);
			wset = rset;
			tv.tv_sec = nsec;
			tv.tv_usec = 0;

			if ((res =
				select(sockfd + 1, &rset, &wset, NULL,
				    nsec ? &tv : NULL)) == 0) {
				close(sockfd);
				traceLog("Connection attempt timedout");
				errno = ETIMEDOUT;
				return -1;
			}

			if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
				len = sizeof(error);
				if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR,
					&error, (socklen_t *) &len) < 0)
					return -1;
			} else {
				traceLog("select error, sockfd not set");
				return -1;
			}
		}
	}


	return sockfd;
}

/* ============================================================================== */

/*!
 * \brief Initializes a Spocp session, that is creates a Spocp struct but does not
 *   connect to any server.
 * \param hosts Where the servers is to be found, space separated list of servers
 * \param tv_sec The seconds part of the timeout for I/O (read/writes)
 * \param tv_usec The usec part of the timeout value
 * \return A Spocp connection struct
 */
SPOCP *
spocpc_init(char *hosts, long tv_sec, long tv_usec)
{
	SPOCP	*spocp;
	int	n;

	spocp = (SPOCP *) calloc(1, sizeof(SPOCP));
	spocp->srv = line_split( hosts, ' ', '\\', 0, 0, &n) ;
	spocp->cursrv = spocp->srv;
	if ( tv_sec != 0L && tv_usec != 0L) {
		spocp->com_timeout = (struct timeval *) Malloc (sizeof(struct timeval));
		spocp->com_timeout->tv_sec = tv_sec;
		spocp->com_timeout->tv_usec = tv_usec;
	}

	return spocp;
}

/*!
 * Opens a new Spocp session. Inlcudes not just creating the session structure but
 * also opening a non-SSL connection to the Spocp server of choice
 * \param spocp A spocp session struct gotten from spocpc_init() or NULL
 * \param srv The spocp server definition, either a host:port specification or a 
 *            filename representing a unix domain socket .
 * \param nsec The number of seconds that should pass before giving up on a server
 * \return A Spocp session pointer or NULL on failure
 */

SPOCP *
spocpc_open(SPOCP * spocp, char *srv, int nsec)
{
	int fd = 0;
	char **srvs;

	if (srv) {
		if ((fd = spocpc_connect(srv, nsec)) > 0 ) {
			if (spocp == 0)
				spocp = spocpc_init(srv, 0L, 0L) ;
			else {
				char	**arr;
				int	n;

				for( arr = spocp->srv, n = 0; arr; arr++) n++;

				arr = Realloc( spocp->srv, n+2 * sizeof(char *));
				arr[n+1] = 0;
				arr[n] = strdup(srv);
				spocp->srv = arr;
				spocp->cursrv = spocp->srv+n;
			}
		}
	}

	if (fd <= 0 && spocp && spocp->srv) {
		for( srvs = spocp->cursrv ; *srvs ; srvs++) {
			if(( fd = spocpc_connect(*srvs, nsec)) >0 ) {
				spocp->cursrv = srvs;
				break;
			}
		}
		if (srvs == 0) spocp->cursrv = spocp->srv;
	}

	if (fd == 0)
		return NULL;

	if (spocpc_debug)
		traceLog("Connected on %d", fd);

	if (!spocp)
		spocp = (SPOCP *) calloc(1, sizeof(SPOCP));

	spocp->fd = fd;
	spocp->contype = SOCKET;

	return spocp;
}

/*--------------------------------------------------------------------------------*/

/*!
 * \brief Tries to reconnect to a Spocp server after the previous connection
 *   for some reason has disappeared
 * \param spocp The connection struct
 * \param nsec The amount of time this routine should wait for the server to answer
 *   befor giving up
 * \return TRUE if the reconnection was successful otherwise FALSE
 */
int
spocpc_reopen(SPOCP * spocp, int nsec)
{
	char **arr;

	close(spocp->fd);	/* necessary if other side has already closed ? */

	arr = spocp->cursrv;	/* Retry the same first */

	while( arr ) {
		spocp->fd = spocpc_connect(*arr, nsec);
		if (spocp->fd <= 0) spocp->cursrv++;
	}
	if (arr == 0) {
		arr = spocp->srv;
		while( arr != spocp->cursrv ) {
			spocp->fd = spocpc_connect(*arr, nsec);
			if (spocp->fd <= 0) spocp->cursrv++;
		}
	}

	if (spocp->fd <= 0)
		return FALSE;
	else {
		spocp->cursrv = arr;
		return TRUE;
	}
}

/*--------------------------------------------------------------------------------*/

/*!
 * Writes size bytes to a spocp server, Uses SSL/TLS if active.
 * \param spocp The Spocp session
 * \param str A pointer to the buffer containing the bytes to be written
 * \param n The number of bytes to write
 * \return The number of bytes written or -1 if no bytes could be written
 */
ssize_t
spocpc_writen(SPOCP * spocp, char *str, ssize_t n )
{
	ssize_t nleft, nwritten = 0 ;
	char *sp;
	fd_set wset;
	int retval;

	sp = str;
	nleft = n;
	FD_ZERO(&wset);

	spocp->rc = 0;

	if (spocpc_debug)
		traceLog("Writen fd:%d", spocp->fd);

	while (nleft > 0) {
		FD_SET(spocp->fd, &wset);
		retval = select(spocp->fd + 1, NULL, &wset, NULL, spocp->com_timeout);

		if (retval) {
#ifdef HAVE_SSL
			if (spocp->contype == SSL_TLS)
				nwritten = SSL_write(spocp->ssl, (void *) sp,
				    (int) nleft);
			else
#endif
				nwritten = write(spocp->fd, sp, nleft);

			if (nwritten <= 0) {;
				if (errno == EINTR)
					nwritten = 0;
				else
					return -1;
			}

			nleft -= nwritten;
			sp += nwritten;
		} else {	/* timed out */
			spocp->rc = SPOCPC_TIMEOUT;
			break;
		}
	}

	fdatasync(spocp->fd);

	return (n - nleft);
}

/*--------------------------------------------------------------------------------*/
/*!
 * Attempts to read size bytes to a spocp server, Uses SSL/TLS if active.
 * \param spocp The Spocp session
 * \param buf A pointer to the buffer where the bytes read should be placed
 * \param size The number of bytes to read
 * \return The number of bytes read or -1 if no bytes could be read
 */

ssize_t
spocpc_readn(SPOCP * spocp, char *buf, ssize_t size)
{
	fd_set rset;
	int retval;
	ssize_t n = 0;

	FD_ZERO(&rset);

	spocp->rc = 0;

	FD_SET(spocp->fd, &rset);
	retval = select(spocp->fd + 1, &rset, NULL, NULL, spocp->com_timeout);

	if (retval) {
#ifdef HAVE_SSL
		if (spocp->contype == SSL_TLS)
			n = SSL_read(spocp->ssl, (void *) buf, (int) size);
		else
#endif
			n = read(spocp->fd, buf, size);

		if (n <= 0) {
			if (errno == EINTR) {
				spocp->rc = SPOCPC_INTERUPT;
				n = 0;
			}
			else
				return -1;
		}
	} else {		/* timed out */
		spocp->rc = SPOCPC_TIMEOUT;
	}

	return (n);
}

/*--------------------------------------------------------------------------------*/

static int
spocpc_get_result_code(octet_t * rc)
{
	int i;

	for (i = 0; spocp_rescode[i].num; i++) {
		if (strncmp(rc->val, spocp_rescode[i].num, 3) == 0)
			return spocp_rescode[i].code;
	}

	return SPOCPC_OTHER;
}

/*--------------------------------------------------------------------------------*/

static int
spocpc_answer_ok(char *resp, size_t n, queres_t * qr)
{
	octet_t	code, info;
	octet_t	elem[32];	/* max 15 blobs */
	int	res = SPOCPC_TIMEOUT;
	int	i;
	char	cent = 0;

	if ((n = spocpc_sexp_elements(resp, n, elem, 32, &res)) < 0)
		return res;

	if (spocpc_debug)
		traceLog("Got %d elements", n);

	if (n == 1) {
		if ((res =
			sexp_get_response(&elem[0], &code,
			    &info)) != SPOCPC_OK) {
			if (spocpc_debug)
				traceLog("sexp_response returned %d",
				    (int) res);
			return res;
		}

		if (spocpc_debug) {
			strncpy(prbuf, code.val, code.len);
			prbuf[code.len] = '\0';
			traceLog("Spocp returned code: [%s]", prbuf);
		}

		qr->rescode = spocpc_get_result_code(&code);
		qr->str = strndup(info.val, info.len);
		qr->blob = 0;
	} else if (n > 1) {
		n--;
		/* first the blobs */

		for (i = 0; i < (int) n; i++) {
			if ((res =
				sexp_get_response(&elem[i], &code,
				    &info)) != SPOCPC_OK) {
				if (spocpc_debug)
					traceLog("sexp_response returned %d",
					    (int) res);
				return res;
			}

			/* multilines are resultcodes 201 or 301 followed by 200 resp. 300 */
			if ((*code.val == '2' || *code.val)
			    && code.val[1] == '0' && code.val[2] == '1') {
				if (cent == 0)
					cent = *code.val;
				else if (cent != *code.val)
					return SPOCPC_PROTOCOL_ERROR;

				if (spocpc_debug) {
					strncpy(prbuf, info.val, info.len);
					prbuf[info.len] = '\0';
					traceLog("Spocp returned info: [%s]",
					    prbuf);
				}

				qr->blob = octarr_add( qr->blob, octdup(&info));
			} else
				return SPOCPC_PROTOCOL_ERROR;
		}

		/* Then the response code, which must be X00, where X is
		   the same as in the previous rescode */
		if ((res =
			sexp_get_response(&elem[i], &code,
			    &info)) != SPOCPC_OK) {
			if (spocpc_debug)
				traceLog("sexp_response returned %d",
				    (int) res);
			return res;
		}

		if (spocpc_debug) {
			strncpy(prbuf, code.val, code.len);
			prbuf[code.len] = '\0';
			traceLog("Spocp returned code: [%s]", prbuf);
		}

		if (cent != *code.val)
			return SPOCPC_PROTOCOL_ERROR;

		qr->rescode = spocpc_get_result_code(&code);
		qr->str = strndup(info.val, info.len);
	}

	return res;
}

/*--------------------------------------------------------------------------------*/

static int
spocpc_protocol_op(octarr_t *oa, octet_t *prot)
{
	char *op, *sp;
	int i, l, la, tot;
	size_t n;

	for (i = 0, l = 0; i < oa->n ; i++) {
		la = oa->arr[i]->len ;
		l += la + DIGITS(la) + 1;
	}

	tot = (l + DIGITS(l) + 1);
	op = prot->val = (char *) malloc((tot + 1) * sizeof(char));

	sprintf(op, "%d:", l);
	l = strlen(op);
	op += l;
	n = (size_t) tot - l;

	sp = sexp_printv( op, &n, "X", oa);

	if (sp == 0) return -1;

	prot->len = tot - (int) n;

	return prot->len;
}

/* ============================================================================== */

static int
spocpc_just_send_X(SPOCP * spocp, octarr_t *argv)
{
	octet_t o;
	int	l, n;

	l = spocpc_protocol_op(argv, &o);

	if (spocpc_debug) {
		char *tmp;
		tmp = oct2strdup( &o, '%');

		traceLog("[%s](%d) ->(%d/%d)", tmp, l, spocp->fd,
		    spocp->contype);

		free( tmp );
	}

	/* what if I haven't been able to write everything ? */
	n = spocpc_writen(spocp, o.val, o.len);
	octclr(&o);

	if (n < 0) {
		return SPOCPC_CON_ERR;
	}
	else if ( n == 0 && spocp->rc ) {
		return spocp->rc;
	}

	if (spocpc_debug)
		traceLog("Wrote %d chars", n);

	return SPOCPC_OK;
}

/* ============================================================================== */

static int
spocpc_get_result(SPOCP * spocp, queres_t * qr)
{
	char	resp[BUFSIZ];
	int	ret, n;

	if ((n = spocpc_readn(spocp, resp, BUFSIZ)) < 0)
		return SPOCPC_OTHER;
	else if (n == 0 && spocp->rc)
		return spocp->rc;

	if (spocpc_debug)
		traceLog("Read %d chars", n);

	resp[n] = '\0';

	ret = spocpc_answer_ok(resp, n, qr);

	if (spocpc_debug)
		traceLog("response: [%s] (%d)", resp, ret);

	return ret;
}

/* ============================================================================== */

static int
spocpc_send_X(SPOCP * spocp, octarr_t *argv, queres_t * qr)
{
	int	ret;

	if ((ret = spocpc_just_send_X(spocp, argv)) == SPOCPC_OK)
		ret = spocpc_get_result(spocp, qr);

	return ret;
}

/*--------------------------------------------------------------------------------*/
/*!
 * Send a Spocp query to a spocp server
 * \param spocp The Spocp session
 * \param path The name of the ruleset
 * \param query The string buffer holding the query in the form of a s-expression
 * \param qr A struct into which the result received from the spocp server 
 *   should be placed
 * \return A Spocpc result code
 */
int
spocpc_str_send_query(SPOCP * spocp, char *path, char *query, queres_t * qr)
{
	octarr_t	argv;
	octet_t		arr[3], *ap = arr;

	argv.arr = &ap;
	argv.size = 0;
	argv.n = 3;

	oct_assign(&arr[0], "QUERY");

	if (path && *path)
		oct_assign(&arr[1],path);
	else
		arr[1].len = 0;

	oct_assign(&arr[2], query);

	return spocpc_send_X(spocp, &argv, qr);
}

int
spocpc_send_query(SPOCP * spocp, octet_t *path, octet_t *query, queres_t * qr)
{
	octarr_t	argv;
	octet_t		*arr[3], op;

	argv.arr = arr;
	argv.size = 0;
	argv.n = 3;

	oct_assign(&op, "QUERY");
	arr[0] = &op;

	if (path)
		arr[1] = path;
	else
		arr[1] = 0;

	arr[2] = query;

	return spocpc_send_X(spocp, &argv, qr);
}

/*--------------------------------------------------------------------------------*/

/*!
 * Send a Spocp delete operation to a spocp server
 * \param spocp The Spocp session
 * \param path The name of the ruleset
 * \param rule The ruleID that one wants to remove
 * \param qr A struct into which the result received from the spocp server 
 *   should be placed
 * \return A Spocpc result code
 */
int
spocpc_str_send_delete(SPOCP * spocp, char *path, char *rule, queres_t * qr)
{
	octarr_t	argv;
	octet_t		arr[3], *ap = arr;

	argv.arr = &ap;
	argv.size = 0;
	argv.n = 3;

	oct_assign(&arr[0], "DELETE");

	if (path && *path)
		oct_assign(&arr[1],path);
	else
		arr[1].len = 0;

	oct_assign(&arr[2], rule);

	return spocpc_send_X(spocp, &argv, qr);
}

int
spocpc_send_delete(SPOCP * spocp, octet_t *path, octet_t *rule, queres_t * qr)
{
	octarr_t	argv;
	octet_t		*arr[3], op;

	argv.arr = arr;
	argv.size = 0;
	argv.n = 3;

	oct_assign(&op, "DELETE");
	arr[0] = &op;

	if (path)
		arr[1] = path;
	else
		arr[1] = 0;

	arr[2] = rule;

	return spocpc_send_X(spocp, &argv, qr);
}

/*--------------------------------------------------------------------------------*/

/*!
 * Send a subject definition to a spocp server
 * \param spocp The Spocp session
 * \param subject The string in someway representing information about the subject
 *   that is controlling this session
 * \param qr A struct into which the result received from the spocp server 
 *   should be placed
 * \return A Spocpc result code
 */
int
spocpc_str_send_subject(SPOCP * spocp, char *subject, queres_t * qr)
{
	octarr_t	argv;
	octet_t		arr[2], *ap = arr;

	argv.arr = &ap;
	argv.size = 0;
	argv.n = 1;

	oct_assign(&arr[0], "SUBJECT");

	if (subject && *subject) {
		oct_assign(&arr[1],subject);
		argv.n = 2;
	}

	return spocpc_send_X(spocp, &argv, qr);
}

/*--------------------------------------------------------------------------------*/

int
spocpc_send_subject(SPOCP * spocp, octet_t *subject, queres_t * qr)
{
	octarr_t	argv;
	octet_t		*arr[3], op;

	argv.arr = arr;
	argv.size = 0;
	argv.n = 1;

	oct_assign(&op, "SUBJECT");
	arr[0] = &op;

	if (subject) {
		arr[1] = subject;
		argv.n = 2;
	}

	return spocpc_send_X(spocp, &argv, qr);
}

/*--------------------------------------------------------------------------------*/

/*!
 * Send a rule to a Spocp server
 * \param spocp The Spocp session
 * \param path The name of the ruleset
 * \param rule The string buffer holding the query
 * \param bcond The boundary condition expression
 * \param info The static blob 
 * \param qr A struct into which the result received from the spocp server 
 *   should be placed
 * \return A Spocpc result code
 */
int
spocpc_str_send_add(SPOCP * spocp, char *path, char *rule, char *bcond,
	char *info, queres_t * qr)
{
	octarr_t	argv;
	octet_t		arr[5], *ap = arr;

	if (rule == 0 || *rule == '\0')
		return SPOCPC_PARAM_ERROR;

	argv.arr = &ap;
	argv.size = 0;
	argv.n = 3;

	oct_assign(&arr[0], "ADD");

	if (path && *path)
		oct_assign(&arr[1],path);
	else
		arr[1].len = 0;

	oct_assign(&arr[2], rule);

	if (bcond && *bcond) {
		oct_assign(&arr[3], bcond);
		argv.n = 4;
	}

	if (info && *info) {
		if (bcond == 0 || *bcond == 0)
			oct_assign(&arr[3], "NULL");

		oct_assign(&arr[4], info);
		argv.n = 5;
	}

	return spocpc_send_X(spocp, &argv, qr);
}

/*--------------------------------------------------------------------------------*/
int
spocpc_send_add(SPOCP * spocp, octet_t *path, octet_t *rule, octet_t *bcond,
	octet_t *info, queres_t * qr)
{
	octarr_t	argv;
	octet_t		*arr[5], op, bc;

	if (rule == 0 )
		return SPOCPC_PARAM_ERROR;

	argv.arr = arr;
	argv.size = 0;
	argv.n = 3;

	oct_assign(&op, "ADD");
	arr[0] = &op;

	if (path)
		arr[1] = path;
	else
		arr[1] = 0;

	arr[2] = rule;

	if (bcond) {
		arr[3] = bcond;
		argv.n = 4;
	}

	if (info) {
		if (bcond == 0) {
			oct_assign(&bc, "NULL");
			arr[3] = &bc;
		}

		arr[4] = info;
		argv.n = 5;
	}

	return spocpc_send_X(spocp, &argv, qr);
}

/*--------------------------------------------------------------------------------*/

/*!
 * \brief Sends a LOGOUT request to a Spocp server
 * \param spocp The Spocp session
 * \return A spocpc result code
 */
int
spocpc_send_logout(SPOCP * spocp)
{
	octarr_t	argv;
	octet_t		arr[1], *ap = arr;
	queres_t	qres;

	argv.arr = &ap;
	argv.size = 0;
	argv.n = 1;

	oct_assign(&arr[0], "LOGOUT");

	memset( &qres, 0, sizeof( queres_t));

	return spocpc_send_X(spocp, &argv, &qres);
}

/*--------------------------------------------------------------------------------*/

/*!
 * \brief Sends a BEGIN (transaction start) request to a Spocp server
 * \param spocp The Spocp session
 * \param qr A struct into which the result received from the spocp server 
 *   should be placed
 * \return A spocpc result code
 */
int
spocpc_open_transaction(SPOCP * spocp, queres_t * qr)
{
	octarr_t	argv;
	octet_t		arr[1], *ap = arr;

	argv.arr = &ap;
	argv.size = 0;
	argv.n = 1;

	oct_assign(&arr[0], "BEGIN");

	return spocpc_send_X(spocp, &argv, qr);
}

/*--------------------------------------------------------------------------------*/

/*!
 * \brief Sends a COMMIT (transaction end) request to a Spocp server
 * \param spocp The Spocp session
 * \param qr A struct into which the result received from the spocp server 
 *   should be placed
 * \return A spocpc result code
 */
int
spocpc_commit(SPOCP * spocp, queres_t * qr)
{
	octarr_t	argv;
	octet_t		arr[1], *ap = arr;

	argv.arr = &ap;
	argv.size = 0;
	argv.n = 1;

	oct_assign(&arr[0], "COMMIT");

	return spocpc_send_X(spocp, &argv, qr);
}

/*--------------------------------------------------------------------------------*/

/*!
 * \brief Sends a STARTTLS request to a Spocp server
 * \param spocp The Spocp session
 * \param qr A struct into which the result received from the spocp server 
 *   should be placed
 * \return A spocpc result code
 */
int
spocpc_attempt_tls(SPOCP * spocp, queres_t * qr)
{
	int res = SPOCPC_OK;
#ifdef HAVE_SSL
	octarr_t	argv;
	octet_t		arr[1], *ap = arr;

	if (spocp->contype == SSL_TLS)
		return SPOCPC_STATE_VIOLATION;

	argv.arr = &ap;
	argv.size = 0;
	argv.n = 1;

	oct_assign(&arr[0], "STARTTLS");

	return spocpc_send_X(spocp, &argv, qr);
#else
	res = SPOCPC_NOSUPPORT;
#endif

	return res;
}

/* ============================================================================== */

/*! 
 * \brief Takes a multiline response and works its way through the information
 *  returned, printing the relevant information to a filedescriptor
 * \param resp Memory area holding the multiline response
 * \param n  The size of the memory area
 * \param fp A file descriptor which is to be used for the output of this routine
 * \param wid If not zero print the rule ID together with the other information
 * \return A spocpc result code
 */
int
spocpc_parse_and_print_list(char *resp, int n, FILE * fp, int wid)
{
	octet_t code, content;
	int l, re, j, sn, ln;
	octet_t elem[64], part[6];
	int rc = SPOCPC_TIMEOUT, res;

	if ((re = spocpc_sexp_elements(resp, n, elem, 64, &rc)) == -1)
		return rc;

	if (spocpc_debug)
		traceLog("Got %d elements", re);

	if (re == 0)
		return rc;

	for (j = 0; j < re; j++) {

		l = elem[j].len;

		if (spocpc_debug) {
			strncpy(prbuf, elem[j].val, elem[j].len);
			prbuf[elem[j].len] = '\0';
			traceLog("Element[%d]: [%s](%d)", j, prbuf,
			    elem[j].len);
		}

		if ((res =
			sexp_get_response(&elem[j], &code,
			    &content)) != SPOCPC_OK) {
			rc = res;
			break;
		}

		if (spocpc_debug) {
			strncpy(prbuf, code.val, code.len);
			prbuf[code.len] = '\0';
			traceLog("code: [%s](%d)", prbuf, code.len);

			strncpy(prbuf, content.val, content.len);
			prbuf[content.len] = '\0';
			traceLog("content: [%s](%d)", prbuf, content.len);
		}

		if (strncmp(code.val, "200", 3) == 0) {	/* have gotten everything I'm to get */
			rc = SPOCPC_OK;
			break;
		} else if (strncmp(code.val, "201", 3) == 0) {	/* multiline response */
			if ((sn =
				spocpc_sexp_elements(content.val, content.len,
				    part, 8, &rc)) == -1)
				return rc;

			if (spocpc_debug)
				traceLog("%d parts", sn);

			ln = skip_length(&(part[0].val), part[0].len, &rc);

			strncpy(prbuf, part[0].val, ln);
			prbuf[ln] = '\0';
			if (spocpc_debug)
				traceLog("path: [%s]", prbuf);
			fwrite(prbuf, 1, ln, fp);
			fwrite(" ", 1, 1, fp);

			if (wid) {

				ln = skip_length(&(part[1].val), part[1].len,
				    &rc);

				if (spocpc_debug)
					traceLog("ruleid length %d", ln);

				strncpy(prbuf, part[1].val, ln);
				prbuf[ln] = '\0';
				if (spocpc_debug)
					traceLog("ruleid: [%s]", prbuf);
				fwrite(prbuf, 1, ln, fp);
				fwrite(" ", 1, 1, fp);
			}

			ln = skip_length(&(part[2].val), part[2].len, &rc);

			strncpy(prbuf, part[2].val, ln);
			prbuf[ln] = '\0';
			if (spocpc_debug)
				traceLog("rule: [%s]", prbuf);
			fwrite(prbuf, 1, ln, fp);
			fwrite("\n", 1, 1, fp);
		} else {
			rc = SPOCPC_PROTOCOL_ERROR;
			break;
		}
	}

	return rc;
}

/*--------------------------------------------------------------------------------*/
