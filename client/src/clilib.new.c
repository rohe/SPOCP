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

#ifdef GLIBC2
#define _GNU_SOURCE
#endif

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
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>

#include <spocp.h>
#include <wrappers.h>
#include <log.h>
#include <spocpcli.h>

#ifdef HAVE_SASL
#include <sasl/sasl.h>
#include <sasl/saslutil.h>
#endif

/* #define AVLUS 1 */

#if!defined(MAXHOSTNAMELEN) || (MAXHOSTNAMELEN < 64)
#undef MAXHOSTNAMELEN
/*! The max size of a FQDN */
#define MAXHOSTNAMELEN 256
#endif

/*! If nothing else is specified this is how long I will wait on a Spocp server to answer
 * my call
 */
#define SPOCP_DEFAULT_CONN_WAIT     5

/*! Should not have to define this here */
/*
char    *strndup(const char *old, size_t sz);
*/

/*! Local shothand alias */
typedef struct sockaddr SA;

struct _err2str_map spocpc_err2str_map[] = {
    { SPOCPC_SYNTAXERROR, "Syntax Error"},
    { SPOCPC_MISSING_CHAR, "Missing characters in input" },
    { SPOCPC_OK, "OK" },
    { SPOCPC_SYNTAX_ERROR, "Syntax Error"},
    { SPOCPC_PROTOCOL_ERROR, "Protocol Error"},
    { SPOCPC_OTHER, "Unspecified Error" },
    { SPOCPC_TIMEOUT, "Timeout occured" },
    { SPOCPC_CON_ERR, "Connecction Problem" },
    { SPOCPC_PARAM_ERROR, "Parameter Error" },
    { SPOCPC_STATE_VIOLATION, "State violation" },
    { SPOCPC_NOSUPPORT, "Function not supported"},
    { SPOCPC_INTERUPT, "Operation was interupted" },
    { SPOCPC_UNKNOWN_RESCODE, "What ??"},
    { 0, NULL }
} ;

/*! 
 * \var prbuf
 * \brief global variable to hold strings to be printed
 */
char prbuf[BUFSIZ];

octarr_t *spocpc_sexp_elements( octet_t *oct, int *rc) ;
int spocpc_answer_ok(octet_t *resp, queres_t * qr);

int spocpc_str_send_query(SPOCP *, char *, char *, queres_t *);
int spocpc_str_send_delete(SPOCP *, char *, char *, queres_t *);
int spocpc_str_send_subject(SPOCP *, char *, queres_t *);
int spocpc_str_send_add(SPOCP *, char *, char *, char *, char *, queres_t *);

static int _connect(int sockfd, char *host, int port, long timeout);


/*------------------------------------------------------------------------*/

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
    size_t len;

    len = strlen( (char *) userdata);
    if (num < (int) len + 1) {
        if (spocpc_debug)
            traceLog(LOG_DEBUG,"Not big enough place for the password (%d)",
                num);
        return (0);
    }

    strncpy(buf, userdata, num-1);
    return ((int) len);
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
        traceLog(LOG_ERR,"SSL verify error: depth=%d error=%s cert=%s",
            x509ctx->error_depth,
            X509_verify_cert_error_string(x509ctx->error), txt);
        return 0;
    }

    /* the check against allowed clients are done elsewhere */

    if (spocpc_debug)
        traceLog(LOG_DEBUG,"SSL authenticated peer: %s\n", txt);

    return 1;       /* accept */
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
    SSL_load_error_strings();   /* basic set up */

    if (!RAND_status()) {
        if (spocpc_debug)
            traceLog(LOG_DEBUG,"Have to find more entropy\n");

        /* get some entropy */

        add_entropy(spocp->sslEntropyFile);

        if (!RAND_status()) {
            if (spocpc_debug)
                traceLog(LOG_DEBUG,
                    "Failed to find enough entropy on your system");
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
            traceLog(LOG_DEBUG,"Failed to load certificate chain file");
        SSL_CTX_free(spocp->ctx);
        spocp->ctx = 0;
        return 0;
    }

    if (spocp->passwd) {
        SSL_CTX_set_default_passwd_cb(spocp->ctx, password_cb);
        SSL_CTX_set_default_passwd_cb_userdata(spocp->ctx,
            (void *) spocp->passwd);
    }

    if (spocp->privatekey && !SSL_CTX_use_PrivateKey_file(
                spocp->ctx, spocp->privatekey,
                SSL_FILETYPE_PEM)) {
        if (spocpc_debug)
            traceLog(LOG_DEBUG,"Failed to load private key file, passwd=%s",
                spocp->passwd);
        SSL_CTX_free(spocp->ctx);
        spocp->ctx = 0;
        return 0;
    }

    if (spocp->calist
        && !SSL_CTX_load_verify_locations(spocp->ctx, spocp->calist, 0)) {
        traceLog(LOG_ERR,"Failed to load ca file");
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
        traceLog(LOG_ERR,"Certificate doesn't verify");
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
        traceLog(LOG_DEBUG,"Peers subject name: %s", name);

    pp = index(host, ':');
    /* do I need to reset this, no */
    if (pp)
        *pp = 0;

    r = X509_NAME_get_text_by_NID(xn, NID_commonName, hostname, 256);
    if (spocpc_debug)
        traceLog(LOG_DEBUG,
            "Peers hostname: %s, connections hostname: %s",
            hostname, host);

/*
  if (strcasecmp(peer_CN,host)) {
    traceLog(LOG_ERR,"Common name doesn't match host name");
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
    char    sslhost[MAXHOSTNAMELEN];
    int     r;
    char    *target;
    int     cc;

    if (tls_init(spocp) == 0)
        return 0;

    if (spocpc_debug)
        traceLog(LOG_DEBUG,"TLS init done");

    /* Connect the SSL/TLS socket */
    if ((spocp->ssl = SSL_new(spocp->ctx)) == 0)
        return 0;

    if (SSL_set_fd(spocp->ssl, spocp->fd) != 1) {
        traceLog(LOG_ERR,"starttls: Error setting fd\n");
        SSL_free(spocp->ssl);
        return 0;
    }

    SSL_set_mode(spocp->ssl, SSL_MODE_AUTO_RETRY);

    if (spocpc_debug)
        traceLog(LOG_DEBUG,"SSL_connect");

    /* Initiates the TLS handshake with a server */
    while((r= SSL_connect(spocp->ssl)) <= 0)
    {
        int err = SSL_get_error(spocp->ssl, r);
        struct timeval tv = {0, 100};
        fd_set rfds;
        fd_set wfds;
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        if(err == SSL_ERROR_WANT_READ)
            FD_SET(spocp->fd, &rfds);
        else if(err == SSL_ERROR_WANT_WRITE)
            FD_SET(spocp->fd, &wfds);
        else
        {
            traceLog(LOG_ERR,"SSL connect failed %d", r);
            print_tls_errors();
            SSL_free(spocp->ssl);
            return 0;
        }
        err = select(spocp->fd + 1, &rfds, &wfds, NULL, &tv);
        if(err == -1)
            return(0);
    }

    target = oct2strdup(spocp->srv->arr[spocp->cursrv], '%');

    if (*target == '/' || (strcmp(target, "localhost") == 0)) {
        gethostname(sslhost, MAXHOSTNAMELEN);

        Free( target ); 

        if (check_cert_chain(spocp->ssl, (char *) sslhost) == 0) {
            SSL_free(spocp->ssl);
            return 0;
        }
    } else {
        cc = check_cert_chain(spocp->ssl, target);

        Free( target ); 

        if (cc== 0) {
            SSL_free(spocp->ssl);
            return 0;
        }
    }

    spocp->contype = SSL_TLS;

    if (spocpc_debug)
        traceLog(LOG_DEBUG,"STARTTLS done");

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

SPOCP *spocpc_cpy( SPOCP *copy, SPOCP *old )
{
    if( old == 0 || copy == 0)
        return 0;

    copy->contype = 0;
    copy->srv = old->srv;
    copy->cursrv = 0;
    copy->com_timeout.tv_sec = old->com_timeout.tv_sec;
    copy->com_timeout.tv_usec = old->com_timeout.tv_usec;
    copy->rc = 0;

#ifdef HAVE_SSL
    copy->servercert = old->servercert;
    copy->verify_ok = old->verify_ok;
    copy->tls = old->tls;
    copy->ctx = old->ctx;
    copy->ssl = 0;
#endif

    copy->sslEntropyFile = old->sslEntropyFile;
    copy->certificate = old->certificate;
    copy->privatekey = old->privatekey;
    copy->calist = old->calist;
    copy->passwd = old->passwd;

    copy->copy = 1;

    return copy;
}

SPOCP *spocpc_dup( SPOCP *old )
{
    SPOCP *copy;

    copy = (SPOCP *) Calloc(1, sizeof( SPOCP));

    return spocpc_cpy( copy, old );
}

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
        SSL_free(spocp->ssl);
        spocp->ssl = 0;
    }
#endif
#ifdef HAVE_SASL
    if (spocp->sasl) {
        sasl_dispose(&spocp->sasl);
        spocp->sasl_ssf = NULL;
        sasl_done();
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
        if (s->srv) {
            octarr_free(s->srv);
        }

#ifdef HAVE_SSL
        if (s->copy == 0) {
            SSL_CTX_free(s->ctx);
            s->ctx = 0;
        }
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
        if (s->copy == 0) 
            spocpc_clr(s);
        free(s);
    }
}

/*----------------------------------------------------------------------------*/

octarr_t *
spocpc_sexp_elements( octet_t *oct, int *rc)
{
    octet_t     ro, *op;
    octarr_t    *oa=NULL;

    if (spocpc_debug)
        traceLog(LOG_DEBUG,"spocpc_sexp_elements");

    while( oct->len && (*rc = get_str( oct, &ro )) == SPOCP_SUCCESS ) {
        op = oct_new( ro.len, ro.val);
        oa = octarr_add( oa, op);
    }

    return oa;
}

/*----------------------------------------------------------------------------*/
/* typically buf->val is 3:2002:Ok */
static int
sexp_get_response(octet_t * buf, octet_t * code, octet_t * info)
{
    int rc;

    /* First part of the response value */
    rc = get_str( buf, code );
    if (rc != SPOCP_SUCCESS)
        return SPOCPC_SYNTAXERROR;

    if ( buf->len ) {
        rc = get_str( buf, info );
    }
    else 
        memset( info, 0, sizeof( octet_t ));

    return SPOCPC_OK;
}

static int _connect_without_timeout(int soc, struct sockaddr *addr)
{
    return connect(soc, (struct sockaddr *)&addr, sizeof(addr)); 
}

static int _connect_with_timeout(int soc, struct sockaddr *addr, long sec)
{
    int             res, valopt;
    socklen_t       lon;
    struct timeval  tv;
    fd_set          myset;
    
    /* Trying to connect with timeout */
    res = connect(soc, (struct sockaddr *)&addr, sizeof(addr)); 
    if (res < 0) { 
        if (errno == EINPROGRESS) { 
            fprintf(stderr, "EINPROGRESS in connect() - selecting\n"); 
            do { 
                tv.tv_sec = sec; 
                tv.tv_usec = 0; 
                FD_ZERO(&myset); 
                FD_SET(soc, &myset); 
                res = select(soc+1, NULL, &myset, NULL, &tv); 
                if (res < 0 && errno != EINTR) { 
                    fprintf(stderr, "Error connecting %d - %s\n", errno, 
                            strerror(errno)); 
                    return -1; 
                } 
                else if (res > 0) { 
                    /* Socket selected for write */
                    lon = sizeof(int); 
                    if (getsockopt(soc, SOL_SOCKET, SO_ERROR, (void*)(&valopt), 
                                    &lon) < 0) { 
                        fprintf(stderr, "Error in getsockopt() %d - %s\n", 
                                errno, strerror(errno)); 
                        return -1; 
                    } 
                    /* Check the value returned... */
                    if (valopt) { 
                        fprintf(stderr, "Error in delayed connection() %d - %s\n", 
                                valopt, strerror(valopt)); 
                        return -1; 
                    } 
                    break; 
                }
                else { 
                    fprintf(stderr, "Timeout in select() - Cancelling!\n"); 
                    return -1; 
                } 
            } while (1); 
        } 
        else { 
            fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
            return -1; 
        } 
    }
    
    return res;
}

static int 
_connect(int sockfd, char *host, int port, long timeout)
{
    struct sockaddr_in  sin;
    struct in_addr      **pptr;
    struct hostent      *hp;
    int                 res = -1;

    if (strcmp(host, "localhost") == 0) {
        /* printf("spocp_connect - host=localhost\n"); */
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);
        sin.sin_addr.s_addr = inet_addr("127.0.0.1");

        res = _connect_without_timeout(sockfd, (SA *) &sin);
        /* res = _connect_with_timeout(sockfd, (SA *) &sin, timeout); */
        printf("spocp_connect - res=%d (%d, %d)\n", res, sockfd, errno); /* */
        if (res != -1)
            return sockfd;
    } else {
        /* Use getaddrinfo and freeaddrinfo instead ?? */
        if ((hp = gethostbyname(host)) == NULL) {
            traceLog(LOG_ERR,"DNS problem with %s\n", host);
            return -1;
        }

        pptr = (struct in_addr **) hp->h_addr_list;

        for (; *pptr; pptr++) {
            bzero(&sin, sizeof(sin));
            sin.sin_family = AF_INET;
            sin.sin_port = htons(port);
            memcpy(&sin.sin_addr, *pptr,
                sizeof(struct in_addr));

            res = _connect_with_timeout(sockfd, (SA *) &sin, timeout) ;
            if (res != -1)
                return sockfd;
        }
    }
    
    return res;
}

static int
spocpc_connect(char *srv, int nsec)
{
    int                 port, res = 0, sockfd = 0, val;
    char                *cp, *host, buf[256];


    if (srv == 0) return -2;

    if (*srv == '/') {  /* unix domain socket */

        struct sockaddr_un  serv_addr;

        if(( sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0 ) {
            traceLog(LOG_ERR,"Couldn't open socket\n");
            return -1;
        }

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sun_family = AF_UNIX;
        strlcpy(serv_addr.sun_path, srv, sizeof(serv_addr.sun_path));

        res = connect(sockfd, (SA *) &serv_addr, sizeof(serv_addr));

        if (res < 0) {
            traceLog(LOG_ERR,"connect error [%s] \"%s\"\n", srv,
#ifdef sun
                strerror(errno));
#else
                strerror_r(errno, buf, 256));
#endif
            return -1;
        }
    } else {        /* IP socket */
        int one = 1;

        host = strdup(srv);
        if ((cp = index(host, ':')) == 0)
            return -1;   /* no default port */
        *cp++ = '\0';
        port = atoi(cp);

        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &one,
            sizeof(int)) < 0) {
            traceLog(LOG_ERR,"Unable to disable nagle: %.100s",
                strerror(errno));
            return -1;
        }

        val = fcntl(sockfd, F_GETFL, 0);
        if (val == -1) {
            traceLog(LOG_ERR,
                "Unable to get file descriptor flags on fd=%d: %s",
                sockfd, strerror(errno));
            close(sockfd);
            return -1;
        }

        if (fcntl(sockfd, F_SETFL, val | O_NONBLOCK) == -1) {
            traceLog(LOG_ERR,"Unable to set socket nonblock on fd=%d: %s",
                sockfd, strerror(errno));
            close(sockfd);
            return -1;
        }

        res = _connect(sockfd, host, port, (long) nsec);
    }

    return sockfd;
}

/* ============================================================================== */

/*!
 * \brief Sets the timeout for responses from a Spocp server
 * \param spocp The spocp session
 * \param sec The number of seconds to wait
 * \param usec The number of microseconds to wait
 * 
 */

SPOCP *
spocpc_set_timeout( SPOCP *spocp, long sec, long usec)
{
    if (spocp == 0 && ( sec != 0L || usec != 0L ))
        spocp = (SPOCP *) calloc(1, sizeof(SPOCP));
        
    if (sec == 0L && usec == 0L)
        return spocp;

    spocp->com_timeout.tv_sec = sec;
    spocp->com_timeout.tv_usec = usec;

    return spocp;
}

/*!
 * \brief Initializes a Spocp session, that is creates a Spocp struct but does not
 *   connect to any server.
 * \param hosts Where the servers is to be found, space separated list of servers
 * \param tv_sec The seconds part of the timeout for I/O (read/writes)
 * \param tv_usec The usec part of the timeout value
 * \return A Spocp connection struct
 */
SPOCP *
spocpc_init(char *hosts, long sec, long usec)
{
    char    **arr, **tmp;
    SPOCP   *spocp;
    int     n;
    octet_t *op;

    if( hosts == 0 || *hosts == '\0' ) 
        return 0;

    spocp = (SPOCP *) calloc(1, sizeof(SPOCP));
    arr = line_split( hosts, ' ', '\\', 0, 0, &n) ;

    for( tmp = arr; *tmp; tmp++) { 
        op = str2oct( *tmp, 1 );
        spocp->srv = octarr_add( spocp->srv, op);
    }
    free(arr);

    spocp->cursrv = 0;

    if ( sec != 0L || usec != 0L) 
        spocpc_set_timeout( spocp, sec, usec);

    return spocp;
}

/*!
 * \brief Adds another spocp server to the list of servers that can be used in
 * this context
 * \param spocp The spocp session to which the server should be added
 * \param host The server to be added
 * \return Pointer to the spocp session struct
 */

SPOCP *
spocpc_add_server( SPOCP *spocp, char *host)
{
    if( host == 0 || *host == '\0' )
        return spocp;

    if (spocp == 0)
        spocp = (SPOCP *) calloc(1, sizeof( SPOCP));

    spocp->srv = octarr_add( spocp->srv, str2oct( Strdup(host), 1));    

    return spocp;
}

/* ---------------------------------------------------------------------- */

static int 
spocpc_round_robin_check( SPOCP *spocp, int cur, int nsec)
{
    int     fd = -1, j;
    octarr_t    *oa;
    char        *srv;

    if ( spocp && spocp->srv && spocp->srv->n > 1) {
        oa = spocp->srv;
        for( j = cur+1; j != cur ; j++) {
            if (j == oa->n) {
                j = 0;
                if( j == cur ) break;
            }

            srv = oa->arr[j]->val;
            if(( fd = spocpc_connect(srv, nsec)) >= 0 ) {
                spocp->cursrv = j;
                break;
            }
        }

        if (j == cur) spocp->cursrv = 0;
    }

    return fd;
}

/*!
 * Opens a new Spocp session. Inlcudes not just creating the session structure but
 * also opening a non-SSL connection to the Spocp server of choice
 * \param spocp A spocp session struct gotten from spocpc_init() or NULL
 * \param srv A spocp server definition, either a host:port specification or a 
 *            filename representing a unix domain socket .
 * \param nsec The number of seconds that should pass before giving up on a server
 * \return A Spocp session pointer or NULL on failure
 */

SPOCP *
spocpc_open(SPOCP * spocp, char *srv, int nsec)
{
    int     fd = 0, i = 0;
    octarr_t    *oa;
    char        *cur;

    if (spocp == 0 && srv == 0) 
        return 0;

    if (srv) {
        spocp = spocpc_add_server( spocp, srv);
        oa = spocp->srv;
        i = oa->n - 1; /* The latest addition */
        fd = spocpc_connect(srv, nsec);
    }
    else {
        spocp->cursrv = 0;
        cur = spocp->srv->arr[0]->val;
        fd = spocpc_connect(cur, nsec);
    }

    /* printf("\nfd: %d\n", fd); */
    
    if (fd < 0 )
        fd = spocpc_round_robin_check( spocp, spocp->cursrv, nsec);

    if (fd < 0)
        return NULL;

    if (spocpc_debug)
        traceLog(LOG_DEBUG,"Connected on %d", fd);

    if (!spocp)
        spocp = (SPOCP *) calloc(1, sizeof(SPOCP));

    spocp->fd = fd;
    spocp->contype = SOCKET;

    return spocp;
}

/*----------------------------------------------------------------------------*/

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
    int     fd;
    char        *cur;
    octarr_t    *oa = spocp->srv;

    close(spocp->fd);   /* necessary if other side has already closed ? */

    cur = oa->arr[spocp->cursrv]->val;  /* Retry the same first */

    fd = spocpc_connect( cur, nsec );
    spocp->contype = SOCKET;

    if (spocp->fd <= 0 )
        spocp->fd = spocpc_round_robin_check( spocp, spocp->cursrv, nsec);

    if (spocp->fd <= 0)
        return FALSE;
    else
        return TRUE;
}

/*----------------------------------------------------------------------------*/

/*!
 * Writes size bytes to a spocp server, Uses SSL/TLS if active.
 * \param spocp The Spocp session
 * \param str A pointer to the buffer containing the bytes to be written
 * \param n The number of bytes to write
 * \return The number of bytes written or -1 if no bytes could be written
 */
ssize_t
spocpc_writen(SPOCP * spocp, char *str, size_t n )
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
        traceLog(LOG_DEBUG,"Writen fd:%d", spocp->fd);

    while (nleft > 0) {
        FD_SET(spocp->fd, &wset);
        retval = select(spocp->fd + 1, NULL, &wset, NULL, &spocp->com_timeout);

        if (retval) {
#ifdef HAVE_SSL
            if (spocp->contype == SSL_TLS)
                nwritten = SSL_write(spocp->ssl, (void *) sp,
                    (int) nleft);
            else
#endif
            {
#ifdef HAVE_SASL
                if(spocp->sasl_ssf && *spocp->sasl_ssf > 0)
                {
                    const char *enc_data;
                    size_t enc_len;
                    sasl_encode(spocp->sasl, sp, nleft, &enc_data, &enc_len);
                    sp = (char*)enc_data;
                    nleft = enc_len;
                }
#endif
                nwritten = write(spocp->fd, sp, nleft);
            }

            if (nwritten <= 0) {;
                if (errno == EINTR)
                    nwritten = 0;
                else
                    return -1;
            }

            nleft -= nwritten;
            sp += nwritten;
        } else {    /* timed out */
            spocp->rc = SPOCPC_TIMEOUT;
            break;
        }
    }

#ifdef HAVE_FSYNC
    fsync(spocp->fd);
#else
#ifdef HAVE_FDATASYNC
    fdatasync(spocp->fd);
#endif
#endif

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
spocpc_readn(SPOCP * spocp, char *buf, size_t size)
{
    fd_set rset;
    int retval;
    ssize_t n = 0;
    struct timeval *tv = &spocp->com_timeout;

    FD_ZERO(&rset);

    spocp->rc = 0;

    FD_SET(spocp->fd, &rset);

#ifdef AVLUS
    traceLog(LOG_DEBUG,"Waiting %ld seconds and %ld milliseconds",
                tv->tv_sec, tv->tv_usec) ;
#endif

    if (tv->tv_sec == 0L && tv->tv_usec == 0L) 
        retval = select(spocp->fd + 1, &rset, NULL, NULL, NULL);
    else
        retval = select(spocp->fd + 1, &rset, NULL, NULL, &spocp->com_timeout);

    if (retval) {
#ifdef HAVE_SSL
        if (spocp->contype == SSL_TLS)
            n = SSL_read(spocp->ssl, (void *) buf, (int) size);
        else
#endif
        {
            n = read(spocp->fd, buf, size);
#ifdef HAVE_SASL
            if(spocp->sasl_ssf && *spocp->sasl_ssf > 0)
            {
                const char *plain_data;
                size_t plain_len;
                sasl_decode(spocp->sasl, buf, n, &plain_data, &plain_len);
                if(plain_len <= size)
                {
                    memcpy(buf, plain_data, plain_len);
                    n = plain_len;
                }
            }
#endif
        }

        if (n <= 0) {
            if (errno == EINTR) {
                spocp->rc = SPOCPC_INTERUPT;
                n = 0;
            }
            else
                return -1;
        }
    } else {        /* timed out */
        spocp->rc = SPOCPC_TIMEOUT;
    }

    return (n);
}

/*----------------------------------------------------------------------------*/

static int
spocpc_get_result_code(octet_t * rc)
{
    int i;

    for (i = 0; spocp_rescode[i].num; i++) {
        if (strncmp(rc->val, spocp_rescode[i].num, 3) == 0)
            return spocp_rescode[i].code;
    }

    return SPOCPC_UNKNOWN_RESCODE;
}

/*----------------------------------------------------------------------------*/

int
spocpc_answer_ok(octet_t *oct, queres_t * qr)
{
    octet_t     code, info, *op;
    octarr_t    *oa ;
    int         res = SPOCPC_TIMEOUT;
    char        ml = 0, *cv;

    if ((oa = spocpc_sexp_elements(oct, &res)) == NULL)
        return res;

    if (spocpc_debug)
        traceLog(LOG_DEBUG,"Got %d elements", oa->n);

    while (oa->n) {
        op = octarr_rpop( oa );
        res = sexp_get_response(op, &code, &info);
        if (res != SPOCPC_OK) {
            if (spocpc_debug)
                traceLog(LOG_DEBUG,"sexp_response returned %d",
                    (int) res);
            return res;
        }

        cv = code.val ;
        if (spocpc_debug) {
            oct_print( LOG_DEBUG, "code", &code );
            oct_print( LOG_DEBUG, "info", &info );
        }

        if ((*cv == '2' || *cv == '3') && *(cv+1) == '0' && *(cv+2) == '1') {
            if( ml == 0) 
                ml = *cv;
            else if (ml != *cv) {
                res = SPOCPC_PROTOCOL_ERROR;
                break;
            }

            qr->blob = octarr_add( qr->blob, octdup(&info));
        }
        else {
            qr->rescode = spocpc_get_result_code(&code);
            qr->str = strndup(info.val, info.len);
        }
    } 
    octarr_free(oa);

    return res;
}

/*----------------------------------------------------------------------------*/

static int
spocpc_protocol_op(octarr_t *oa, octet_t *prot)
{
    char        *op, *sp;
    int     i, l, la, tot;
    unsigned int    n;

    for (i = 0, l = 0; i < oa->n ; i++) {
        if (oa->arr[i]->len == 0)
            continue;
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
    int l, n;

    l = spocpc_protocol_op(argv, &o);

    if (spocpc_debug) {
        char *tmp;
        tmp = oct2strdup( &o, '%');

        traceLog(LOG_DEBUG,"[%s](%d) ->(%d/%d)", tmp, l, spocp->fd,
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
        traceLog(LOG_DEBUG,"Wrote %d chars", n);

    return SPOCPC_OK;
}

/* ========================================================================== */
/* ========================================================================== */

static int
spocpc_get_result(SPOCP * spocp, queres_t * qr)
{
    char    resp[BUFSIZ];
    ssize_t n;
    int     ret=SPOCPC_OK;
    octet_t oct;

    while( !qr->rescode ) {
        if ((n = spocpc_readn(spocp, resp, BUFSIZ)) < 0) {
            traceLog(LOG_DEBUG,"Couldn't read anything");

            if (!spocp->rc)
                return SPOCPC_OTHER;
            else
                return spocp->rc;
        }
        else if (n == 0 && spocp->rc)
            return spocp->rc;

        if (spocpc_debug)
            traceLog(LOG_DEBUG,"Read %d chars", n);

        resp[n] = '\0';

        oct.val = resp ;
        oct.len = n;

        ret = spocpc_answer_ok( &oct, qr );
    }

    if (spocpc_debug)
        traceLog(LOG_DEBUG,"response: [%s] (%d)", resp, ret);

    return ret;
}

/* ============================================================================== */

static int
spocpc_send_X(SPOCP * spocp, octarr_t *argv, queres_t * qr)
{
    int ret;

    if ((ret = spocpc_just_send_X(spocp, argv)) == SPOCPC_OK)
        ret = spocpc_get_result(spocp, qr);

    return ret;
}

/*--------------------------------------------------------------------------------*/


static octarr_t *
argv_make( char *arg1, char *arg2, char *arg3, char *arg4, char *arg5 )
{
    octarr_t    *oa;
    octet_t     *op;
    
    oa = octarr_new(6);

    op = str2oct(arg1, 0);

    octarr_add(oa, op);

    if (arg2 && *arg2)
        op = str2oct(arg2, 0);
    else
        op = oct_new(0, 0);

    octarr_add(oa, op);

    if (arg3 && *arg3)
        op = str2oct(arg3, 0);
    else
        op = oct_new(0, 0);

    octarr_add(oa, op);

    if (arg4 && *arg4)
        op = str2oct(arg4, 0);
    else
        op = oct_new(0, 0);

    octarr_add(oa, op);

    if (arg5 && *arg5)
        op = str2oct(arg5, 0);
    else
        op = oct_new(0, 0);

    octarr_add(oa, op);

    return oa;
}
 
static int
spocpc_oct_send( SPOCP *spocp, char *type, octet_t **oa, int n, queres_t *qr)
{
    octarr_t    *argv;
    octet_t     *op;
    int     ret, i;

    argv = octarr_new(n+1);

    op = str2oct(type,0);
    octarr_add( argv, op);

    for (i = 0; i < n ; i++) {
        if (oa[i])
            op = octdup(oa[i]);
        else
            op = oct_new(0,0);

        octarr_add( argv, op);
    }

    ret = spocpc_send_X(spocp, argv, qr);
    octarr_free( argv );
    return ret;
}

/*--------------------------------------------------------------------------------*/
/*!
 * Send a Spocp query to a spocp server
 * \param spocp The Spocp session
 * \param path The name of the ruleset
 * \param query The string buffer holding the query in the canonical
 * form of a s-expression
 * \param qr A struct into which the result received from the spocp server 
 *   should be placed
 * \return A Spocpc result code
 */
int
spocpc_str_send_query(SPOCP * spocp, char *path, char *query, queres_t * qr)
{
    octarr_t    *argv;
    int     ret;

    if( query == 0 || *query == 0 )
        return SPOCPC_MISSING_ARG;

    argv = argv_make( "QUERY", path, query, 0, 0 );

    ret = spocpc_send_X(spocp, argv, qr);

    octarr_free( argv );
    
    return ret;
}

/*!
 * Send a Spocp query to a spocp server
 * \param spocp The Spocp session
 * \param path The name of the ruleset in octet representation
 * \param query An octet struct holding the query in the form of a s-expression
 * \param qr A struct into which the result received from the spocp server 
 *   should be placed
 * \return A Spocpc result code
 */
int
spocpc_send_query(SPOCP * spocp, octet_t *path, octet_t *query, queres_t * qr)
{
    octet_t *oa[3];

    oa[0] = path;
    oa[1] = query;

    return spocpc_oct_send( spocp, "QUERY", oa, 2, qr); 
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
    octarr_t    *argv;
    int     ret;

    if( rule == 0 || *rule == 0 )
        return SPOCPC_MISSING_ARG;

    argv = argv_make( "DELETE", path, rule, 0, 0 );

    ret = spocpc_send_X(spocp, argv, qr);

    octarr_free( argv );
    
    return ret;
}

/*!
 * Send a Spocp delete operation to a spocp server
 * \param spocp The Spocp session
 * \param path The name of the ruleset in a octet struct
 * \param rule The ruleID that one wants to remove in a octet struct
 * \param qr A struct into which the result received from the spocp server 
 *   should be placed
 * \return A Spocpc result code
 */
int
spocpc_send_delete(SPOCP * spocp, octet_t *path, octet_t *rule, queres_t * qr)
{
    octet_t *oa[3];

    oa[0] = path;
    oa[1] = rule;

    return spocpc_oct_send( spocp, "DELETE", oa, 2, qr); 
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
    octarr_t    *argv;
    int     ret;

    argv = argv_make( "SUBJECT", subject, 0, 0, 0 );

    ret = spocpc_send_X(spocp, argv, qr);

    octarr_free( argv );
    
    return ret;
}

/*----------------------------------------------------------------------------*/

/*!
 * Send a subject definition to a spocp server
 * \param spocp The Spocp session
 * \param subject An octet with info that in some way representing information
 *   about the subject that is controlling this session
 * \param qr A struct into which the result received from the spocp server 
 *   should be placed
 * \return A Spocpc result code
 */
int
spocpc_send_subject(SPOCP * spocp, octet_t *subject, queres_t * qr)
{
    octet_t *oa[3];

    oa[0] = subject;

    return spocpc_oct_send( spocp, "SUBJECT", oa, 1, qr); 
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
    octarr_t    *argv;
    int     ret;

    if( rule == 0 || *rule == 0 )
        return SPOCPC_MISSING_ARG;

    argv = argv_make( "ADD", path, rule, bcond, info );

    ret = spocpc_send_X(spocp, argv, qr);

    octarr_free( argv );
    
    return ret;
}

/*--------------------------------------------------------------------------------*/
/*!
 * Send a rule to a Spocp server
 * \param spocp The Spocp session
 * \param path The name of the ruleset(octet struct)
 * \param rule The string buffer holding the query(octet struct)
 * \param bcond The boundary condition expression(octet struct)
 * \param info The static blob (octet struct)
 * \param qr A struct into which the result received from the spocp server 
 *   should be placed
 * \return A Spocpc result code
 */
int
spocpc_send_add(SPOCP * spocp, octet_t *path, octet_t *rule, octet_t *bcond,
    octet_t *info, queres_t * qr)
{
    octet_t *oa[3];

    if (rule == 0 )
        return SPOCPC_PARAM_ERROR;

    oa[0] = path;
    oa[1] = rule;
    oa[2] = bcond;
    oa[3] = info;

    return spocpc_oct_send( spocp, "DELETE", oa, 4, qr); 
}


/*----------------------------------------------------------------------------*/

int
spocpc_str_send_list(SPOCP * spocp, char *path, queres_t * qr)
{
    octarr_t    *argv;
    int         ret;

    argv = argv_make( "LIST", path, 0, 0, 0 );

    ret = spocpc_send_X(spocp, argv, qr);

    octarr_free( argv );
    
    return ret;
}

/*----------------------------------------------------------------------------*/

int
spocpc_send_list(SPOCP * spocp, octet_t *path, queres_t * qr)
{
    if (path) {
        octet_t *oa[1];

        oa[0] = path;

        return spocpc_oct_send( spocp, "LIST", oa, 1, qr); 
    }
    else {
        return spocpc_oct_send( spocp, "LIST", NULL, 0, qr); 
    }
}

/*----------------------------------------------------------------------------*/

/*!
 * \brief Sends a LOGOUT request to a Spocp server
 * \param spocp The Spocp session
 * \return A spocpc result code
 */
int
spocpc_send_logout(SPOCP * spocp)
{
    queres_t    qres;

    memset( &qres, 0, sizeof( queres_t));

    return spocpc_oct_send( spocp, "LOGOUT", NULL, 0, &qres); 
}

/*----------------------------------------------------------------------------*/

/*!
 * \brief Sends a NOOP to a Spocp server
 * \param spocp The Spocp session
 * \return A spocpc result code
 */

int
spocpc_send_noop(SPOCP * spocp)
{
    queres_t    qres;

    memset( &qres, 0, sizeof( queres_t));

    return spocpc_oct_send( spocp, "NOOP", NULL, 0, &qres); 
}

/*----------------------------------------------------------------------------*/

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
    return spocpc_oct_send( spocp, "BEGIN", NULL, 0, qr); 
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
    return spocpc_oct_send( spocp, "COMMIT", NULL, 0, qr); 
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
    if (spocp->contype == SSL_TLS)
        return SPOCPC_STATE_VIOLATION;

    res = spocpc_oct_send( spocp, "STARTTLS", NULL, 0, qr); 
#else
    res = SPOCPC_NOSUPPORT;
#endif

    return res;
}

/*!
 * \brief Sends a CAPA request to a Spocp server
 * \param spocp The Spocp session
 * \param qr A struct into which the result received from the spocp server 
 *   should be placed
 * \return A spocpc result code
 */
int
spocpc_send_capa(SPOCP * spocp, queres_t * qr)
{
    int res = SPOCPC_OK;
#ifdef HAVE_SASL
    res = spocpc_oct_send ( spocp, "CAPA", NULL, 0, qr);
#else
    res = SPOCPC_NOSUPPORT;
#endif
    return res;
}

/*!
 * \brief Sends a AUTH request to a Spocp server
 * \param spocp The Spocp session
 * \param mech  List of space sepparated mechanisms to try.
 * \param qr    A queres_t struct holding server response.
 * \param callbacks a void casted sasl_callback_t.
 * \param secprops  a void casted sasl_security_properties_t.
 * \return A spocpc result code
 */
int
spocpc_auth(SPOCP * spocp, char * mechs, queres_t *qr, void *callbacks, void *secprops)
{
    int res = SPOCPC_OK;
#ifdef HAVE_SASL
    char *server_name = strdup(spocp->srv->arr[spocp->cursrv]->val);
    char *port = strstr(server_name, ":");
    int res_sa;
    memset(qr, 0, sizeof(queres_t));

    if(spocp->sasl_ssf)
        return(SPOCPC_STATE_VIOLATION);

    if(port)
        *port = '\0';

    sasl_client_init((sasl_callback_t *)callbacks);
    res_sa = sasl_client_new("spocp", server_name, NULL, NULL, NULL, 0, &spocp->sasl);

    if(res_sa == SASL_OK)
    {
        const char *out = NULL;
        char *outcode = NULL;
        sasl_interact_t *interact = NULL;
        unsigned int outlen;
        const char *mechused = NULL;
        octarr_t *argv;
        char *mechsend;
        sasl_security_properties_t tmp_secprops;

        if(!secprops)
        {
            tmp_secprops.min_ssf = 0;
            tmp_secprops.max_ssf = 8192;
            tmp_secprops.maxbufsize = 1024;
            tmp_secprops.property_names = NULL;
            tmp_secprops.property_values = NULL;
            tmp_secprops.security_flags = SASL_SEC_NOANONYMOUS;
            secprops = &tmp_secprops;
        }
        sasl_setprop(spocp->sasl, SASL_SEC_PROPS, (sasl_security_properties_t *)secprops);
        res_sa = sasl_client_start(spocp->sasl, mechs, &interact, &out, &outlen, &mechused);
        mechsend = Malloc(sizeof(char) * strlen(mechused) + 5);
        sprintf(mechsend, "SASL:");
        strcat(mechsend, mechused);
        if(out)
        {
            unsigned outlen2 = 0;
            outcode = Malloc(outlen * 4);
            sasl_encode64(out, outlen, outcode, outlen * 4, &outlen2);
        }

        argv = argv_make("AUTH", mechsend, outcode, 0, 0);
        res = spocpc_send_X(spocp, argv, qr);
        octarr_free(argv);
        if(outcode)
            Free(outcode);
        Free(mechsend);
    }
    while(res_sa == SASL_CONTINUE)
    {
        octarr_t *argv;
        unsigned decodelen = 0;
        char *serverdecode = NULL;
        sasl_interact_t *interact = NULL;
        const char *data_out = NULL;
        char *step_dataout = NULL;
        unsigned data_len = 0;
        if(qr->blob)
        {
            serverdecode = Malloc(qr->blob->arr[0]->len);
            sasl_decode64(qr->blob->arr[0]->val, qr->blob->arr[0]->len, serverdecode, qr->blob->arr[0]->len, &decodelen);
        }
        res_sa = sasl_client_step(spocp->sasl, serverdecode, decodelen, &interact, &data_out, &data_len);
        Free(serverdecode);
        if(data_out)
        {
            unsigned outlen2 = 0;
            step_dataout = Malloc(data_len * 4);
            sasl_encode64(data_out, data_len, step_dataout, data_len * 4, &outlen2);
        }
        argv = argv_make("AUTH", step_dataout, 0, 0, 0);
        memset(qr, 0, sizeof(queres_t));
        res = spocpc_send_X(spocp, argv, qr);
        if(step_dataout)
            Free(step_dataout);
    }

    if(res_sa == SASL_OK)
        sasl_getprop(spocp->sasl, SASL_SSF,
                (const void **) &spocp->sasl_ssf);
    if(res_sa != SASL_OK && res == SPOCPC_OK)
        res = SPOCPC_OTHER;
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
/*
int
spocpc_parse_and_print_list(char *resp, int n, FILE * fp, int wid)
{
    octet_t code, content;
    int l, re, j, sn, ln;
    octet_t elem[64], part[6];
    int rc = SPOCPC_TIMEOUT, res;

    if ((oa = spocpc_sexp_elements(resp, &n, &rc)) == -1)
        return rc;

    if (spocpc_debug)
        traceLog(LOG_DEBUG,"Got %d elements", re);

    if (re == 0)
        return rc;

    for (j = 0; j < oa->n; j++) {

        l = oa->arr[j].len;

        if (spocpc_debug) {
            strncpy(prbuf, elem[j].val, elem[j].len);
            prbuf[elem[j].len] = '\0';
            traceLog(LOG_DEBUG,"Element[%d]: [%s](%d)", j, prbuf,
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
            traceLog(LOG_DEBUG,"code: [%s](%d)", prbuf, code.len);

            strncpy(prbuf, content.val, content.len);
            prbuf[content.len] = '\0';
            traceLog(LOG_DEBUG,"content: [%s](%d)", prbuf, content.len);
        }

        if (strncmp(code.val, "200", 3) == 0) { 
            rc = SPOCPC_OK;
            break;
        } else if (strncmp(code.val, "201", 3) == 0) {  
            if ((sn =
                spocpc_sexp_elements(content.val, content.len,
                    part, 8, &rc)) == -1)
                return rc;

            if (spocpc_debug)
                traceLog(LOG_DEBUG,"%d parts", sn);

            ln = skip_length(&(part[0].val), part[0].len, &rc);

            strncpy(prbuf, part[0].val, ln);
            prbuf[ln] = '\0';
            if (spocpc_debug)
                traceLog(LOG_DEBUG,"path: [%s]", prbuf);
            fwrite(prbuf, 1, ln, fp);
            fwrite(" ", 1, 1, fp);

            if (wid) {

                ln = skip_length(&(part[1].val), part[1].len,
                    &rc);

                if (spocpc_debug)
                    traceLog(LOG_DEBUG,"ruleid length %d", ln);

                strncpy(prbuf, part[1].val, ln);
                prbuf[ln] = '\0';
                if (spocpc_debug)
                    traceLog(LOG_DEBUG,"ruleid: [%s]", prbuf);
                fwrite(prbuf, 1, ln, fp);
                fwrite(" ", 1, 1, fp);
            }

            ln = skip_length(&(part[2].val), part[2].len, &rc);

            strncpy(prbuf, part[2].val, ln);
            prbuf[ln] = '\0';
            if (spocpc_debug)
                traceLog(LOG_DEBUG,"rule: [%s]", prbuf);
            fwrite(prbuf, 1, ln, fp);
            fwrite("\n", 1, 1, fp);
        } else {
            rc = SPOCPC_PROTOCOL_ERROR;
            break;
        }
    }

    return rc;
}
*/

/*----------------------------------------------------------------------------*/

char *spocpc_err2string( int r )
{
    int i ;

    for ( i = 0; spocpc_err2str_map[i].str != NULL; i++ ) {
        if ( i == r )
            return spocpc_err2str_map[i].str;
    }

    return "";
}

void queres_free( queres_t *q)
{
    if( q ){
        free( q->str );
        if (q->blob)
            octarr_free( q->blob );
    }
}
