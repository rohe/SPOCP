#ifndef __SRV_H
#define __SRV_H

#include <plugin.h>

/* -------------------------------------- */

#define SPOCP_BUFSIZE	4096

#define SUBTREE		0x04
#define ONELEVEL	0x02
#define BASE		0x01

#define ALL		0x10

#define NATIVE	1
#define SOAP	2

#ifdef NAME_MAX
#define MAXNAMLEN    NAME_MAX
#else
#define MAXNAMLEN    255
#endif

#ifndef NI_MAXHOST
#define NI_MAXHOST	1025
#endif

#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST	1
#endif

/* -------------------------------------- */

#ifndef MAX
#define MAX(a,b) ((a) < (b) ? b : a)
#endif /* MAX */

/* -------------------------------------- */

struct _conn ;
struct _server ;
struct _work_info ;

/* -------------------------------------- */

typedef spocp_result_t (proto_op)( struct _work_info *);

/* -------------------------------------- */

typedef struct _spocp_iobuf {
	char		*buf ;
	char		*r ;	/* Where in the buffer reading should start */
	char		*w ;	/* Where in the buffer writing should start */
	char		*p ;	/* Where in the buffer ... */
	char		*end ;	/* end of buffer */
	size_t		bsize ;
	size_t		left ;
	pthread_mutex_t	lock ;
	pthread_cond_t	empty ; /* do we need one for full too ?? */
} spocp_iobuf_t ;

typedef struct _regexp {
	regex_t	preg ;
	char	 *regex ;
} regexp_t ;
	
typedef struct _ruleset {
	pthread_rdwr_t	rw_lock;
	pthread_mutex_t	transaction ;
	db_t		*db ;
	char		*name ;
	struct _ruleset	*left ;
	struct _ruleset	*right ;
	struct _ruleset	*down ;
	struct _ruleset	*up ;
} ruleset_t ;

/* -------------------------------------- */

/* The connection states. */

#define CNST_FREE	0
#define CNST_SETUP	1
#define CNST_ACTIVE	2
#define CNST_STOP	5
#define CNST_WRITE	6

#define INACTIVE    0
/* SSL/TLS requested, waiting for the reply to hit the client */
#define REQUEST    	1
/* SSL/TLS negotiation can be started */
#define NEGOTIATION	2
/* SSL/TLS is in action */
#define ACTIVE      3


/* -------------------------------------- */

struct _tpool ;

/* --- pool of items ---- */

typedef struct _pool_item {
	struct _pool_item	*prev;
	struct _pool_item	*next;
	void			*info ;
} pool_item_t ;

typedef struct _pool {
	int		size ;
	pool_item_t	*head ;
	pool_item_t	*tail ;
	pthread_mutex_t lock ;		/* look on the pool */
} pool_t ;

/* struct having items in active and free pools */
typedef struct _afpool {
	pool_t		*active ;
	pool_t		*free ;
	pthread_mutex_t aflock ;		/* look on the pool */
/*
	pthread_cond_t	pool_not_empty;
	pthread_cond_t	pool_not_full;
	pthread_cond_t	pool_empty;
*/
} afpool_t ;

/* -------------------------------------------------------------------- */
/*
	this is the configuration stuff
	first the server configuration */

/* and then plugin specific configuration */

typedef struct _other {
	char		*key ;
	strarr_t	*arr ;
	struct _other	*next ;
} other_t ;

/* -------------------------------------------------------------------- */

typedef struct opstack_t {
	int			oper;
	ruleset_t		*rs;
	octarr_t		*oparg;
	octarr_t		*rollback;
	struct opstack_t	*next;
	void			*rule;
} opstack_t ;

/* -------------------------------------------------------------------- */

typedef struct _thread {
	int		id ;
	pthread_t	thread ;
	long		count ; 
	struct _conn	*conn ;
} spocp_thread_t ;

/* -------------------------------------------------------------------- */

typedef struct _server
{
	pthread_mutex_t mutex ;	/* lock on the database */
	ruleset_t	*root ;	/* rule database */

	dback_t		*dback ;
	plugin_t	*plugin ;

	int		listen_fd ; /* listen socket */
	pthread_mutex_t	mlock ;	/* listener lock */
	int		type ;	/* AF_INET, AF_INET6 or AF_LOCAL */	
	char		*id ;

	int		threads ;

	int		timeout ;
	int		nconn ;		/* max simultaneous connections */
	time_t	mtime;		/* time of last modification */
	char	*certificateFile ;
	char	*privateKey ;
	char	*caList ;
	char	*dhFile ;
	char	*SslEntropyFile ;
	char	*passwd ;

	char	*logfile ;
	char	*pidfile ;
	int		port ;
	char	*uds ;
	char	*rulefile ;
	int		sslverifydepth ;
	char	*server_id ;
	int		srvtype ;
	
	char	*hostname;

#ifdef HAVE_SSL
	int		clientcert ;
#define HARD	0
#define DEMAND	1
#define NONE	2

	void	*ctx;
	void	*ssl;
			/* security strength factor, not used presently */
	int		ssf ;
#endif


	pthread_mutex_t	t_lock ;	 /* look on the work pool */
	struct _tpool	*work ;		/* the pool of work items */

	afpool_t	*connections ;	/* pool of connections active/free */

	int		readonly;
} srv_t ;

/* -------------------------------------------------------------------- */

/* forward declaration */
struct _reply ;

/* -------------------------------------------------------------------- */

typedef struct _conn {

	int		fd ;	    /* connection filedescriptor */
	int		fdw ;	    /* for writing if different from reading */
	int		status ;
	int 	sslstatus ;
	int		con_type ;
	int		ops_pending ;
	int		stop ;	    /* Set (!= 0) if this connection is
				       to be closed */
	int		operations;
	int		layer;
#define SPOCP_LAYER_NONE 0x0
#define SPOCP_LAYER_CONF 0x1
#define SPOCP_LAYER_INT	0x2

	/* The last time anything happend on this connection */
	time_t		last_event ; 
	/* When the last operation was started */
	struct timeval	op_start;
	/* When the last operation was completed */
	struct timeval	op_end; 
	/* Information about the subject that issued the operation */
	spocp_req_info_t sri ;	
	/* A pointer to the server on which this connection is running */
	struct _server	*srv ;	
	/* rule database for this connection */
	ruleset_t	*rs ;	

	/* information about the persistent store */
	dbcmd_t		dbc ;

	/* transaction stuff */
	int		transaction ; /* Set (!= 0) if transaction is active */
	opstack_t	*ops;

	pthread_mutex_t	clock ;

	/* Input/Output buffers */
	spocp_iobuf_t	*in ;
	spocp_iobuf_t	*out ;

	/* lock on the reply queue */
	pthread_mutex_t rlock;
	/* the reply queue */
	struct _reply	*head;

#ifdef HAVE_SSL
	void		*ssl;
	int		tls_ssf ;	 /* security strength factor, placeholder */
#endif
#ifdef HAVE_SASL
	sasl_conn_t	*sasl;
	int		sasl_ssf;
	char		*sasl_mech;
	char		*sasl_username;
#endif

#ifdef HAVE_SSL
	char		*subjectDN ;
	char		*issuerDN ;
	char		*cipher ; 
	char		*ssl_vers ;
#endif
#ifdef HAVE_SASL
#endif
	char		*transpsec ;

	/* pointers to functions that is used for reading, writing and closing from
	this connection */
	int (*readn) (struct _conn *conn, char *buf, size_t len);
	int (*writen)(struct _conn *conn, char *buf, size_t count);
	int (*close) (struct _conn *conn);

} conn_t ;

/* -------------------------------------------------------------------- */

typedef struct _work_info {
	proto_op	*routine ;
	conn_t		*conn ;
	octarr_t	*oparg ;
	octet_t		*oppath;
	octet_t		oper;
	spocp_iobuf_t	*buf;
} work_info_t;

typedef struct _reply {
	work_info_t	*wi;
	spocp_iobuf_t	*buf;
	struct _reply	*next;
} reply_t ;

/* -------------------------------------------------------------------- */

typedef struct _tpool {
	/* pool characteristics */
	int			num_threads;
	int			do_not_block_when_full;

	/* the threads */
	pthread_t		*threads;

	/* work pool */
	int			max_queue_size;
	int			cur_queue_size;
	afpool_t		*queue ;		/* work queue */
	int			queue_closed;

	int			shutdown;

	/* pool synchronization */
	pthread_cond_t		queue_not_empty;
	pthread_cond_t		queue_not_full;
	pthread_cond_t		queue_empty;
} tpool_t;

/* ---------------------------------------------------------------------- */

#define SPOCP_ADD	0x01
#define SPOCP_DEL	0x02

#define SPOCP_IOBUFSIZE 4096

/************* functions ********************/

/* main.c */
spocp_result_t get_rules( srv_t *srv );

/*  */
void	wake_listener( int );
void	daemon_init( char *procname, int facility );
int	spocp_stream_socket( int port );
int	spocp_unix_domain_socket( char *uds );

void	spocp_server( void *vp );
int	send_results( conn_t *conn );
int	spocp_send_results( conn_t *conn );

/* iobuf.c */

spocp_iobuf_t	*iobuf_new( size_t size );
spocp_result_t	iobuf_resize( spocp_iobuf_t *io, int increase, int lock );
spocp_result_t	iobuf_add( spocp_iobuf_t *io, char *s );
spocp_result_t	iobuf_addn( spocp_iobuf_t *io, char *s, size_t n );
spocp_result_t	iobuf_add_octet( spocp_iobuf_t *io, octet_t *o );
spocp_result_t	iobuf_add_len_tag( spocp_iobuf_t *io);
spocp_result_t	iobuf_insert( spocp_iobuf_t *io, char *where, char *what, int len );
void		iobuf_shift( spocp_iobuf_t *io );
void		iobuf_flush( spocp_iobuf_t *io );
int		iobuf_content( spocp_iobuf_t *io );
void		conn_iobuf_clear( conn_t * );
void		iobuf_free( spocp_iobuf_t *);
void		iobuf_print( char *,spocp_iobuf_t *);
void		iobuf_info( spocp_iobuf_t *);

/* conn.c */

char *spocp_next_line( conn_t *conn );

int	conn_writen( conn_t *ct, char *str, size_t n );
int	conn_readn( conn_t *ct, char *str, size_t max );
int	conn_close( conn_t *conn );

conn_t	*spocp_open_connection( int fd, srv_t *ss );
void	conn_iobuf_clear( conn_t *con );
conn_t	*conn_get( srv_t *srv );
void	conn_free( conn_t *c );
void	conn_reset( conn_t *c );
conn_t	*conn_new( void );
void	conn_init( conn_t *con );
int	conn_setup( conn_t *con, srv_t *srv, int fd, char *host, char *ipaddr );
int	spocp_conn_write( conn_t *conn );
int	spocp_conn_read( conn_t *conn );

/* ------ reply.c ------- */

/* internal
reply_t	*reply_new( work_info_t * );
void	reply_free( reply_t * );
reply_t	*reply_push( reply_t * );
reply_t	*reply_pop( reply_t ** );
*/
reply_t	*reply_add( reply_t *, work_info_t * );
int	gather_replies( spocp_iobuf_t *, conn_t *);
int	add_reply_queuer( conn_t *, work_info_t *);

/* ------ read.c ------- */

int	read_rules( srv_t *, char *, dbcmd_t *);
int	dback_read_rules( dbcmd_t *dbc, srv_t *srv, spocp_result_t *rc );

#ifdef HAVE_SSL
SSL_CTX		*tls_init( srv_t *srv );
spocp_result_t	tls_start( conn_t *conn, ruleset_t *rs )	;
#endif

/* config.c */
int		read_config( char *file, srv_t *srv );
spocp_result_t	conf_get( void *vp, int arg, void **res );
int		set_backend_cachetime( srv_t *srv );

/* */

regexp_t	*pattern_new( char *s );
int	pattern_match( regexp_t *regp, char *s );
void	pattern_rm( regexp_t *regp );

/*
int	add_client_aci( char *str, ruleset_t **rs );
int	rm_aci( char *str, ruleset_t *rs );
int	print_aci( conn_t *conn, ruleset_t *rs );
aci_t	*aci_dup( aci_t *old );
*/

spocp_result_t	treeList( ruleset_t *rs, conn_t *con, octarr_t *oa, int f );

/* util.c */

char	*rm_lt_sp( char *s, int shift );

/* ruleset.c */

void		ruleset_free( ruleset_t *rs, int f );
void		ruleset_free_onelevel( ruleset_t *rs );
void		ruleset_tree( ruleset_t *rs, int lev );
ruleset_t	*ruleset_new( octet_t *name );
ruleset_t	*ruleset_create( octet_t *name, ruleset_t *root );
ruleset_t	*ruleset_find( octet_t *name, ruleset_t *rs );

spocp_result_t	ruleset_name( octet_t *orig, octet_t *rsn );
spocp_result_t	search_in_tree( ruleset_t *, octet_t *, octet_t *, spocp_req_info_t *, int );
spocp_result_t	pathname_get( ruleset_t *rs, char *buf, int buflen );

/* ss.c */

spocp_result_t	ss_allow( ruleset_t *, octet_t *, resset_t **, int );
spocp_result_t	ss_del_rule( ruleset_t *rs, dbcmd_t *d, octet_t *op, int scope );
spocp_result_t	skip_sexp( octet_t *sexp );
/*
spocp_result_t ss_add_rule( ruleset_t *rs, octarr_t *oa, bcdef_t *b );
*/

void	ss_del_db( ruleset_t *rs, int scope );
octet_t **ss_list_rules( ruleset_t *rs, octet_t *pattern, int *rc, int scope );
int	ss_rules( ruleset_t *rs, int scope );
void	*ss_dup( ruleset_t *rs, int scope );
void	free_db( db_t *db );

/* srv.c */

spocp_result_t	com_subject( work_info_t *wi );
spocp_result_t	com_begin( work_info_t *wi );
spocp_result_t	com_rollback( work_info_t *wi );
spocp_result_t	com_commit( work_info_t *wi );
spocp_result_t	com_login( work_info_t *wi );
spocp_result_t	com_logout( work_info_t *wi );
spocp_result_t	com_query( work_info_t *wi );
spocp_result_t	com_starttls( work_info_t *wi );
spocp_result_t	com_tlsneg( work_info_t *wi );
spocp_result_t	com_auth( work_info_t *wi );
spocp_result_t	com_capa( work_info_t *wi );
spocp_result_t	com_remove( work_info_t *wi );
spocp_result_t	com_add( work_info_t *wi );
spocp_result_t	com_list( work_info_t *wi );
spocp_result_t	com_summary( work_info_t *wi );
spocp_result_t	com_show( work_info_t *wi );

spocp_result_t	get_operation(work_info_t *);

spocp_result_t	do_reread_rulefile( work_info_t *);

spocp_result_t	return_busy( work_info_t * );

/* run.c */

void	spocp_srv_run( srv_t *srv );

/* cpool.c */

int	srv_init( srv_t *srv, char *configfile );
void	srv_free( srv_t *srv);

/* pool.c */

pool_item_t	*item_get( afpool_t *afp );
pool_item_t	*pool_pop( pool_t *pool );
pool_item_t	*first_active( afpool_t *afp );
void	 pool_add( pool_t *p, pool_item_t *pi );
void	 pool_rm( pool_t *pool, pool_item_t *item );


int	item_return( afpool_t *afp, pool_item_t *item );
int	afpool_free_item( afpool_t *afp );
int	afpool_active_item( afpool_t *afp );
afpool_t	*afpool_new( void );
int	afpool_lock( afpool_t *afp );
int	afpool_unlock( afpool_t *afp );
void	afpool_free( afpool_t *a, ffunc *ff );


/* con.c */

char	*next_line( conn_t *conn );
void	conn_free( conn_t *conn );

/* init.c */

int	run_plugin_init( srv_t *srv );

/* saci.c */

void		saci_init( void );
spocp_result_t	server_access( conn_t *con );
spocp_result_t	operation_access( work_info_t *);
spocp_result_t	rule_access( work_info_t *);

/* tpool.c */

tpool_t	*tpool_init( int, int, int );
int	tpool_add_work( tpool_t *, work_info_t *);
int	tpool_destroy( tpool_t *, int );

/* pool.c */

pool_item_t	*get_item( afpool_t *afp );
pool_item_t	*pop_from_pool( pool_t *pool );
pool_item_t	*dd_to_pool( pool_t *pool, pool_item_t *item );
pool_item_t	*first_active( afpool_t *afp );

int	return_item( afpool_t *afp, pool_item_t *item );
int	afpool_free_item( afpool_t *afp );
int	afpool_active_item( afpool_t *afp );
void	add_to_pool( pool_t *p, pool_item_t *pi );
void	rm_from_pool( pool_t *pool, pool_item_t *item );

afpool_t	*afpool_new( void );

int	afpool_lock( afpool_t *afp );
int	afpool_unlock( afpool_t *afp );

int	number_of_active( afpool_t *afp );
int	number_of_free( afpool_t *afp );

/* -------------------------------------------*/

void		afpool_push_item( afpool_t *afp, pool_item_t *pi );
void		afpool_push_empty( afpool_t *afp, pool_item_t *pi );
pool_item_t	*afpool_get_item( afpool_t *afp );
pool_item_t	*afpool_get_empty( afpool_t *afp );
pool_item_t	*afpool_first( afpool_t *afp );

/* -------------------------------------------*/

void		opstack_free( opstack_t *osn );
opstack_t	*opstack_new( int, ruleset_t *, octarr_t *);
opstack_t	*opstack_push( opstack_t *a, opstack_t *b);

/* -------------------------------------------*/

varr_t *collect_indexes(ruleset_t *rs);

/* -------------------------------------------*/

#ifdef HAVE_SSL

int	THREAD_setup( void );
int	THREAD_cleanup( void );

#endif 

/* -------------------------------------------*/

int	lutil_pair( int sds[2] );

/* ------------ GLOBALS ------------------ */

int		spocp_err ;
extern srv_t	srv ;
struct		utsname myname ;
char		*localcontext ;

extern int	allow_serverity ;
extern int	deny_serverity ;

int	wake_sds[2] ;

#endif

