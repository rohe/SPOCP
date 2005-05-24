
/***************************************************************************
                           tpool.c  -  description
                             -------------------
    begin                : Wed Jan 9 2002
    copyright            : (C) 2002 by Roland Hedberg
    email                : roland@catalogix.se
 ***************************************************************************/

#include "locl.h"

/*
#ifdef NO_THREADS
static int waking;
#define WAKE_LISTENER(w) \
((w && !waking) ? write( wake_sds[1], "0", 1 ), waking=1 : 0)
#else
#define WAKE_LISTENER(w) \
do { if (w) write( wake_sds[1], "0", 1 ); } while(0)
#endif
*/

void           *tpool_thread(void *);

#define DEF_QUEUE_SIZE  16

typedef struct _ptharg {
    tpool_t        *tpool;
    int             id;
} ptharg_t;

/* #define DEBUG_TPO 0 */

static int
add_work_structs(pool_t * pool, int n)
{
    int             i;
    void           *tw;
    pool_item_t    *pi;

    for (i = 0; i < n; i++) {
        tw = Calloc(1, sizeof(work_info_t));
        pi = (pool_item_t *) Calloc(1, sizeof(pool_item_t));
        pi->info = tw;

        pool_add(pool, pi);
    }

    return i;
}

static void
free_work_struct(void *vp)
{
    work_info_t *wi;

    if (vp) {
        wi = (work_info_t *) vp;
        conn_free(wi->conn);
        octarr_free(wi->oparg);
        oct_free(wi->oppath);
        octclr(&wi->oper);
        iobuf_free(wi->buf);
        Free( wi );
    }
}

static void
free_wpool(afpool_t * afp)
{
    afpool_free( afp, free_work_struct);
}

/*
 * ! Create the initial set of threads, initialize them and add them to the
 * free pool. \param wthread Number of work threads to start with \param
 * max_queue_size This is as far as the workpool size is allowed to grow
 * \param do_not_block_when_full If no work thread is free one can either wait 
 * or so something like signal that server is too busy. \return A pointer to
 * the thread pool 
 */
tpool_t        *
tpool_init(int wthreads, int max_queue_size, int do_not_block_when_full)
{
    int             i;
    tpool_t        *tpool = 0;
    ptharg_t       *ptharg;

    /*
     * allocate a pool data structure 
     */
    tpool = (tpool_t *) Malloc(sizeof(tpool_t));

    /*
     * initialize th fields 
     */
    tpool->num_threads = wthreads;
    tpool->max_queue_size = max_queue_size;
    tpool->do_not_block_when_full = do_not_block_when_full;

    /*
     * there is a lower limit to who small the pool can be 
     */
    if (max_queue_size < DEF_QUEUE_SIZE)
        tpool->cur_queue_size = max_queue_size;
    else
        tpool->cur_queue_size = DEF_QUEUE_SIZE;

    tpool->queue = afpool_new();

    add_work_structs(tpool->queue->free, tpool->cur_queue_size);

    tpool->queue_closed = 0;
    tpool->shutdown = 0;

    /*
     * traceLog(LOG_DEBUG,"Creating %d threads", wthreads ) ; 
     */

    tpool->threads = (pthread_t *) Calloc(wthreads, sizeof(pthread_t));

    pthread_cond_init(&(tpool->queue_not_empty), NULL);
    pthread_cond_init(&(tpool->queue_not_full), NULL);
    pthread_cond_init(&(tpool->queue_empty), NULL);

    /*
     * create threads 
     */
    for (i = 0; i < wthreads; i++) {
        ptharg = (ptharg_t *) Calloc (1, sizeof( ptharg_t ));

        ptharg->tpool = tpool;
        ptharg->id = i;

        pthread_create(&(tpool->threads[i]), NULL, tpool_thread,
                   (void *) ptharg);
    }

    return tpool;
}

/* FIXME: should set a upper limit on work items per connection */
/* Would that mean that I have to set an upper limit on connections from a 
 * specific host, of a specific type ?? */

int
tpool_add_work(tpool_t * tpool, work_info_t *wi)
{
    work_info_t    *workp;
    pool_item_t    *pi;
    int             fi;
    afpool_t        *afp = tpool->queue;

#ifdef DEBUG_TPO
    timestamp("tpool_add_work");
    traceLog(LOG_DEBUG,"cur_queue_size:%d", tpool->cur_queue_size);
    traceLog(LOG_DEBUG,"max_queue_size:%d", tpool->max_queue_size);
    traceLog(LOG_DEBUG,"shutdown:%d", tpool->shutdown);
    traceLog(LOG_DEBUG,"queue_closed:%d", tpool->queue_closed);
#endif

    /*
     * traceLog(LOG_DEBUG, "queued items: %d", number_of_active( afp )) ; 
     */

    /*
     * traceLog(LOG_DEBUG, "queue size: %d, max_size: %d", tpool->cur_queue_size,
     * tpool->max_queue_size) ; 
     */

    /*
     * queue full and at maxsize but not locked 
     */
    afpool_lock( afp );
    fi = afpool_free_item(afp);

    while ((fi == 0) &&
           (tpool->cur_queue_size == tpool->max_queue_size) &&
           (!(tpool->shutdown || tpool->queue_closed))) {

        traceLog(LOG_DEBUG,"current queue_size=%d", tpool->cur_queue_size);

        /*
         * and this caller doesn't want to wait 
         */
        if (tpool->do_not_block_when_full) {
            return 0;
        }

        /*
         * wait for a struct to be free 
         */
        pthread_cond_wait(&(tpool->queue_not_full),
                  &(afp->aflock));
    }
    afpool_unlock( afp );

    /*
     * the pool is in the process of being destroyed 
     */
    if (tpool->shutdown || tpool->queue_closed) {
        return -1;
    }

    /*
     * Any free workp structs around ? 
     */

    afpool_lock( afp );
    if (afpool_free_item(afp) == 0) {  /* NO ! */
        traceLog(LOG_NOTICE,"No free workp structs !! ");
        if (tpool->cur_queue_size < tpool->max_queue_size) {
            if (tpool->cur_queue_size + DEF_QUEUE_SIZE <
                tpool->max_queue_size)
                add_work_structs(afp->free,
                         DEF_QUEUE_SIZE);
            else
                add_work_structs(afp->free,
                         tpool->max_queue_size -
                         tpool->cur_queue_size);
        } else {    /* shouldn't happen */
            traceLog(LOG_NOTICE,"Reached the max_queue_size");
            afpool_unlock(afp);
            return 0;
        }
    }
    afpool_unlock( afp );

#ifdef DEBUG_TPO
    traceLog(LOG_NOTICE,"get_empty");
#endif
    pi = afpool_get_empty(afp);

    workp = (work_info_t *) pi->info;

    workp->routine = wi->routine;
    workp->conn = wi->conn;
    workp->oparg = wi->oparg;
    workp->oper = wi->oper;
#ifdef DEBUG_TPO
    traceLog(LOG_NOTICE,"New workp->buf");
#endif
    workp->buf = iobuf_new( 1024 );
    workp->conn->ops_pending++;

    add_reply_queuer( wi->conn, workp );

    afpool_push_item(afp, pi);

#ifdef DEBUG_TPO
    timestamp("Signal the workitem");
#endif
    pthread_cond_signal(&(tpool->queue_not_empty));

    return 1;
}

int
tpool_destroy(tpool_t * tpool, int finish)
{
    int         i;
    afpool_t    *afp;

    /*
     * Is a shutdown already in progress? 
     */
    if (tpool->queue_closed || tpool->shutdown) {
        return 0;
    }

    tpool->queue_closed = 1;
    afp = tpool->queue;

    /*
     * If the finish flag is set, wait for workers to drain queue 
     */
    if (finish == 1) {
        afpool_lock(afp);
        while (tpool->cur_queue_size != 0) {
            pthread_cond_wait(&(tpool->queue_empty),
                      &(afp->aflock));
        }
        afpool_unlock(afp);
    }

    tpool->shutdown = 1;

    /*
     * Wake up any workers so they recheck shutdown flag 
     */
    pthread_cond_signal(&(tpool->queue_not_empty));
    pthread_cond_signal(&(tpool->queue_not_full));


    /*
     * Wait for workers to exit 
     */
    for (i = 0; i < tpool->num_threads; i++) {
        pthread_join(tpool->threads[i], NULL);
    }

    /*
     * Now free pool structures 
     */
    Free(tpool->threads);

    free_wpool(afp);

    Free(tpool);

    return 1;
}

/*
 * where the threads are spending their time 
 */

void           *
tpool_thread(void *arg)
{
    ptharg_t    *ptharg = (ptharg_t *) arg;
    tpool_t     *tpool;
    work_info_t *my_workp;
    int     res, id;
    pool_item_t *pi;
    afpool_t    *workqueue;
    conn_t      *conn;

    tpool = ptharg->tpool;
    id = ptharg->id;
    workqueue = tpool->queue;

    for (;;) {

#ifdef DEBUG_TPO
        traceLog(LOG_DEBUG, "Thread %d looking for work", id ) ; 
#endif

        afpool_lock(workqueue);

#ifdef DEBUG_TPO
        traceLog(LOG_DEBUG, "Thread %d got the lock", id ) ; 
#endif
        /*
         * nothing in the queue, wait for something to appear 
         */
        while (afpool_active_item(workqueue) == 0
               && (!tpool->shutdown)) {
            /*
             * cond_wait unlocks aflock on entering, locks it on
             * return 
             */
#ifdef DEBUG_TPO
        traceLog(LOG_DEBUG, "Thread %d waiting for something to appear", id ) ; 
#endif

            pthread_cond_wait(&(tpool->queue_not_empty),
                      &(workqueue->aflock));

#ifdef DEBUG_TPO
        traceLog(LOG_DEBUG, "Thread %d awaken", id ) ; 
#endif

        }
        afpool_unlock(workqueue);

#ifdef DEBUG_TPO
        traceLog(LOG_DEBUG, "Thread %d working", id ) ; 
#endif

        /*
         * Has a shutdown started while I was sleeping? 
         */
        if (tpool->shutdown == 1) {
            pthread_exit(NULL);
        }

        /*
         * Get to work, dequeue the next item 
         */

        pi = afpool_get_item(workqueue);


        if (!pi)
            continue;

        /*
         * traceLog(LOG_DEBUG, "Active workitems %d", number_of_active(
         * workqueue )) ; 
         */

        if(( my_workp = (work_info_t *) pi->info) == NULL )
            continue;

        conn = my_workp->conn;

        /*
         * Handle waiting destroyer threads
         * if( afpool_active_items( afp ) == 0 && 
         * afpool_cond_signal_empty( afp )) ; 
         */

        /*
         * Do this work item 
         */
        res = (*(my_workp->routine)) ( my_workp );

        gettimeofday( &conn->op_end, NULL );
        conn->operations++;

        reply_add( conn->head, my_workp );
        if ( send_results(conn) == 0)
            res = SPOCP_CLOSE;

        conn->ops_pending-- ;
        /*
        if (conn->stop == 1 && iobuf_content(conn->out) == 0 )
            res = SPOCP_CLOSE;
        */

        DEBUG( SPOCP_DSRV )
            print_elapsed("Elapsed time", conn->op_start,
                conn->op_end) ; 

        wake_listener(1);

        /*
         * If error get a clean sheat 
         */
        if (res == -1)
            iobuf_flush(conn->in);

        octarr_free(my_workp->oparg); 
        memset( my_workp, 0, sizeof( work_info_t ));

        /*
         * returned to free list 
         */
        afpool_push_empty(workqueue, pi);

        /*
         * tell the world there are room for more work 
         */
        if ((!tpool->do_not_block_when_full) &&
            (workqueue->free->size == 1)) {

            pthread_cond_signal(&(tpool->queue_not_full));
        }

#ifdef DEBUG_TPO
        timestamp("loopturn done");
#endif
    }

    return (NULL);
}

