/*!
 * \file spocp.h
 * \brief Spocp basic definitions
 */
#ifndef __SPOCP_H
#define __SPOCP_H

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

/*!
 * system health levels 
 */
/*! system is unusable */
#define	SPOCP_EMERG	0	
/*! action must be taken immediately by the administrator */
#define	SPOCP_ALERT	1
/*! critical conditions */
#define	SPOCP_CRIT	2	
/*! error conditions */
#define	SPOCP_ERR	3	
/*! warning conditions */
#define	SPOCP_WARNING	4	
/*! normal but significant condition */
#define	SPOCP_NOTICE	5	
/*! informational */
#define	SPOCP_INFO	6	
/*! debug-level messages */
#define	SPOCP_DEBUG	7	
/*! mask off the level value */
#define	SPOCP_LEVELMASK 7	

/*!
 * System debug levels
 */
/*! Parsing the S-expression */
#define	SPOCP_DPARSE	0x10
/*! Storing the rule in the database */
#define	SPOCP_DSTORE	0x20
/*! Matching a request against the database */
#define	SPOCP_DMATCH	0x40
/*! Boundary conditions */
#define	SPOCP_DBCOND	0x80
/*! Internal server things */
#define	SPOCP_DSRV	0x100


#ifndef FALSE
/*! Basic TRUE/FALSE stuff, should be defined elsewhere */
#define FALSE		0
#endif
#ifndef TRUE
/*! Basic TRUE/FALSE stuff, should be defined elsewhere */
#define TRUE		1
#endif

/*
 * ---------------------------------------- 
 */

/*!
 * result codes 
 */
typedef enum {
	/*! The server is not ready yet but busy with the request */
	SPOCP_WORKING,
	/*! The operation was a success */
	SPOCP_SUCCESS,
	/*! Multiline response */
	SPOCP_MULTI,
	/*! The request was denied, no error */
	SPOCP_DENIED,
	/*! The server will close the connection */
	SPOCP_CLOSE,
	/*! Something that was to be added already existed */
	SPOCP_EXISTS,
	/*! Unspecified error in the Server when performing an operation */
	SPOCP_OPERATIONSERROR,
	/*! The client actions did not follow the protocol specifications */
	SPOCP_PROTOCOLERROR,
	/*! Unknown command */
	SPOCP_UNKNOWNCOMMAND,
	/*! One or more arguments to an operation does not follow the prescribed syntax */
	SPOCP_SYNTAXERROR,
	/*! Time limit exceeded */
	SPOCP_TIMELIMITEXCEEDED
	/*! Size limit exceeded */,
	SPOCP_SIZELIMITEXCEEDED,
	/*! Authentication information */
	SPOCP_AUTHDATA,
	/*! Authentication failed */
	SPOCP_AUTHERR,
	/*! Authentication in progress */
	SPOCP_AUTHINPROGRESS,
	/*! The server is overloaded and would like you to come back later */
	SPOCP_BUSY,
	/*! Occupied, resend the operation later */
	SPOCP_UNAVAILABLE,
	/*! The server is unwilling to perform a operation */
	SPOCP_UNWILLING,
	/*! Server has crashed */
	SPOCP_SERVER_DOWN,
	/*! Local error within the Server */
	SPOCP_LOCAL_ERROR,
	/*! Something did not happen fast enough for the server */
	SPOCP_TIMEOUT,
	/*! The filters to a LIST command was faulty */
	SPOCP_FILTER_ERROR,
	/*! The few or to many arguments to a protocol operation */
	SPOCP_PARAM_ERROR,
	/*! The server is out of memory !! */
	SPOCP_NO_MEMORY,
	/*! Command not supported */
	SPOCP_NOT_SUPPORTED,
	/*! I/O buffer has reached the maximumsize */
	SPOCP_BUF_OVERFLOW,
	/*! What has been received from the client sofar is not enough */
	SPOCP_MISSING_CHAR,
	/*! Missing argument to a operation */
	SPOCP_MISSING_ARG,
	/*! State violation */
	SPOCP_STATE_VIOLATION,
	/*! It's OK to start the SSL negotiation */
	SPOCP_SSL_START,
	/*! Something went wrong during the SSL negotiation */
	SPOCP_SSL_ERR,
	/*! Could not verify a certificate */
	SPOCP_CERT_ERR,
	/*! Something, unspecified what, went wrong */
	SPOCP_OTHER,
	/*! While parsing the S-expression a unknown element was encountered */
	SPOCP_UNKNOWN_TYPE,
	/*! A command or a subset of a command are unimplemented */
	SPOCP_UNIMPLEMENTED,
	/*! No such information, usually a plugin error */
	SPOCP_INFO_UNAVAIL,
	/*! SASL binding in progress */
	SPOCP_SASLBINDINPROGRESS,
	/*! SSL connection alreay exists, so don't try to start another one */
	SPOCP_SSLCON_EXIST,
	/*! Don't talk to me about this, talk to this other guy */
	SPOCP_REDIRECT
	/*
	* SPOCP_ALIASPROBLEM , SPOCP_ALIASDEREFERENCINGPROBLEM , 
	*/
} spocp_result_t;

/*! The maximum I/O buffer size */
#define SPOCP_MAXBUF	262144	/* quite arbitrary */

/*
 * ------------------------------------- 
 */

/*! \brief The really basic struct, where byte arrays are kept */
typedef struct {
	/*! The size of the byte array */
	size_t	len;
	/*! The size of the memory area allocated */
	size_t	size;
	/*! Pointer to the byte array, these are not string hence not NULL terminated */
	char	*val;
} octet_t;

/*! \brief A struct representing a array of byte arrays */
typedef struct {
	/*! The current number of byte arrays */
	int	n;
	/*! The maximum number of byte arrays that can be assigned */
	int	size;
	/*! The array of byte arrays, not NULL terminated */
	octet_t	**arr;
} octarr_t;

/*
 * --------------------------------------------------------------------------- 
 */

/*! \brief Information about the operation requestor, not just information about the
 *	client but also about the subject using the client.
 */
typedef struct {
	/*! FQDN of the client */
	char	*hostname;
	/*! IP address of the client */
	char	*hostaddr;
	/*! inverted representation of the clients FQDN */
	char	*invhost;
	/*! Information about the subject using the client, normally in the form
 	 *  of a S-expression */
	octet_t	*subject;
	/*! This is just a string representation of the subjecct */
	char	*sexp_subject;
} spocp_req_info_t;

/*! \brief information about a piece of a S-expression, used for parsing */
typedef struct _chunk {
	/*! The piece */
	octet_t        *val;
	/*! Next chunk */
	struct _chunk  *next;
	/*! Previouos chunk */
	struct _chunk  *prev;
} spocp_chunk_t;

/*! \brief Input buffer used by the S-expression parsing functions */
typedef struct {
	/*! Pointer to the string to parse */
	char           *str;
	/*! Where parsing should start */
	char           *start;
	/*! The size of the memory area where the string is stored */
	int             size;
	/*! Pointer to a file descriptor where more bytes can be gotten */
	FILE           *fp;
} spocp_charbuf_t;

/*
 * =============================================================================== 
 */

octet_t		*octdup(octet_t * oct);
int		octcmp(octet_t * a, octet_t * b);
int		octrcmp(octet_t * a, octet_t * b);
void		octmove(octet_t * a, octet_t * b);
int		octncmp(octet_t * a, octet_t * b, size_t i);
int		octstr(octet_t * val, char *needle);
int		octchr(octet_t * o, char c);
int		octpbrk(octet_t * o, octet_t * acc);
octet_t		*oct_new(size_t len, char *val);
octarr_t	*oct_split(octet_t * o, char c, char ec, int flag, int max);
void		oct_free(octet_t * o);
void		oct_freearr(octet_t ** o);
int		oct2strcmp(octet_t * o, char *s);
int		oct2strncmp(octet_t * o, char *s, size_t l);
char		*oct2strdup(octet_t * op, char ec);
int		oct2strcpy(octet_t * op, char *str, size_t len, char ec);

octet_t		*str2oct( char *, int );
int		octtoi(octet_t * o);

int		oct_de_escape(octet_t * op);
int		oct_find_balancing(octet_t * p, char left, char right);
void		oct_assign(octet_t * o, char *s);

spocp_result_t	oct_resize(octet_t *, size_t);
spocp_result_t	octcpy(octet_t *, octet_t *);
int		octln(octet_t *, octet_t *);
octet_t 	*octcln(octet_t *);
void		octclr(octet_t *);
spocp_result_t	octcat(octet_t *, char *, size_t);

octarr_t	*octarr_new(size_t n);
octarr_t	*octarr_add(octarr_t * oa, octet_t * o);
octarr_t	*octarr_dup(octarr_t * oa);
octarr_t	*octarr_join(octarr_t * a, octarr_t * b);
void		octarr_mr(octarr_t * oa, size_t n);
void		octarr_free(octarr_t * oa);
void		octarr_half_free(octarr_t * oa);
octet_t		*octarr_pop(octarr_t * oa);
octet_t		*octarr_rm(octarr_t * oa, int n);

char		**line_split(char *s, char c, char ec, int flag, int max,
				int *parts);

void		rmcrlf(char *s);
char		*rmlt(char *s);
char		*find_balancing(char *p, char left, char right);
void		charmatrix_free( char **m );
/*
 * ===============================================================================
 */

int		get_len(octet_t * oct, spocp_result_t *rc);
spocp_result_t	get_str(octet_t * oct, octet_t * str);
int		sexp_len(octet_t * sexp);

char		*sexp_printa(char *sexp, unsigned int *bsize, char *fmt,
				void **argv);
char		*sexp_printv(char *sexp, unsigned int *bsize, char *fmt, ...);

/*
 * =============================================================================== 
 */

void		FatalError(char *msg, char *s, int i);
/*! defined in lib/log.c */
void		traceLog(int priority, const char *fmt, ...);
void		print_elapsed(char *s, struct timeval start,
				struct timeval end);
void		timestamp(char *s);

/*
 * =============================================================================== 
 */

spocp_charbuf_t *charbuf_new( FILE *fp, size_t size );
void		charbuf_free( spocp_charbuf_t * );

void		chunk_free(spocp_chunk_t *);
char		charbuf_peek( spocp_charbuf_t *cb );
char		*get_more(spocp_charbuf_t *);
octet_t		*chunk2sexp(spocp_chunk_t *);
spocp_chunk_t	*get_sexp( spocp_charbuf_t *ib, spocp_chunk_t *pp );
spocp_chunk_t	*get_chunk(spocp_charbuf_t * io);
spocp_chunk_t	*chunk_add( spocp_chunk_t *pp, spocp_chunk_t *np );

spocp_chunk_t	*get_sexp_from_str(char *);
octet_t		*sexp_normalize(char *s);

/*
 * =============================================================================== 
 */

/*! The curremt log level */
extern int	spocp_loglevel;
/*! The debug level */ 
extern int	spocp_debug;
/* whether syslog is used or not */
extern int	log_syslog;

/*
 * =============================================================================== 
 */

/*! A macro that tests whether the log level is high enough to cause loggin */
#define LOG(x)	if( spocp_loglevel >= (x) )
/*! A macro that tests whether the debug level is high enough to cause loggin */
#define DEBUG(x) if( spocp_loglevel == SPOCP_DEBUG && ((x) & spocp_debug))

/*
 * =============================================================================== 
 */

#endif
