#ifndef __SPOCP_H
#define __SPOCP_H

#include <config.h>

#include <time.h>
#include <sys/time.h>
#include <pthread.h>

/* system health levels */
#define  SPOCP_EMERG     0       /* system is unusable */
#define  SPOCP_ALERT     1       /* action must be taken immediately */
#define  SPOCP_CRIT      2       /* critical conditions */
#define  SPOCP_ERR       3       /* error conditions */
#define  SPOCP_WARNING   4       /* warning conditions */
#define  SPOCP_NOTICE    5       /* normal but significant condition */
#define  SPOCP_INFO      6       /* informational */
#define  SPOCP_DEBUG     7       /* debug-level messages */
#define  SPOCP_LEVELMASK 7       /* mask off the level value */

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
#define SPOCP_ERR_SYNTAX        1
#define SPOCP_ERR_OPERATION     2
#define SPOCP_ERR_UNKNOWN_ID    3
#define SPOCP_ERR_EXISTS        4
#define SPOCP_ERR_ACCESS        5
#define SPOCP_ERR_MISSING_CHR   6
#define SPOCP_ERR_ARGUMENT      7
#define SPOCP_ERR_BUFOVERFLOW   8
*/

/* result codes */
typedef enum {
  SPOCP_SUCCESS ,
  SPOCP_DENIED ,
  SPOCP_CLOSE ,
  SPOCP_EXISTS ,
  SPOCP_OPERATIONSERROR ,
  SPOCP_PROTOCOLERROR ,
  SPOCP_UNKNOWNCOMMAND ,
  SPOCP_SYNTAXERROR ,
  SPOCP_TIMELIMITEXCEEDED ,
  SPOCP_SIZELIMITEXCEEDED ,
  SPOCP_SASLBINDINPROGRESS ,
  SPOCP_BUSY ,
  SPOCP_UNAVAILABLE ,
  SPOCP_UNWILLINGTOPERFORM ,
  SPOCP_SERVER_DOWN ,
  SPOCP_LOCAL_ERROR ,
  SPOCP_TIMEOUT ,
  SPOCP_FILTER_ERROR ,
  SPOCP_PARAM_ERROR ,
  SPOCP_NO_MEMORY ,
  SPOCP_CONNECT_ERROR ,
  SPOCP_NOT_SUPPORTED ,
  SPOCP_BUF_OVERFLOW ,
  SPOCP_MISSING_CHAR ,
  SPOCP_MISSING_ARG,
  SPOCP_SSLCON_EXIST ,
  SPOCP_SSL_ERR ,
  SPOCP_CERT_ERR ,
  SPOCP_OTHER ,
  SPOCP_UNKNOWN_TYPE
/*
  SPOCP_ALIASPROBLEM ,
  SPOCP_ALIASDEREFERENCINGPROBLEM ,
*/
} spocp_result_t;

#define SPOCP_LINE_SIZE 4096

#define SPOCP_MAXBUF   262144 /* quite arbitrary */ 

/* ------------------------------------- */

typedef struct _octet {
  size_t      len ;
  size_t      size ;
  char        *val ;
} octet_t ;

typedef struct _octarr {
  int n ;
  int size ;
  octet_t **arr ;
} octarr_t ;

typedef struct _octnode {
  octet_t oct ;
  struct _octnode *next ;
} octnode_t ;

typedef struct _keyval {
  octet_t  *key ;
  octet_t  *val ;
  struct _keyval *next ;
} keyval_t ;

octet_t *octdup( octet_t *oct ) ;
int      octcmp( octet_t *a, octet_t *b ) ;
void     octmove( octet_t *a, octet_t *b ) ;
int      octncmp( octet_t *a, octet_t *b, size_t i ) ;
int      octstr( octet_t *val, char *needle ) ;
int      octchr( octet_t *o, char c ) ;
int      octpbrk( octet_t *o, octet_t *acc ) ;
octet_t *oct_new( size_t len, char *val ) ;
octet_t **oct_split(octet_t *o, char c, char ec, int flag, int max, int *parts) ;
void     oct_free( octet_t *o ) ;
void     oct_freearr( octet_t **o ) ; 
int      oct2strcmp(  octet_t *o, char *s ) ;
int      oct2strncmp(  octet_t *o, char *s, size_t l ) ;
char    *oct2strdup( octet_t *op, char ec ) ;
int      oct2strcpy( octet_t *op, char *str, size_t len, int do_escape, char ec ) ;

int      oct_de_escape( octet_t *op ) ;
int      oct_find_balancing(octet_t *p, char left, char right) ;

spocp_result_t oct_resize( octet_t *o, size_t min ) ;
spocp_result_t octcpy( octet_t *new, octet_t *old ) ;
spocp_result_t octcat( octet_t *a, char *s, size_t len ) ;
spocp_result_t oct_expand( octet_t *src, keyval_t *kvp, int noresize ) ;

octarr_t *octarr_new( size_t n ) ;
octarr_t *octarr_add( octarr_t *oa, octet_t *o ) ;
void      octarr_mr( octarr_t *oa, size_t n ) ;
void      octarr_free( octarr_t *oa ) ;

char   **line_split(char *s, char c, char ec, int flag, int max, int *parts) ;
void     charmatrix_free( char **arr ) ;

void     rmcrlf(char *s) ;
char    *rmlt(char *s) ;

char    *safe_strcat( char *dest, char *src, int *size ) ;

char    *find_balancing(char *p, char left, char right) ;

spocp_result_t numstr( char *str, long *l ) ;
spocp_result_t is_date( octet_t *op ) ;
void           to_gmt( octet_t *s, octet_t *t ) ;

/* =============================================================================== */

typedef struct spocp_req_info {
  char *hostname ;
  char *hostaddr ;
  octet_t subject ;
  int  *tls ;
} spocp_req_info_t ;

/* ------------------------------------- */

typedef int (closefn)( void * ) ; 
int P_fclose( void *vp ) ;

#define CON_FREE   0
#define CON_ACTIVE 1

typedef struct _be_con {
  void            *con ;
  int             status ;
  char            *type ;
  char            *arg ;
  closefn         *close ;
  time_t          last ;
  pthread_mutex_t *c_lock ;
  struct _be_con  *next ;
  struct _be_con  *prev ;
} becon_t ;

typedef struct _be_cpool {
  becon_t        *head ;
  becon_t        *tail ;
  size_t          size ;
  size_t          max ;
  pthread_mutex_t *lock ;
} becpool_t ;
  
/* function prototypes */

becon_t   *becon_new( int copy ) ;
becon_t   *becon_dup( becon_t *old ) ;
void       becon_free( becon_t *bc, int close ) ;

becpool_t *becpool_new( size_t max, int copy ) ;
becpool_t *becpool_dup( becpool_t *bcp ) ;
void       becpool_rm( becpool_t *bcp, int close ) ;

becon_t   *becon_push( char *t, char *a, closefn *cf, void *con, becpool_t *b ) ;
becon_t   *becon_get( char *type, char *arg, becpool_t *bcp ) ;
void       becon_return( becon_t *bc ) ;
void       becon_rm( becpool_t *bcp, becon_t *bc ) ;


/* ------------------------------------- */
/* configuration callback function */

typedef spocp_result_t (confgetfn)( void *conf, int arg, char *, void **value ) ;

/* ------------------------------------- */

spocp_result_t spocp_add_rule( void **vp, octet_t **arg, spocp_req_info_t *sri ) ;
spocp_result_t spocp_allowed( void *vp, octet_t *sexp, octnode_t **on, spocp_req_info_t *sri ) ;
spocp_result_t spocp_del_rule( void *vp, octet_t *ids, spocp_req_info_t *sri ) ;
spocp_result_t spocp_list_rules( void *, octet_t *, octarr_t *, spocp_req_info_t *, char * ) ;
spocp_result_t spocp_open_log( char *file, int level ) ;
void           spocp_del_database( void *vp ) ;
void          *spocp_get_rules( void *vp, char *file, int *rc ) ;
void          *spocp_dup( void *vp, spocp_result_t *r ) ;

void       octnode_free( octnode_t *on ) ;
octnode_t *octnode_add( octnode_t *on, octet_t *oct ) ;

/* =============================================================================== */

void FatalError( char *msg, char *s, int i ) ;
void traceLog( const char *fmt, ...) ;
void print_elapsed( char *s, struct timeval start, struct timeval end ) ;
void timestamp( char *s ) ;

/* =============================================================================== */

int spocp_loglevel ;
int spocp_debug ;

/* =============================================================================== */

#define LOG(x)	if( spocp_loglevel >= (x) )
#define DEBUG(x) if( spocp_loglevel == SPOCP_DEBUG  && ((x) & spocp_debug))

/* =============================================================================== */

#endif
