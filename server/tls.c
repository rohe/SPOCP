
/***************************************************************************
                          tls.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Ume� University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include "locl.h"

#if (defined(HAVE_SSL) || defined(HAVE_SASL))

#include <openssl/lhash.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

/*
 * thread stuff for openSSL, demands posix threads 
 */

#define MUTEX_TYPE       pthread_mutex_t
#define MUTEX_SETUP(x)   pthread_mutex_init( &(x), NULL )
#define MUTEX_CLEANUP(x) pthread_mutex_destroy(&(x))
#define MUTEX_LOCK(x)    pthread_mutex_lock(&(x))
#define MUTEX_UNLOCK(x)  pthread_mutex_unlock(&(x))
#define THREAD_ID        pthread_self()

static MUTEX_TYPE *mutex_buf = NULL;

/*
 * Structure for collecting random data for seeding. 
 */

typedef struct randstuff {
	time_t          t;
	pid_t           p;
} randstuff_t;

typedef struct _sslsockdata {
	SSL_CTX        *ctx;
	SSL            *ssl;
} sslsockdata_t;

/*
 * Local static variables 
 */

static char     ssl_errstring[256];

#define CIPHER_LIST "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"

/*
 * static const unsigned char *sid_ctx = "spocp"; static unsigned char
 * *ssl_xfer_buffer = NULL; static int ssl_xfer_buffer_size = 4096; static int 
 * ssl_xfer_buffer_lwm = 0; static int ssl_xfer_buffer_hwm = 0; static int
 * ssl_xfer_eof = 0; static int ssl_xfer_error = 0; 
 */

/*
 * static int tls_error( int level, conn_t *conn, char *msg ) ; 
 */
static int      generate_eph_rsa_key(SSL_CTX * ctx, int keylength);
static int      init_dh(SSL_CTX * ctx, unsigned char *dhparam);
static int      add_entropy(const char *file);
static int      check_cert_chain(conn_t * conn, SSL * ssl, ruleset_t * rs);

/*
 * SSL_CTX *tls_init( char *certificate, char *privatekey, char *calist ) ; 
 */
spocp_result_t  tls_start(conn_t * conn, ruleset_t * rs);

/************************************************
************************************************/

static void
locking_function(int mode, int n, const char *file, int line)
{
	if (mode & CRYPTO_LOCK)
		MUTEX_LOCK(mutex_buf[n]);
	else
		MUTEX_UNLOCK(mutex_buf[n]);
}

/*
static unsigned long
id_function(void)
{
	return (unsigned long int) THREAD_ID;
}
*/

int
THREAD_setup(void)
{
	int             i;

	mutex_buf =
	    (MUTEX_TYPE *) Malloc(CRYPTO_num_locks() * sizeof(MUTEX_TYPE));
	if (!mutex_buf)
		return 0;

	for (i = 0; i < CRYPTO_num_locks(); i++)
		MUTEX_SETUP(mutex_buf[i]);

	CRYPTO_set_id_callback(NULL);
	CRYPTO_set_locking_callback(locking_function);

	return 1;
}

int
THREAD_cleanup(void)
{
	int             i;

	if (!mutex_buf)
		return 0;

	CRYPTO_set_id_callback(NULL);
	CRYPTO_set_locking_callback(NULL);

	for (i = 0; i < CRYPTO_num_locks(); i++)
		MUTEX_CLEANUP(mutex_buf[i]);

	free(mutex_buf);
	mutex_buf = NULL;

	return 1;
}

/************************************************
************************************************/

static int
verify_callback_ok(int preverify_ok, X509_STORE_CTX * x509ctx)
{
	return 1;
}

static int
verify_callback(int preverify_ok, X509_STORE_CTX * x509ctx)
{
	X509_NAME      *xn;
	static char     txt[256];

	xn = X509_get_subject_name(x509ctx->current_cert);
	X509_NAME_oneline(xn, txt, sizeof(txt));

	if (preverify_ok == 0) {
		LOG(SPOCP_ERR)
		    traceLog(LOG_ERR,"SSL verify error: depth=%d error=%s cert=%s",
			     x509ctx->error_depth,
			     X509_verify_cert_error_string(x509ctx->error),
			     txt);
		return 0;
	}

	/*
	 * the check against allowed clients are done elsewhere 
	 */

	LOG(SPOCP_DEBUG) traceLog(LOG_DEBUG,"SSL authenticated peer: %s\n", txt);

	return 1;		/* accept */
}

/************************************************
************************************************/

static int
tls_error(int level, conn_t * conn, char *msg)
{
	char            errorstr[1024];
	unsigned long   e;

	e = ERR_get_error();
	ERR_error_string_n(e, errorstr, 1024);

	LOG(level) traceLog(LOG_ERR,"TLS error on connection from %s (%s): %s",
			    conn->sri.hostname, conn->sri.hostaddr, msg);
	LOG(level) traceLog(LOG_ERR,"\"%s\"", errorstr);

	return FALSE;
}

/*****************************************************************************/

/*****************************************************************************/

static int
password_cb(char *buf, int num, int rwflag, void *userdata)
{
	char	*passwd = (char *) userdata;
	int	len;

	len = (int) strlen(passwd);
	if (num < len + 1) {
		LOG(SPOCP_ERR)
		    traceLog(LOG_ERR,"Not big enough place for the password (%d)",
			     num);
		return (0);
	}

	strcpy(buf, passwd);
	return (len);
}

/*************************************************
*        Callback to generate RSA key            *
*************************************************/

/*
 * Arguments: s SSL connection export not used keylength keylength
 * 
 * Returns: pointer to generated key 
 */

static int
generate_eph_rsa_key(SSL_CTX * ctx, int keylength)
{
	RSA            *rsa_key;

	LOG(SPOCP_DEBUG) traceLog(LOG_DEBUG,"Generating %d bit RSA key...\n", keylength);

	rsa_key = RSA_generate_key(keylength, RSA_F4, NULL, NULL);

	if (!SSL_CTX_set_tmp_rsa(ctx, rsa_key)) {
		LOG(SPOCP_ERR) traceLog(LOG_ERR,"TLS error (RSA_generate_key): %s",
					ssl_errstring);
		return FALSE;
	}

	RSA_free(rsa_key);

	return TRUE;
}

/*************************************************
*                Initialize for DH               *
*************************************************/

/*
 * If dhparam is set, expand it, and load up the parameters for DH encryption.
 * 
 * Arguments: dhparam DH parameter file
 * 
 * Returns: TRUE if OK (nothing to set up, or setup worked) 
 */

static int
init_dh(SSL_CTX * ctx, unsigned char *dhparam)
{
	int             rc = TRUE;
	BIO            *bio = 0;
	DH             *dh = 0;

	if (dhparam == NULL)
		return TRUE;

	if ((bio = BIO_new_file((char *) dhparam, "r")) == NULL) {
		LOG(SPOCP_ERR)
		    traceLog(LOG_ERR,"DH: could not read %s: %s", dhparam,
			     strerror(errno));
		rc = FALSE;
	} else {

		if ((dh =
		     PEM_read_bio_DHparams(bio, NULL, NULL, NULL)) == NULL) {
			LOG(SPOCP_ERR)
			    traceLog(LOG_ERR,"DH: could not load params from %s",
				     dhparam);
			rc = FALSE;
		} else {
			if (SSL_CTX_set_tmp_dh(ctx, dh) < 0) {
				LOG(SPOCP_DEBUG)
					traceLog(LOG_ERR,
					    "Couldn't set Diffie-Hellman parameters");
				rc = FALSE;
			} else
				LOG(SPOCP_DEBUG)
				    traceLog(LOG_ERR,
					"Diffie-Hellman initialised from %s with %d-bit key",
				    	 dhparam, 8 * DH_size(dh));

			DH_free(dh);
		}

		BIO_free(bio);
	}

	return rc;
}


/************************************************
************************************************/

static int
add_entropy(const char *file)
{
	struct stat     st;
	int             n = -1;

	if (!file)
		return 0;

	if (stat(file, &st) == -1)
		return errno == ENOENT ? 0 : -1;

	/*
	 * check that the file permissions are secure 
	 */
	if (st.st_uid != getuid() ||
	    ((st.st_mode & (S_IWGRP | S_IRGRP)) != 0) ||
	    ((st.st_mode & (S_IWOTH | S_IROTH)) != 0)) {
		LOG(SPOCP_ERR) traceLog(LOG_ERR,"TLS: %s has insecure permissions!",
					file);
		return -1;
	}
#ifdef HAVE_RAND_EGD
	n = RAND_egd(file);
#endif

	if (n <= 0)
		n = RAND_load_file(file, -1);

	return n;
}

/************************************************
************************************************/

static int
ssl_socket_close(conn_t * conn)
{
	int             ret;
	SSL            *ssl = (SSL *) conn->ssl;

	if (ssl) {
		/*
		 * due to the bidirectional shutdown 
		 */
		if(((ret = SSL_shutdown(ssl)) == 0) && (SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN))
			ret = SSL_shutdown(ssl);

		if (ret < 0) {
			switch (SSL_get_error(ssl, ret)) {
			case SSL_ERROR_ZERO_RETURN:	/* Ah, well */
				break;

			case SSL_ERROR_SYSCALL:
			case SSL_ERROR_SSL:
				break;
			}
		}

		SSL_free(ssl);
		conn->ssl = 0;
	}

	return close(conn->fd);
}

/************************************************
************************************************/

static int
ssl_socket_readn(conn_t * conn, char *buf, size_t len)
{
	return SSL_read((SSL *) conn->ssl, buf, len);
}

/************************************************
************************************************/

static int
ssl_socket_writen(conn_t * conn, char *buf, size_t len)
{
	return SSL_write((SSL *) conn->ssl, buf, len);
}

/************************************************
************************************************/

static int
tls_close(conn_t * conn)
{
	int             rc;

	rc = ssl_socket_close(conn);

	/*
	 * reset the read/write pointers 
	 */

	conn->readn = conn_readn;
	conn->writen = conn_writen;
	conn->close = conn_close;

	return rc;
}

/************************************************
************************************************/

SSL_CTX        *
tls_init(srv_t * srv)
{
	char            path[_POSIX_PATH_MAX];
	SSL_CTX        *ctx;
	int             r;
	char            errorstr[1024];
	unsigned long   e;

	SSL_library_init();
	SSL_load_error_strings();	/* basic set up */
	OpenSSL_add_ssl_algorithms();

	/*
	 * Create a TLS context 
	 */

	if (!(ctx = SSL_CTX_new(SSLv23_method()))) {
		LOG(SPOCP_ERR) traceLog(LOG_ERR,"Error allocation SSL_CTX");
		return 0;
	}

	LOG(SPOCP_DEBUG) traceLog(LOG_DEBUG,"Do we have enough randomness ??");

	if (!RAND_status()) {
		/*
		 * load entropy from files 
		 */
		add_entropy(srv->SslEntropyFile);
		add_entropy(RAND_file_name(path, sizeof(path)));

		/*
		 * load entropy from egd sockets 
		 */
#ifdef HAVE_RAND_EGD
		add_entropy(getenv("EGDSOCKET"));
		snprintf(path, sizeof(path), "%s/.entropy", NONULL(Homedir));
		add_entropy(path);
		add_entropy("/tmp/entropy");
#endif

		/*
		 * shuffle $RANDFILE (or ~/.rnd if unset) 
		 */
		RAND_write_file(RAND_file_name(path, sizeof(path)));
		if (!RAND_status()) {
			LOG(SPOCP_ERR)
				traceLog(LOG_ERR,
				    "Failed to find enough entropy on your system");
			return 0;
		}
	}

	/*
	 * Initialize with DH parameters if supplied 
	 */

	if (srv->dhFile) {
		if (init_dh(ctx, (unsigned char *) srv->dhFile) == FALSE) {
			LOG(SPOCP_ERR) traceLog(LOG_ERR,"Error initializing DH");
			SSL_CTX_free(ctx);
			return 0;
		} else
			LOG(SPOCP_ERR) traceLog(LOG_ERR,"Initializing DH OK");
	}

	/*
	 * and a RSA key too 
	 */

	if (generate_eph_rsa_key(ctx, 512) == FALSE) {
		LOG(SPOCP_ERR) traceLog(LOG_ERR,"Error initializing RSA key");
		SSL_CTX_free(ctx);
		return 0;
	} else
		LOG(SPOCP_ERR) traceLog(LOG_ERR,"Initializing RSA key OK");

	/*
	 * Set up certificates and keys 
	 */

	if (srv->certificateFile != NULL) {
		LOG(SPOCP_INFO) traceLog(LOG_INFO,"Reading Certificate File");
		if (!SSL_CTX_use_certificate_chain_file
		    (ctx, srv->certificateFile)) {
			LOG(SPOCP_ERR)
			    traceLog(LOG_ERR,"Error in SSL_CTX_use_certificate_file");
			SSL_CTX_free(ctx);
			return 0;
		}
	}

	if (srv->privateKey != NULL) {

		if (srv->passwd) {
			SSL_CTX_set_default_passwd_cb_userdata(ctx,
							       (void *) srv->
							       passwd);
			SSL_CTX_set_default_passwd_cb(ctx, password_cb);
		}

		LOG(SPOCP_INFO) traceLog(LOG_INFO,"Reading Private Key File");
		r = SSL_CTX_use_PrivateKey_file(ctx, srv->privateKey,
						SSL_FILETYPE_PEM);
		if (r == 0) {
			e = ERR_get_error();
			ERR_error_string_n(e, errorstr, 1024);

			LOG(SPOCP_ERR)
			    traceLog(LOG_ERR,"Error in SSL_CTX_use_PrivateKey_file");
			LOG(SPOCP_ERR) traceLog(LOG_ERR,"%s", errorstr);

			SSL_CTX_free(ctx);
			return 0;
		}
	}

	if (srv->caList != NULL) {
		LOG(SPOCP_INFO) traceLog(LOG_INFO,"Reading Trusted CAs File");
		if (!SSL_CTX_load_verify_locations(ctx, srv->caList, 0)) {
			LOG(SPOCP_ERR)
			    traceLog(LOG_ERR,"Error in SSL_CTX_load_verify_locations");
			SSL_CTX_free(ctx);
			return 0;
		}
	}

	if (srv->clientcert == NONE)
		SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, verify_callback_ok);
	else {
		int             i = SSL_VERIFY_PEER;
		if (srv->clientcert == HARD)
			i |= SSL_VERIFY_CLIENT_ONCE;

		SSL_CTX_set_verify(ctx, i, verify_callback);
	}

	SSL_CTX_set_verify_depth(ctx, srv->sslverifydepth);

	SSL_CTX_set_options(ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2);

	if (SSL_CTX_set_cipher_list(ctx, CIPHER_LIST) != 1) {
		LOG(SPOCP_ERR) traceLog(LOG_ERR,"No valid ciphers in cipherlist");
		SSL_CTX_free(ctx);
		return 0;
	}

	LOG(SPOCP_DEBUG) traceLog(LOG_DEBUG,"Initialised TLS");

	return ctx;
}

/************************************************
************************************************/

/*
 * Check that the common name matches the host name
 */
static int
check_cert_chain(conn_t * conn, SSL * ssl, ruleset_t * rs)
{
	X509           *peer;
	X509_NAME      *xn;
	static char     subject[1024];
	int             r = FALSE, extc;

	if (SSL_get_verify_result(ssl) != X509_V_OK) {
		LOG(SPOCP_ERR) traceLog(LOG_ERR,"Certificate doesn't verify");
		return FALSE;
	}

	/*
	 * Check the cert chain. The chain length is automatically checked by
	 * OpenSSL when we set the verify depth in the ctx 
	 */

	peer = SSL_get_peer_certificate(ssl);
	if (!peer) {
		LOG(SPOCP_ERR) traceLog(LOG_ERR,"No peer certificate");
		return TRUE;
	}

	/*
	 * check subjectaltname 
	 */

	if ((extc = X509_get_ext_count(peer)) > 0) {
		int             i;

		for (i = 0; r == FALSE && i < extc; i++) {
			X509_EXTENSION *ext;
			const char     *extstr;

			ext = X509_get_ext(peer, i);
			extstr =
			    OBJ_nid2sn(OBJ_obj2nid
				       (X509_EXTENSION_get_object(ext)));

			if (strcmp(extstr, "subjectAltName") == 0) {
				int             j;
				unsigned char  *data;
				STACK_OF(CONF_VALUE) * val;
				CONF_VALUE     *nval;
				X509V3_EXT_METHOD *meth;

				if ((meth = X509V3_EXT_get(ext)) == 0)
					break;

				data = ext->value->data;

				val =
				    meth->i2v(meth,
					      meth->d2i(NULL, &data,
							ext->value->length),
					      NULL);

				for (j = 0;
				     r == FALSE && i < sk_CONF_VALUE_num(val);
				     j++) {
					nval = sk_CONF_VALUE_value(val, j);
					if (strcasecmp(nval->name, "DNS") == 0
					    && strcasecmp(nval->value,
							  conn->sri.
							  hostname)) {
						r = TRUE;
					}
				}
			}
		}
	}

	if (r == FALSE) {
		/*
		 * Check the subject name 
		 */
		xn = X509_get_subject_name(peer);
		X509_NAME_get_text_by_NID(xn, NID_commonName, subject, 1024);
		subject[1023] = '\0';

		traceLog(LOG_DEBUG,"\"%s\" = \"%s\" ?", subject, conn->sri.hostname);
		if (strcasecmp(subject, conn->sri.hostname) == 0) {
			r = TRUE;
		}
	}

	if (r == TRUE) {
		conn->subjectDN =
		    X509_NAME_oneline(X509_get_subject_name(peer), NULL, 0);
		conn->issuerDN =
		    X509_NAME_oneline(X509_get_issuer_name(peer), NULL, 0);
	}

	X509_free(peer);

	return r;
}

/************************************************
************************************************/

spocp_result_t
tls_start(conn_t * conn, ruleset_t * rs)
{
	SSL            *ssl;
	SSL_CTX        *ctx = (SSL_CTX *) conn->srv->ctx;
	int             maxbits, r, n = 0;
	char           *sid_ctx = "spocp";
	SSL_CIPHER     *cipher;

	if (conn->ssl != NULL) {
		tls_error(SPOCP_WARNING, conn,
			  "STARTTLS received on already encrypted connection");
		return SPOCP_STATE_VIOLATION;
	}

	if (!(ssl = SSL_new(ctx))) {
		tls_error(SPOCP_ERR, conn, "Error creating SSL context");
		return SPOCP_OPERATIONSERROR;
	}

	/*
	 * do these never fail ?? 
	 */

	SSL_set_session_id_context(ssl, (unsigned char *) sid_ctx,
				   strlen(sid_ctx));

	if (SSL_set_fd(ssl, conn->fd) == 0) {
		traceLog(LOG_ERR,"Couldn't set filedescriptor in SSL");
		return SPOCP_OPERATIONSERROR;
	}

	n = iobuf_content(conn->in);
	traceLog(LOG_INFO,"tls_start: %d bytes in input buffer", n);
	if (n) {
		traceLog(LOG_INFO,"tls_start: %x%x%x%x", conn->in->r[0], conn->in->r[1],
			 conn->in->r[2], conn->in->r[3]);
	}

	LOG(SPOCP_DEBUG)
	    traceLog(LOG_DEBUG,"Waiting for client on %d to initiate handshake",
		     conn->fd);

	/*
	 * waits for the client to initiate the handshake 
	 */
	{
		fd_set rset ; int retval ;
	  
		FD_ZERO( &rset );
		FD_SET( conn->fd, &rset );
		traceLog(LOG_DEBUG, "Waiting for the client" ) ;
		retval = select(conn->fd+1,&rset,NULL,NULL,0) ;
	}
	if ((r = SSL_accept(ssl)) <= 0) {
		int se ;

		if ((se = SSL_get_error(ssl, r)) == SSL_ERROR_WANT_READ) {
			traceLog(LOG_DEBUG,"Want_read");
		} else if (se == SSL_ERROR_SYSCALL) {
			unsigned long err ;

			err = ERR_get_error();
			if( err == 0L && r == 0 ) {
				traceLog(LOG_DEBUG,"EOF observed") ;
			}
			else 
				traceLog(LOG_ERR,"I/O error occured (%ld/%d)", err, r);
		} else {
			traceLog(LOG_ERR,"SSL_get_error: %d", se);
			tls_error(SPOCP_ERR, conn, "SSL accept error");
			SSL_free(ssl);
		}
		conn->status = CNST_ACTIVE;
		return SPOCP_SSL_ERR;
	}
	/*
	 * } 
	 */

	LOG(SPOCP_DEBUG) {
		traceLog(LOG_DEBUG,"SSL accept done");
		traceLog(LOG_DEBUG,"Checking client certificate");
	 }

	if (!check_cert_chain(conn, ssl, rs)) {
		traceLog(LOG_ERR,"Certificate chain check failed");
		SSL_free(ssl);
		conn->status = CNST_ACTIVE;
		return SPOCP_CERT_ERR;
	}

	/*
	 * So the cert is OK and the hostname is in the DN, but do I want to
	 * talk to this guy ?? 
	 */

	cipher = SSL_get_current_cipher(ssl);

	conn->cipher = Strdup((char *) SSL_CIPHER_get_name(cipher));

	conn->ssl_vers = Strdup(SSL_CIPHER_get_version(cipher));

	if (server_access(conn) == 0) {
		traceLog(LOG_ERR,"Client not allowed access");
		SSL_free(ssl);

		conn->status = CNST_ACTIVE;
		return SPOCP_CERT_ERR;
	}

	LOG(SPOCP_DEBUG) traceLog(LOG_DEBUG,"SSL accept done");

	/*
	 * TLS has been set up. Change input/output to read via TLS instead 
	 */

	conn->readn = ssl_socket_readn;
	conn->writen = ssl_socket_writen;
	conn->close = tls_close;

	conn->ssl = (void *) ssl;
	conn->tls_ssf = SSL_CIPHER_get_bits(cipher, &maxbits);
	conn->status = CNST_ACTIVE;

	return SPOCP_SUCCESS;
}
#else
int             spocp_server_tls_unused;
#endif
