#include "locl.h"
RCSID("$Id$");

/*
 * Todo: o Timeouts and limits on how long connections should be kept when
 * not actively written to or read from. Also for half-done reads and writes.
 * o Check exception handling 
 */


typedef struct sockaddr SA;

#define XYZ 1

/*
 * ---------------------------------------------------------------------- 
 */

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
	if (w)
		write(wake_sds[1], "0", 1);
}

/*
 * ---------------------------------------------------------------------- 
 */


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
	char            ipaddr[65], hname[NI_MAXHOST + 1];
	int             pe = 1;

	signal(SIGPIPE, SIG_IGN);

	traceLog("Running server!!");

	if (srv->connections) {
		int             f, a;
		f = number_of_free(srv->connections);
		a = number_of_active(srv->connections);
		traceLog("Active: %d, Free: %d", a, f);
	} else {
		traceLog("NO connection pool !!!");
	}

	while (!stop_loop) {

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		FD_SET(srv->listen_fd, &rfds);
		FD_SET(wake_sds[0], &rfds);
		maxfd = MAX(wake_sds[0], srv->listen_fd);

		/*
		 * Loop through connection list and prune away closed
		 * connections, the active are added to the fd_sets Since
		 * this routine is the only one that adds and delets
		 * connections and there is only one thread that runs this
		 * routine I don't have to lock the connection pool 
		 */
		for (pi = afpool_first(srv->connections); pi; pi = next) {
			conn = (conn_t *) pi->info;

			if ((err = pthread_mutex_lock(&conn->clock)) != 0)
				traceLog
				    ("Panic: unable to obtain lock on connection: %.100s",
				     strerror(err));

			/*
			 * DEBUG( SPOCP_DSRV ) traceLog("fd=%d [status: %d,
			 * ops_pending:%d]\n", conn->fd, conn->status,
			 * conn->ops_pending); 
			 */

			/*
			 * this since pi might disapear before the next turn
			 * of this loop is done 
			 */
			next = pi->next;

			if (conn->stop && conn->ops_pending <= 0) {
				if (XYZ)
					timestamp("removing connection");
				conn->close(conn);
				conn_reset(conn);
				/*
				 * unlocks the conn struct as a side effect 
				 */
				item_return(srv->connections, pi);

				/*
				 * release lock 
				 */
				if ((err =
				     pthread_mutex_unlock(&conn->clock)) != 0)
					traceLog
					    ("Panic: unable to release lock on connection: %s",
					     strerror(err));

				pe--;
				if (XYZ)
					traceLog("Next is %p", next);
			} else {
				if (!conn->stop) {
					if (conn->status == CNST_WRITE){
						maxfd = MAX(maxfd, conn->fd);
						FD_SET(conn->fd, &wfds);
					}
				    	if (conn->status != CNST_SSL_NEG) {
						maxfd = MAX(maxfd, conn->fd);
						FD_SET(conn->fd, &rfds);
					}
				}

				if ((err =
				     pthread_mutex_unlock(&conn->clock)) != 0)
					traceLog
					    ("Panic: unable to release lock on connection: %s",
					     strerror(err));

			}
		}

		/*
		 * 0.001 seconds timeout 
		 */
		noto.tv_sec = 0;
		noto.tv_usec = 1000;

		if (XYZ)
			timestamp("before select");
		/*
		 * Select on all file descriptors, wait forever 
		 */
		nready = select(maxfd + 1, &rfds, &wfds, NULL, 0);
		if (XYZ)
			timestamp("after select");

		/*
		 * Check for errors 
		 */
		if (nready < 0) {
			if (errno != EINTR)
				traceLog("Unable to select: %s\n",
					 strerror(errno));
			continue;
		}

		/*
		 * Timeout... or nothing there 
		 */
		if (nready == 0)
			continue;

		if (XYZ)
			timestamp("Readable/Writeable");

		/*
		 * traceLog( "maxfd: %d, nready: %d", maxfd, nready ) ; 
		 */

		if (FD_ISSET(wake_sds[0], &rfds)) {
			char            c[BUFSIZ];
			read(wake_sds[0], c, sizeof(c));
		}

		/*
		 *  Check for new connections 
		 */

		if (FD_ISSET(srv->listen_fd, &rfds)) {

			if (1)
				timestamp("New connection");

			len = sizeof(client_addr);
			client =
			    accept(srv->listen_fd, (SA *) & client_addr, &len);

			if (client < 0) {
				if (errno != EINTR)
					traceLog("Accept error: %.100s",
						 strerror(errno));
				continue;
			}

			DEBUG(SPOCP_DSRV) traceLog("Got connection on fd=%d",
						   client);
			if (XYZ)
				timestamp("Accepted");

			val = fcntl(client, F_GETFL, 0);
			if (val == -1) {
				traceLog
				    ("Unable to get file descriptor flags on fd=%d: %s",
				     client, strerror(errno));
				close(client);
				goto fdloop;
			}

			/*
			 * if( fcntl( client, F_SETFL, val|O_NONBLOCK ) == -1) 
			 * { traceLog("Unable to set socket nonblock on fd=%d: 
			 * %s",client,strerror(errno)); close(client); goto
			 * fdloop; } 
			 */

			/*
			 * Get address not hostname of the connection 
			 */
			if ((err =
			     getnameinfo((SA *) & client_addr, len, hname,
					 NI_MAXHOST, NULL, 0, 0)) != 0
			    || (err =
				getnameinfo((SA *) & client_addr, len, ipaddr,
					    64, NULL, 0,
					    NI_NUMERICHOST)) != 0) {
				traceLog
				    ("Unable to getnameinfo for fd %d:%.100s",
				     client, strerror(err));
				close(client);
				goto fdloop;
			}

			if (strcmp(hname, "localhost") == 0) {
				strcpy(hname, srv->hostname);
			}

			if (srv->connections) {
				int             f, a;
				f = number_of_free(srv->connections);
				a = number_of_active(srv->connections);
				traceLog("Active: %d, Free: %d", a, f);
			} else {
				traceLog("NO connection pool !!!");
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
				traceLog
				    ("Unable to get free connection for fd=%d",
				     client);
				close(client);
			} else {
				/*
				 * initialize all the conn values 
				 */

				DEBUG(SPOCP_DSRV)
				    traceLog("Initializing connection to %s",
					     hname);
				conn_setup(conn, srv, client, hname, ipaddr);

				LOG(SPOCP_DEBUG)
				    traceLog("Doing server access check");

				if (server_access(conn) != SPOCP_SUCCESS) {
					traceLog
					    ("connection from %s(%s) DISALLOWED",
					     conn->fd, hname, ipaddr);

					conn_reset(conn);
					/*
					 * unlocks the conn struct as a side
					 * effect 
					 */
					item_return(srv->connections, pi);
					close(client);
				} else {
					traceLog
					    ("Accepted connection from %s on fd=%d",
					     hname, client);
					afpool_push_item(srv->connections, pi);

					pe++;
				}
			}
			if (1)
				timestamp("Done with new connection setup");
		}

		/*
		 * Main I/O loop: Check all filedescriptors and read and write
		 * nonblocking as needed.
		 */

	      fdloop:
		for (pi = afpool_first(srv->connections); pi; pi = pi->next) {
			spocp_result_t  res;
			proto_op       *operation = 0;
			/*
			 * int f = 0 ; 
			 */

			conn = (conn_t *) pi->info;

			if (conn->status == CNST_SSL_NEG)
				continue;

			/*
			 * wasn't around when the select was done 
			 */
			if (conn->fd > maxfd) {
				if (XYZ)
					traceLog("Not this time ! Too young");
				continue;
			}

			/*
			 * Don't have to lock the connection, locking the io
			 * bufferts are quite sufficient if(( err =
			 * pthread_mutex_lock(&conn->clock)) != 0)
			 * traceLog("Panic: unable to obtain lock on
			 * connection %d: %s", conn->fd,strerror(err)); 
			 */

			if (FD_ISSET(conn->fd, &rfds) && !conn->stop) {
				if (XYZ)
					traceLog("input readable on %d",
					    conn->fd);

				/*
				 * read returns number of bytes read 
				 */
				n = spocp_conn_read(conn);
				if (XYZ)
					traceLog("Read returned %d from %d", n,
						 conn->fd);
				/*
				 * traceLog( "[%s]", conn->in->buf ) ; 
				 */
				if (n == 0) {	/* connection probably
						 * terminated by other side */
					conn->stop = 1;
					continue;
				}
				if (XYZ)
					timestamp("read con");

				res = get_operation(conn, &operation);
				if (1)
					traceLog("Getops returned %d", res);
				if (res != SPOCP_SUCCESS)
					continue;

				if (XYZ)
					timestamp("add work item");
				/*
				 * this will not happen until the for loop is
				 * done 
				 */
				tpool_add_work(srv->work, operation,
					       (void *) conn);
			}

			if (FD_ISSET(conn->fd, &wfds)) {

				DEBUG(SPOCP_DSRV) {
					n = iobuf_content(conn->out);
					traceLog(
					    "Outgoing data on fd %d (%d)\n",
					    conn->fd, n);
				}

				n = spocp_conn_write(conn);

				if (XYZ)
					traceLog("Wrote %d on %d",
					   n, conn->fd);

				if (n == -1)
					traceLog
					    ("Error in writing to %d",
					     conn->fd);
				else if (iobuf_content(conn->out) == 0)
					conn->status = CNST_ACTIVE;
			}

			/*
			 * if(( err = pthread_mutex_unlock(&conn->clock)) !=
			 * 0) traceLog("Panic: unable to release lock on
			 * connection %d: %s", conn->fd,strerror(err)); 
			 */

		}
		if (XYZ)
			timestamp("one loop done");
	}
}
