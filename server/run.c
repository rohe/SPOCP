#include "locl.h"
RCSID("$Id$");

/*
 * Todo: o Timeouts and limits on how long connections should be kept when
 * not actively written to or read from. Also for half-done reads and writes.
 * o Check exception handling 
 */


typedef struct sockaddr SA;
extern int received_sigterm;

/*
#define AVLUS 0
*/

/*
 * ---------------------------------------------------------------------- 
 */

static int
spocp_getnameinfo( const struct sockaddr *sa, int len, char *name,
    int namelen, char *ipaddr, int addrlen)
{
        int rc = 0;
    
#if defined( HAVE_GETNAMEINFO )

        rc = getnameinfo( sa, len, name, namelen, NULL, 0, 0 );
    if (rc)
        return rc;

        rc = getnameinfo( sa, len, ipaddr, addrlen, NULL, 0, NI_NUMERICHOST );

#else /* !HAVE_GETNAMEINFO */
        char *addr;
        int alen;
        struct hostent *hp = NULL;

        if (sa->sa_family == AF_INET) {
                struct sockaddr_in *sin = (struct sockaddr_in *)sa;
                addr = (char *)&sin->sin_addr;
                alen = sizeof(sin->sin_addr);
        } else 
                rc = NO_RECOVERY;

    hp = gethostbyaddr( addr, alen, sa->sa_family );
    if (hp) {
        strlcpy( name, hp->h_name, namelen );
        strlcpy( ipaddr, inet_ntoa(hp->h_addr_list[0]), addrlen);
        rc = 0;
    } else 
        rc = h_errno;


#endif  /* !HAVE_GETNAMEINFO */

        return rc;
}

/*
 * !!! This is borrowed from OpenLDAP !!!! 
 */

/*
 * Return a pair of socket descriptors that are connected to each other. The
 * returned descriptors are suitable for use with select(). The two
 * descriptors may or may not be identical; the function may return the same
 * descriptor number in both slots. It is guaranteed that data written on
 * sds[1] will be readable on sds[0]. The returned descriptors may be datagram 
 * oriented, so data should be written in reasonably small pieces and read all 
 * at once. On Unix systems this function is best implemented using a single
 * pipe() call. 
 */

int
lutil_pair(int sds[2])
{
#ifdef HAVE_PIPE
    return pipe(sds);
#else
    struct sockaddr_in si;
    int             rc, len = sizeof(si);
    int             sd;

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd == -1) {
        return sd;
    }

    (void) memset((void *) &si, '\0', len);
    si.sin_family = AF_INET;
    si.sin_port = 0;
    si.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    rc = bind(sd, (SA *) & si, len);
    if (rc == -1) {
        close(sd);
        return rc;
    }

    rc = getsockname(sd, (SA *) & si, (socklen_t *) & len);
    if (rc == -1) {
        close(sd);
        return rc;
    }

    rc = connect(sd, (SA *) & si, len);
    if (rc == -1) {
        close(sd);
        return rc;
    }

    sds[0] = sd;
#if !HAVE_WINSOCK
    sds[1] = dup(sds[0]);
#else
    sds[1] = sds[0];
#endif
    return 0;
#endif
}

void
wake_listener(int w)
{
    if (w) {
        DEBUG(SPOCP_DSRV)
            traceLog(LOG_DEBUG,"waking listener on %d", wake_sds[1]);
        write(wake_sds[1], "0", 1);
    }
}

/*
 * ---------------------------------------------------------------------- 
 */

static void
run_stop( srv_t *srv, conn_t *conn, pool_item_t *pi )
{
    int err;

    DEBUG(SPOCP_DSRV)
        traceLog(LOG_DEBUG,"removing connection %d", conn->fd);

    conn->close(conn);
    conn_reset(conn);
    /*
     * unlocks the conn struct as a side effect 
     */
    item_return(srv->connections, pi);

    /*
     * release lock 
     */
    if ((err = pthread_mutex_unlock(&conn->clock)) != 0)
        traceLog(LOG_ERR,"Panic: unable to release lock on connection: %s",
            strerror(err));

}

static int read_work( srv_t *srv, conn_t *conn, int flag )
{
    int     n;
    spocp_result_t  res;
    work_info_t wi;

#ifdef AVLUS
    traceLog(LOG_DEBUG,"read_work:input readable on %d", conn->fd);
#endif

    /*
     * read returns number of bytes read 
     */
	if (flag) {
	    n = spocp_conn_read(conn);

#ifdef AVLUS
	    traceLog(LOG_DEBUG,"read_work:Read returned %d from %d", n, conn->fd);
#endif

	    /*
	     * traceLog(LOG_DEBUG, "[%s]", conn->in->buf ) ; 
	     */
	    if (n == 0) {   /* connection probably
	             * terminated by other side */
	            /* Is there a better check ? */
	        conn->stop = 1;
	        return 1;
	    }
	}
#ifdef AVLUS
    timestamp("read_work:read con");
#endif

    while( iobuf_content( conn->in )) {
        memset( &wi, 0, sizeof( work_info_t ));
        wi.conn = conn;

        res = get_operation(&wi);
#ifdef AVLUS
        traceLog(LOG_DEBUG,"read_work:Getops returned %d", res);
#endif
        if (res != SPOCP_SUCCESS)
            return 1;

        gettimeofday( &conn->op_start, NULL );
        time(&conn->last_event);

#ifdef AVLUS
        timestamp("read_work:add work item");
#endif

        if( tpool_add_work(srv->work, &wi) == 0 ) {
            /* place this last in the list */
#ifdef AVLUS
            timestamp("read_work:add_reply_queur");
#endif
            add_reply_queuer( conn, &wi );
#ifdef AVLUS
            timestamp("read_work:new iobuf");
#endif
            wi.buf = iobuf_new(512);
            return_busy( &wi );
#ifdef AVLUS
            timestamp("read_work:reply add");
#endif
            reply_add( conn->head, &wi );
#ifdef AVLUS
            timestamp("read_work:send_results");
#endif
            if (send_results(conn) == 0)
                res = SPOCP_CLOSE;
            return 1;
        }
    }

    return 0;
}

void
spocp_srv_run(srv_t * srv)
{
    int             val, err = 0, maxfd, client, nready, stop_loop = 0, n;
    socklen_t       len;
    conn_t         *conn;
    pool_item_t    *pi, *next;
    struct sockaddr_in client_addr;
    fd_set          rfds, wfds;
    struct timeval  noto;
    char            ipaddr[64], hname[NI_MAXHOST];
    int             pe = 1;

    signal(SIGPIPE, SIG_IGN);

    if (lutil_pair(wake_sds) != 0) {
        fprintf(stderr, "Problem in creating wakeup pair");
        exit(0);
    }

    traceLog(LOG_INFO,"Running server!!");

    memset( ipaddr, 0, 64);

    if (srv->connections) {
        int             f, a;
        f = number_of_free(srv->connections);
        a = number_of_active(srv->connections);
        DEBUG(SPOCP_DSRV) traceLog(LOG_DEBUG,"Active: %d, Free: %d", a, f);
    } else {
        DEBUG(SPOCP_DSRV) traceLog(LOG_DEBUG,"NO connection pool !!!");
    }

    while (!stop_loop) {

        FD_ZERO(&rfds);
        FD_ZERO(&wfds);

        FD_SET(srv->listen_fd, &rfds);
        FD_SET(wake_sds[0], &rfds);
        maxfd = MAX(wake_sds[0], srv->listen_fd);

        /*
         * Loop through connection list and prune away closed
         * connections, the active are added to the fd_sets. Since
         * this routine is the only one that adds and delets
         * connections and there is only one thread that runs this
         * routine I don't have to lock the connection pool 
         */
        pi = afpool_first(srv->connections);

        if( pi == 0 && received_sigterm) break;

        for (; pi; pi = next) {
            conn = (conn_t *) pi->info;

            if ((err = pthread_mutex_lock(&conn->clock)) != 0)
                traceLog(LOG_ERR,
                    "Panic: unable to obtain lock on connection: %.100s",
                     strerror(err));

            /*
             * DEBUG( SPOCP_DSRV ) traceLog(LOG_DEBUG,"fd=%d [status: %d,
             * ops_pending:%d]", conn->fd, conn->status,
             * conn->ops_pending); 
             */

            /*
             * this since pi might disapear before the next turn
             * of this loop is done 
             */
            next = pi->next;

            if(time(NULL) - conn->last_event > conn->srv->timeout)
                conn->stop = 1;

            /* only close if output queue is empty */
            /*if (conn->stop && !conn->ops_pending && */
            if (conn->stop) {
                int nr = 0;

                traceLog(LOG_INFO,"Pending stop on %d, ops undone %d",
                    conn->fd, conn->ops_pending);
                if(conn->ops_pending ||
                    (nr = iobuf_content(conn->out))) {
                    traceLog(LOG_INFO,
                        "Not done yet, %d bytes in queue", nr);
                    maxfd = MAX(maxfd, conn->fd);
                }
                else {
                    run_stop( srv, conn, pi);
                    pe--;
                    continue;
                }
            } else {
                if (conn->status == CNST_WRITE){
                    maxfd = MAX(maxfd, conn->fd);
                    FD_SET(conn->fd, &wfds);
                }
                if (conn->sslstatus != NEGOTIATION && conn->sslstatus != REQUEST){
                    maxfd = MAX(maxfd, conn->fd);
                    FD_SET(conn->fd, &rfds);
                }
            }

            if ((err = pthread_mutex_unlock(&conn->clock)) != 0)
                traceLog(LOG_ERR,
                  "Panic: unable to release lock on connection: %s",
                  strerror(err));

        }

        /*
         * 0.001 seconds timeout 
         */
        noto.tv_sec = 0;
        noto.tv_usec = 1000;

#ifdef AVLUS
        timestamp("before select");
        if( FD_ISSET(wake_sds[0], &rfds))
            traceLog(LOG_INFO, "wakeup listener is on");
        traceLog(LOG_INFO, "Maxfd:%d", maxfd);
#endif
        /*
         * Select on all file descriptors, wait forever 
         */
        nready = select(maxfd + 1, &rfds, &wfds, NULL, 0);
#ifdef AVLUS
        timestamp("after select");
#endif

        /*
         * Check for errors 
         */
        if (nready < 0) {
            if (errno != EINTR)
                traceLog(LOG_ERR,"Unable to select: %s",
                     strerror(errno));
            continue;
        }

        /*
         * Timeout... or nothing there 
         */
        if (nready == 0)
            continue;

#ifdef AVLUS
        timestamp("Readable/Writeable");
#endif

        /*
         * traceLog(LOG_DEBUG, "maxfd: %d, nready: %d", maxfd, nready ) ; 
         */

        if (FD_ISSET(wake_sds[0], &rfds)) {
            char            c[BUFSIZ];

#ifdef AVLUS
            traceLog(LOG_DEBUG,"Woken by wake listener");
#endif
            read(wake_sds[0], c, sizeof(c));
        }

        /*
         * Check for new connections, don't accept any if SIGTERM has
         * been received 
         */

        if (received_sigterm == 0 && FD_ISSET(srv->listen_fd, &rfds)) {

#ifdef AVLUS
            timestamp("New connection");
#endif

            len = sizeof(client_addr);
            client =
                accept(srv->listen_fd, (SA *) & client_addr, &len);

            if (client < 0) {
                if (errno != EINTR)
                    traceLog(LOG_ERR,"Accept error: %.100s",
                         strerror(errno));
                continue;
            }

            DEBUG(SPOCP_DSRV) traceLog(LOG_ERR,"Got connection on fd=%d",
                           client);
#ifdef AVLUS
            timestamp("Accepted");
#endif

            val = fcntl(client, F_GETFL, 0);
            if (val == -1) {
                traceLog(LOG_ERR,
                    "Unable to get file descriptor flags on fd=%d: %s",
                     client, strerror(errno));
                close(client);
                goto fdloop;
            }

            /*
             * if( fcntl( client, F_SETFL, val|O_NONBLOCK ) == -1) 
             * { traceLog(LOG_ERR,"Unable to set socket nonblock on fd=%d: 
             * %s",client,strerror(errno)); close(client); goto
             * fdloop; } 
             */

            if( srv->type == AF_INET || srv->type == AF_INET6 ) {
                /*
                 * Get address not hostname of the connection 
                 */
                if ((err = spocp_getnameinfo((SA *) &client_addr, len,
                    hname, NI_MAXHOST, ipaddr, 64)) != 0) {
                    close(client);
                    goto fdloop;
                }

                if (strcmp(hname, "localhost") == 0) {
                    strlcpy(hname, srv->hostname,
                        NI_MAXHOST);
                }
            } else { /* something else I should get instead ? */
                strlcpy(hname, srv->hostname, NI_MAXHOST);
            }

            if (srv->connections) {
                int  f, a;
                f = number_of_free(srv->connections);
                a = number_of_active(srv->connections);
                traceLog(LOG_INFO,"Active: %d, Free: %d", a, f);
            } else {
                traceLog(LOG_ERR,"NO connection pool !!!");
            }

            /*
             * get a connection object 
             */
            pi = afpool_get_empty(srv->connections);

            if (pi)
                conn = (conn_t *) pi->info;
            else
                conn = 0;

            if (conn == 0) {
                traceLog(LOG_ERR,
                    "Unable to get free connection for fd=%d",
                     client);
                close(client);
            } else {
                /*
                 * initialize all the conn values 
                 */

                DEBUG(SPOCP_DSRV)
                    traceLog(LOG_DEBUG,"Initializing connection to %s",
                         hname);
                conn_setup(conn, srv, client, hname, ipaddr);

                LOG(SPOCP_DEBUG)
                    traceLog(LOG_DEBUG,"Doing server access check");

                if (server_access(conn) != SPOCP_SUCCESS) {
                    traceLog(LOG_INFO,
                        "connection %d from %s(%s) DISALLOWED",
                         conn->fd, hname, ipaddr);

                    conn_reset(conn);
                    /*
                     * unlocks the conn struct as a side
                     * effect 
                     */
                    item_return(srv->connections, pi);
                    close(client);
                } else {
                    traceLog(LOG_INFO,
                        "Accepted connection from %s on fd=%d",
                        hname, client);
                    afpool_push_item(srv->connections, pi);

                    pe++;
                }
            }
#ifdef AVLUS
            timestamp("Done with new connection setup");
#endif
        }

        /*
         * Main I/O loop: Check all filedescriptors and read and write
         * nonblocking as needed.
         */

fdloop:
        for (pi = afpool_first(srv->connections); pi; pi = next) {

            /*
             * int f = 0 ; 
             */

            conn = (conn_t *) pi->info;
            next = pi->next;

            if (conn->sslstatus == REQUEST || conn->sslstatus == NEGOTIATION)
                continue;

            /*
             * wasn't around when the select was done 
             */
            if (conn->fd > maxfd) {
#ifdef AVLUS
                traceLog(LOG_DEBUG,
                    "%d Not this time ! Too young", conn->fd);
#endif
                continue;
            }

            /*
             * Don't have to lock the connection, locking the io
             * bufferts are quite sufficient if(( err =
             * pthread_mutex_lock(&conn->clock)) != 0)
             * traceLog(LOG_ERR,"Panic: unable to obtain lock on
             * connection %d: %s", conn->fd,strerror(err)); 
             */

            if (FD_ISSET(conn->fd, &rfds) && !conn->stop) {
                if (read_work( srv, conn, 1 )) 
                    continue;
            }

            if ((n = iobuf_content(conn->out))) {

                DEBUG(SPOCP_DSRV) {
                    traceLog(LOG_DEBUG,
                        "Outgoing data on fd %d (%d)",
                        conn->fd, n);
                }

                n = spocp_conn_write(conn);

                DEBUG(SPOCP_DSRV) 
                    traceLog(LOG_DEBUG,"Wrote %d on %d",
                       n, conn->fd);

                if (n == -1)
                    traceLog(LOG_ERR,
                        "Error in writing to %d", conn->fd);
                else if (iobuf_content(conn->out) == 0) {
                    if (conn->stop) {
                        run_stop( srv, conn, pi);
                        pe--;
                    }
                    else{
						/* this is a hack */
						traceLog(LOG_DEBUG,"Nothing left in the output buffer, state=%d",conn->status);
						if (conn->phase){
							if (conn->phase & PS_STARTTLS){
								iobuf_flush(conn->in);
								/* this should be made more intelligent */
								iobuf_add(conn->in,"10:8:STARTTLS");
								conn->sslstatus = NEGOTIATION;
								traceLog(LOG_DEBUG,"Placing workitem on the work queue");
								read_work( srv, conn, 0 );
							}
							else if ((conn->phase - PS_AUTH) == 1){
								iobuf_flush(conn->in);
								iobuf_add(conn->in,"6:4:AUTH");
								traceLog(LOG_DEBUG,"Placing workitem on the work queue");
								read_work( srv, conn, 0 );
							}
						}
						else
                        	conn->status = CNST_ACTIVE;
                    }
                }
            }
            else if( received_sigterm ) {
                run_stop( srv, conn, pi);
                pe--;
            }

            /*
             * if(( err = pthread_mutex_unlock(&conn->clock)) !=
             * 0) traceLog(LOG_ERR,"Panic: unable to release lock on
             * connection %d: %s", conn->fd,strerror(err)); 
             */

        }
#ifdef AVLUS
        timestamp("one loop done");
#endif
    }

    /* */
    ruleset_free( srv->root, 1 );
    
    traceLog(LOG_NOTICE,"Closing down");
    exit(0);
}
