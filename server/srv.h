#ifndef __SRV_H
#define __SRV_H

#include <config.h>

#include <pthread.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <regex.h>

#ifdef HAVE_LIBSSL
#include <openssl/ssl.h>
#endif

#include <struct.h>
#include <db0.h>
#include <spocp.h>
#include <pthread.h>

/* -------------------------------------- */

#define SPOCP_BUFSIZE  4096

#define SUBTREE  0x04
#define ONELEVEL 0x02
#define BASE     0x01

#define NATIVE   1
#define SOAP     2

#define METHOD_GET    1
#define METHOD_HEAD   2
#define METHOD_POST   3
#define METHOD_PUT    4

/* -------------------------------------- */

#ifndef MAX
#define MAX(a,b) ((a) < (b) ? b : a)
#endif /* MAX */

/* -------------------------------------- */

struct _conn ;
struct _server ;

/* -------------------------------------- */

typedef spocp_result_t (proto_op)( struct _conn * ) ;

/* -------------------------------------- */

typedef struct _spocp_iobuf {
  char    *buf ;
  char    *r ;     /* Where in the buffer reading should start */
  char    *w ;     /* Where in the buffer writing should start */
  char    *end ;    /* end of buffer */
  size_t  bsize ;
  size_t  left ;
} spocp_iobuf_t ;

typedef struct _regexp {
  regex_t      preg ;
  char         *regex ;
} regexp_t ;
  
/*
typedef struct _aci {
  char          access ;   * bitmap *
  char          *string ;
  char          *net ;
  char          *mask ;
  regexp_t      *cert ; 
  struct _aci   *next ;
} aci_t ;
*/

typedef struct _ruleset {
  pthread_rdwr_t   rw_lock;
  pthread_mutex_t  transaction ;
  db_t             *db ;
  char             *name ;
  struct _ruleset  *left ;
  struct _ruleset  *right ;
  struct _ruleset  *down ;
  struct _ruleset  *up ;
} ruleset_t ;

/*
  this is the configuration stuff
  first the server configuration */

typedef struct _hconn {
  char  *referer;
  char  *host ;
  char  *useragent ;
  char  *contenttype ;
  char  *authorization ;
  char  *cookie ;
  char  *soapaction ;
  char  *url ;
  int   method ;
  int   keep_alive ;
  long  contentlength ;
} hconn_t ;

/* and then plugin specific configuration */

typedef struct _other {
  char          *key ;
  strarr_t      *arr ;
  struct _other *next ;
} other_t ;

/* -------------------------------------- */
/* One connection per thread, definitely not optimal in all cases :-)
 */

typedef struct _thread {
  int             id ;
  pthread_t       thread ;
  long            count ; 
  struct _conn   *conn ;
} thread_t ;

typedef struct _server
{
  pthread_mutex_t mutex ;     /* lock on the database */
  ruleset_t       *root ;     /* rule database */

  int             listen_fd ; /* listen socket */
  pthread_mutex_t mlock ;     /* listener lock */
  int             type ;      /* AF_INET, AF_INET6 or AF_LOCAL */  
  int             protocol ;  /* NATIVE, SOAP */
  char            *id ;

  int             threads ;
  thread_t       *worker;

  int             timeout ;
  int             nconn ;
  char            *certificateFile ;
  char            *privateKey ;
  char            *caList ;
  char            *dhFile ;
  char            *SslEntropyFile ;
  char            *passwd ;

  char            *logfile ;
  char            *pidfile ;
  int             port ;
  char            *uds ;
  char            *rulefile ;
  int             sslverifydepth ;
  char            *server_id ;
  int             srvtype ;

  strarr_t        *clients ;

  strarr_t        *ctime ;
  other_t         *other ;

#ifdef HAVE_LIBSSL
  void            *ctx;
  void            *ssl;
                  /* security strength factor, not used presently */
  int             ssf ;
#endif

} srv_t ;

typedef struct _conn {

  int              fd ;
  int              fdw ;      /* for writing if different from reading */
  int              status ;
  int              con_type ;
  int              transaction ;
  int              ops_pending ;
  int              stop ;
  int              tls ;
  time_t           last_event ;

  ruleset_t        *rs ;      /* rule database for this connection */
  spocp_req_info_t sri ;      /* contains hostname/hostaddr */
  struct _server   *srv ;

/* If one thread per connection this isn't needed *
  pthread_mutex_t  lock ;
 */

  spocp_iobuf_t    *in ;
  spocp_iobuf_t    *out ;

  octet_t          oper ;
  octet_t          *oppath ;
  octarr_t         *oparg ;

  hconn_t          http ;

#ifdef HAVE_LIBSSL
  void             *ssl;
  int              ssf ;      /* security strength factor, placeholder */
#endif
  char             *subjectDN ;
  char             *issuerDN ;
  char             *cipher ; 
  char             *ssl_vers ;
  char             *transpsec ;

  int (*readn) (struct _conn *conn, char *buf, size_t len);
  int (*writen)(struct _conn *conn, char *buf, size_t count);
  int (*close) (struct _conn *conn);

} conn_t ;

#define QUERY    0x01
#define LIST     0x02
#define ADD      0x04
#define REMOVE   0x08
#define BEGIN    0x10
#define ALIAS    0x20
#define COPY     0x40

#define ALL     (QUERY|LIST|ADD|REMOVE|BEGIN|ALIAS|COPY) 

#define SPOCP_IOBUFSIZE 4096

/************* functions ********************/

void daemon_init( char *procname, int facility ) ;
int  spocp_stream_socket( int port ) ;
int  spocp_unix_domain_socket( char *uds ) ;

void spocp_server( void *vp ) ;
int  send_results( conn_t *conn ) ;
int  spocp_send_results( conn_t *conn ) ;

spocp_result_t iobuf_resize( spocp_iobuf_t *io, int increase ) ;
spocp_result_t add_to_iobuf( spocp_iobuf_t *io, char *s ) ;
spocp_result_t add_bytestr_to_iobuf( spocp_iobuf_t *io, octet_t *o ) ;
int            spocp_io_write( conn_t *conn ) ;
int            spocp_io_read( conn_t *conn ) ;
void           shift_buffer( spocp_iobuf_t *io ) ;
void           flush_io_buffer( spocp_iobuf_t *io ) ;

char *spocp_next_line( conn_t *conn ) ;

int spocp_writen( conn_t *ct, char *str, size_t n ) ;
int spocp_readn( conn_t *ct, char *str, size_t max ) ;
int spocp_close( conn_t *conn ) ;

conn_t *spocp_open_connection( int fd, srv_t *ss ) ;
void   init_connection( conn_t *conn ) ;
void   clear_buffers( conn_t *con ) ;
conn_t *get_conn( srv_t *srv ) ;
void   free_conn( srv_t *s, conn_t *c ) ;
int    reset_conn( conn_t *c ) ;

void *get_rules( void *vp, char *file, int *rc  ) ;

#ifdef HAVE_LIBSSL
SSL_CTX *tls_init( srv_t *srv ) ;
spocp_result_t tls_start( conn_t *conn, ruleset_t *rs )  ;
#endif

/* config.c */
int            read_config( char *file, srv_t *srv ) ;
spocp_result_t conf_get( void *vp, int arg, char *key, void **res ) ;
int            set_backend_cachetime( srv_t *srv ) ;

/* util.c */

char      *rm_lt_sp( char *s, int t ) ;

/* */

regexp_t  *new_pattern( char *s ) ;
int       match_pattern( regexp_t *regp, char *s ) ;
void      rm_pattern( regexp_t *regp ) ;

/*
int     add_client_aci( char *str, ruleset_t **rs ) ;
int     rm_aci( char *str, ruleset_t *rs ) ;
int     print_aci( conn_t *conn, ruleset_t *rs ) ;
aci_t   *aci_dup( aci_t *old ) ;
*/

spocp_result_t treeList( ruleset_t *rs, conn_t *con, octarr_t *oa, int f ) ;

/* util.c */

octet_t   **join_octarr( octet_t **arr0, octet_t **arr1 ) ;
char      *rm_lt_sp( char *s, int shift ) ;

/* ruleset.c */

void       ruleset_free( ruleset_t *rs ) ;
ruleset_t *ruleset_new( octet_t *name ) ;
ruleset_t *ruleset_create( octet_t *name, ruleset_t **root ) ;
int        ruleset_find( octet_t *name, ruleset_t **rs ) ;

spocp_result_t get_rs_name( octet_t *orig, octet_t *rsn ) ;
spocp_result_t search_in_tree( ruleset_t *, octet_t *, octet_t *, spocp_req_info_t *, int ) ;
spocp_result_t get_pathname( ruleset_t *rs, char *buf, int buflen ) ;

/* ss.c */

spocp_result_t ss_allow( ruleset_t *, octet_t *, octarr_t **, int ) ;
spocp_result_t ss_del_rule( ruleset_t *rs, octet_t *op, int scope ) ;
spocp_result_t ss_add_rule( ruleset_t *rs, octarr_t *oa, bcdef_t *b ) ;
spocp_result_t skip_sexp( octet_t *sexp ) ;

void      ss_del_db( ruleset_t *rs, int scope ) ;
octet_t **ss_list_rules( ruleset_t *rs, octet_t *pattern, int *rc, int scope ) ;
int       ss_rules( ruleset_t *rs, int scope ) ;
void     *ss_dup( ruleset_t *rs, int scope ) ;
void      free_db( db_t *db ) ;

/* n_srv.c */

spocp_result_t com_subject( conn_t *conn ) ;
spocp_result_t com_begin( conn_t *conn ) ;
spocp_result_t com_rollback( conn_t *conn ) ;
spocp_result_t com_commit( conn_t *conn ) ;
spocp_result_t com_login( conn_t *conn ) ;
spocp_result_t com_logout( conn_t *conn ) ;
spocp_result_t com_query( conn_t *conn ) ;
spocp_result_t com_starttls( conn_t *conn ) ;
spocp_result_t com_remove( conn_t *conn ) ;
spocp_result_t com_add( conn_t *conn ) ;
spocp_result_t com_list( conn_t *conn ) ;

spocp_result_t get_operation( conn_t *conn, proto_op **oper ) ;

/* n_run.c */

void spocp_server_run( srv_t *srv ) ;

/* cpool.c */

int init_server( srv_t *srv, char *configfile ) ;
int init_conn( conn_t *con, srv_t *srv, int fd, char *host ) ;

/* pool.c 

pool_item_t *get_item( afpool_t *afp ) ;
pool_item_t *pop_from_pool( pool_t *pool ) ;
pool_item_t *dd_to_pool( pool_t *pool, pool_item_t *item ) ;
pool_item_t *first_active( afpool_t *afp ) ;

int  return_item( afpool_t *afp, pool_item_t *item ) ;
int  afpool_free_item( afpool_t *afp ) ;
int  afpool_active_item( afpool_t *afp ) ;
void add_to_pool( pool_t *p, pool_item_t *pi ) ;
void rm_from_pool( pool_t *pool, pool_item_t *item ) ;

afpool_t *new_afpool( void ) ;

int afpool_lock( afpool_t *afp ) ;
int afpool_unlock( afpool_t *afp ) ;


void free_connection( conn_t *conn ) ;

*/

/* con.c */

char *next_line( conn_t *conn ) ;
void con_reset( conn_t *con ) ;

/* httpd_lite.c */

int hconn_init( hconn_t *hc ) ;
int hconn_reset( hconn_t *hc ) ;

int httpd_get_content( conn_t *c, char **content ) ;
int httpd_parse_request( conn_t *c ) ;
int send_authz_response( conn_t *c, int reqid, int response, octet_t *blob ) ;

/* soap.c */

int soap_server( conn_t *con, spocp_req_info_t *sri, ruleset_t *rs ) ;

/* init.c */

int run_plugin_init( srv_t *srv ) ;

/* saci.c */

void           saci_init( void ) ;
spocp_result_t server_access( conn_t *con ) ;
spocp_result_t operation_access( conn_t *con ) ;

/* -------------------------------------------*/

#ifdef HAVE_LIBSSL

int THREAD_setup( void ) ;
int THREAD_cleanup( void ) ;

#endif 

/* ------------ GLOBALS ------------------ */

int            spocp_err ;
extern srv_t   srv ;
struct utsname myname ;
char           *localcontext ;

/* ssl stuff 
extern char *SslEntropyFile ;
extern int   session_timeout ;
extern char *certificateFile ;
extern char *privateKey ;
extern char *caList ;
extern char *dhFile ;
extern char *passwd ;
extern int   timeout ;

extern int             listenfd ;

extern char *logfile ;
extern int  port ;
extern int  protocol ;
extern char *uds ;
extern char *server_id ;
extern char *rulefile ;
extern int  sslverifydepth ;

extern int  srvtype ;

*/

extern int  allow_serverity ;
extern int  deny_serverity ;

#endif

