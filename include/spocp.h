#ifndef __SPOCP_H
#define __SPOCP_H

#include <config.h>

#include <time.h>
#include <sys/time.h>

/*
 * system health levels 
 */
#define  SPOCP_EMERG     0	/* system is unusable */
#define  SPOCP_ALERT     1	/* action must be taken immediately */
#define  SPOCP_CRIT      2	/* critical conditions */
#define  SPOCP_ERR       3	/* error conditions */
#define  SPOCP_WARNING   4	/* warning conditions */
#define  SPOCP_NOTICE    5	/* normal but significant condition */
#define  SPOCP_INFO      6	/* informational */
#define  SPOCP_DEBUG     7	/* debug-level messages */
#define  SPOCP_LEVELMASK 7	/* mask off the level value */

#define  SPOCP_DPARSE   0x10
#define  SPOCP_DSTORE   0x20
#define  SPOCP_DMATCH   0x40
#define  SPOCP_DBCOND   0x80
#define  SPOCP_DSRV    0x100


#ifndef FALSE
#define FALSE          0
#endif
#ifndef TRUE
#define TRUE           1
#endif

/*
 * ---------------------------------------- 
 */

/*
 * result codes 
 */
typedef enum {
	SPOCP_WORKING,
	SPOCP_SUCCESS,
	SPOCP_MULTI,
	SPOCP_DENIED,
	SPOCP_CLOSE,
	SPOCP_EXISTS,
	SPOCP_OPERATIONSERROR,
	SPOCP_PROTOCOLERROR,
	SPOCP_UNKNOWNCOMMAND,
	SPOCP_SYNTAXERROR,
	SPOCP_TIMELIMITEXCEEDED,
	SPOCP_SIZELIMITEXCEEDED,
	SPOCP_AUTHDATA,
	SPOCP_AUTHERR,
	SPOCP_AUTHINPROGRESS,
	SPOCP_BUSY,
	SPOCP_UNAVAILABLE,
	SPOCP_UNWILLING,
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
	SPOCP_STATE_VIOLATION,
	SPOCP_SSL_START,
	SPOCP_SSL_ERR,
	SPOCP_CERT_ERR,
	SPOCP_OTHER,
	SPOCP_UNKNOWN_TYPE,
	SPOCP_UNIMPLEMENTED,
	SPOCP_TRANS_COMP,
	SPOCP_INFO_UNAVAIL,
	SPOCP_AUTH_ERR,
	SPOCP_REDIRECT
	    /*
	     * SPOCP_ALIASPROBLEM , SPOCP_ALIASDEREFERENCINGPROBLEM , 
	     */
} spocp_result_t;

#define SPOCP_LINE_SIZE 4096

#define SPOCP_MAXBUF   262144	/* quite arbitrary */

/*
 * ------------------------------------- 
 */

typedef struct _octet {
	size_t          len;
	size_t          size;
	char           *val;
} octet_t;

typedef struct _octarr {
	int             n;
	int             size;
	octet_t       **arr;
} octarr_t;

typedef struct _keyval {
	octet_t        *key;
	octet_t        *val;
	struct _keyval *next;
} keyval_t;

/*
 * =============================================================================== 
 */

octet_t        *octdup(octet_t * oct);
int             octcmp(octet_t * a, octet_t * b);
void            octmove(octet_t * a, octet_t * b);
int             octncmp(octet_t * a, octet_t * b, size_t i);
int             octstr(octet_t * val, char *needle);
int             octchr(octet_t * o, char c);
int             octpbrk(octet_t * o, octet_t * acc);
octet_t        *oct_new(size_t len, char *val);
octarr_t       *oct_split(octet_t * o, char c, char ec, int flag, int max);
void            oct_free(octet_t * o);
void            oct_freearr(octet_t ** o);
int             oct2strcmp(octet_t * o, char *s);
int             oct2strncmp(octet_t * o, char *s, size_t l);
char           *oct2strdup(octet_t * op, char ec);
int             oct2strcpy(octet_t * op, char *str, size_t len, char ec);

int             octtoi(octet_t * o);

int             oct_de_escape(octet_t * op);
int             oct_find_balancing(octet_t * p, char left, char right);
void            oct_assign(octet_t * o, char *s);

spocp_result_t  oct_resize(octet_t * o, size_t min);
spocp_result_t  octcpy(octet_t * new, octet_t * old);
void            octln(octet_t * to, octet_t * from);
void            octclr(octet_t * o);
spocp_result_t  octcat(octet_t * a, char *s, size_t len);
spocp_result_t  oct_expand(octet_t * src, keyval_t * kvp, int noresize);

octarr_t       *octarr_new(size_t n);
octarr_t       *octarr_add(octarr_t * oa, octet_t * o);
octarr_t       *octarr_dup(octarr_t * oa);
octarr_t       *octarr_join(octarr_t * a, octarr_t * b);
void            octarr_mr(octarr_t * oa, size_t n);
void            octarr_free(octarr_t * oa);
void            octarr_half_free(octarr_t * oa);
octet_t        *octarr_pop(octarr_t * oa);
octet_t        *octarr_rm(octarr_t * oa, int n);

char          **line_split(char *s, char c, char ec, int flag, int max,
			   int *parts);
void            charmatrix_free(char **arr);

void            rmcrlf(char *s);
char           *rmlt(char *s);

char           *safe_strcat(char *dest, char *src, int *size);

char           *find_balancing(char *p, char left, char right);

spocp_result_t  numstr(char *str, long *l);
spocp_result_t  is_date(octet_t * op);
void            to_gmt(octet_t * s, octet_t * t);

/*
 * =============================================================================== 
 */

typedef struct spocp_req_info {
	char           *hostname;
	char           *hostaddr;
	char           *invhost;
	octet_t        *subject;
	char           *sexp_subject;
} spocp_req_info_t;

/*
 * ------------------------------------- 
 */

spocp_result_t  spocp_add_rule(void **vp, octarr_t * oa);
spocp_result_t  spocp_allowed(void *vp, octet_t * sexp, octarr_t ** on);
spocp_result_t  spocp_del_rule(void *vp, octet_t * ids);
spocp_result_t  spocp_list_rules(void *, octarr_t *, octarr_t *, char *);
spocp_result_t  spocp_open_log(char *file, int level);
void            spocp_del_database(void *vp);
void           *spocp_get_rules(void *vp, char *file, int *rc);
void           *spocp_dup(void *vp, spocp_result_t * r);

/*
 * =============================================================================== 
 */

void            FatalError(char *msg, char *s, int i);
void            traceLog(const char *fmt, ...);
void            print_elapsed(char *s, struct timeval start,
			      struct timeval end);
void            timestamp(char *s);

/*
 * =============================================================================== 
 */

int             spocp_loglevel;
int             spocp_debug;

/*
 * =============================================================================== 
 */

#define LOG(x)	if( spocp_loglevel >= (x) )
#define DEBUG(x) if( spocp_loglevel == SPOCP_DEBUG  && ((x) & spocp_debug))

/*
 * =============================================================================== 
 */

#endif
