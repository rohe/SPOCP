#ifndef __SPOCPCLI__
#define __SPOCPCLI__

#include <unistd.h>
#include <stdio.h>

#ifdef HAVE_SSL
#include <openssl/ssl.h>
#endif

/*
 * The same as in include/spocp.h, but I don't need the other stuff in that
 * file 
 */

#ifndef __SPOCP_H
typedef struct _octet {
	size_t          len;
	size_t          size;
	char           *val;
} octet_t;

typedef enum {
	SPOCP_SUCCESS,
	SPOCP_DENIED,
	SPOCP_CLOSE,
	SPOCP_EXISTS,
	SPOCP_OPERATIONSERROR,
	SPOCP_PROTOCOLERROR,
	SPOCP_UNKNOWNCOMMAND,
	SPOCP_SYNTAXERROR,
	SPOCP_TIMELIMITEXCEEDED,
	SPOCP_SIZELIMITEXCEEDED,
	SPOCP_SASLBINDINPROGRESS,
	SPOCP_BUSY,
	SPOCP_UNAVAILABLE,
	SPOCP_UNWILLINGTOPERFORM,
	SPOCP_SERVER_DOWN,
	SPOCP_LOCAL_ERROR,
	SPOCP_TIMEOUT,
	SPOCP_FILTER_ERROR,
	SPOCP_PARAM_ERROR,
	SPOCP_NO_MEMORY,
	SPOCP_CONNECT_ERROR,
	SPOCP_NOT_SUPPORTED,
	SPOCP_BUF_OVERFLOW,
	SPOCP_MISSING_CHAR,
	SPOCP_MISSING_ARG,
	SPOCP_SSLCON_EXIST,
	SPOCP_SSL_ERR,
	SPOCP_CERT_ERR,
	SPOCP_OTHER,
	SPOCP_UNKNOWN_TYPE,
	SPOCP_NOT_DONE
} spocp_result_t;
#endif

typedef struct _octnode {
	octet_t        *oct;
	struct _octnode *next;
} octnode_t;

typedef enum {
	UNCONNECTED,
	SOCKET,
	SSL_TLS
} spocp_contype_t;

typedef struct _spocp {
	spocp_contype_t contype;
	int             fd;
	char           *srv;

#ifdef HAVE_SSL
	SSL_CTX        *ctx;
	SSL            *ssl;
#endif

	char           *sslEntropyFile;
	char           *certificate;
	char           *privatekey;
	char           *calist;
	char           *passwd;

} SPOCP;

void            _log_err(const char *format, ...);

char           *oct2strdup(octet_t * op, char ec);
spocp_result_t  octcpy(octet_t * cpy, octet_t * oct);

SPOCP          *spocpc_open(char *srv);
int             spocpc_reopen(SPOCP * spocp);
ssize_t         spocpc_readn(SPOCP * spocp, char *str, ssize_t max);
ssize_t         spocpc_writen(SPOCP * spocp, char *str, ssize_t max);

char           *sexp_printa(char *sexp, int *size, char *fmt, void **argv);

char           *sexplist_make(char *sexp, int bsize, char *fmt, ...);
char           *sexplist_add(char *sexp, int bsize, char *fmt, ...);

/*
 * int sexp_get_len( char **str, spocp_result_t *rc ) ; char
 * *sexp_get_next_element( char *sexp, int n, spocp_result_t *rc ) ; int
 * spocp_protocol_op( char **argv, char **prot ) ; spocp_result_t sexp_memcpy( 
 * octet_t *op, char *str, int n ) ; spocp_result_t spocp_answer_ok( char
 * *resp, size_t n ) ; 
 */
int             spocpc_sexp_elements(char *r, int n, octet_t line[], int s,
				     spocp_result_t * rc);
int             skip_length(char **sexp, int n, spocp_result_t * rc);

spocp_result_t  sexp_get_response(octet_t * buf, octet_t * code,
				  octet_t * info);
spocp_result_t  spocpc_parse_and_print_list(char *resp, int n, FILE * fp,
					    int wid);

spocp_result_t  spocpc_send_add(SPOCP * spocp, char *path, char *rule,
				char *info);
spocp_result_t  spocpc_send_aci(SPOCP * spocp, char *path, char *rule);
spocp_result_t  spocpc_send_subject(SPOCP * spocp, char *subject);
spocp_result_t  spocpc_send_query(SPOCP *, char *path, char *query,
				  octnode_t ** info);
spocp_result_t  spocpc_send_delete(SPOCP * spocp, char *path, char *rule);
spocp_result_t  spocpc_send_logout(SPOCP * spocp);
spocp_result_t  spocpc_open_transaction(SPOCP * spocp);
spocp_result_t  spocpc_commit(SPOCP * spocp);
spocp_result_t  spocpc_starttls(SPOCP * spocp);

void            free_spocp(SPOCP * s);
void            spocpc_close(SPOCP * spocp);

octnode_t      *spocpc_octnode_new(void);
void            spocpc_octnode_free(octnode_t *);
char           *spocpc_oct2strdup(octet_t * op, char ec);

#ifdef HAVE_SSL

void            init_tls_env(SPOCP * spocp, char *cert, char *priv, char *ca,
			     char *pwd);

#endif

int             spocpc_debug;

#define DIGITS(n) ( (n) >= 100000 ? 6 : (n) >= 10000 ? 5 : (n) >= 1000 ? 4 : (n) >= 100 ? 3 : ( (n) >= 10 ? 2 : 1 ) )

#endif
