/***************************************************************************
                           conn.c  -  description
                             -------------------
    copyright            : (C) 2010 by Umeå university, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 **************************************************************************/

#include "locl.h"

int             conn_readn(conn_t * ct, char *str, size_t max);
int             conn_writen(conn_t * ct, char *str, size_t n);
int             conn_close(conn_t * conn);

void            conn_iobuf_clear(conn_t * con);

#define _CONN_FLAG 1

/*
 * ---------------------------------------------------------------------- 
 */

void
conn_init(conn_t * conn)
{
	/*
	 * traceLog(LOG_INFO,"Init Connection") ; 
	 */
	conn->in = iobuf_new(SPOCP_IOBUFSIZE);
	conn->out = iobuf_new(SPOCP_IOBUFSIZE);

	conn->readn = conn_readn;
	conn->writen = conn_writen;
	conn->close = conn_close;
	if( pthread_mutex_init(&conn->clock, 0) != 0 )
		traceLog(LOG_WARNING, "mutex init of connection lock failed"); 
	if( pthread_mutex_init(&conn->rlock, 0) != 0 )
		traceLog(LOG_WARNING, "mutex init of reply queue lock failed"); 

}

conn_t         *
conn_new(void)
{
	conn_t         *con = 0;

	con = (conn_t *) Calloc(1, sizeof(conn_t));

	conn_init(con);

	return con;
}

/*
 * ---------------------------------------------------------------------- 
 */

int
conn_close(conn_t * conn)
{
	traceLog(LOG_INFO,"connection on %d closed", conn->fd);
	close(conn->fd);
	conn->fd = 0;

	return TRUE;
}

static void
conn_env_reset(conn_t * con)
{
	if (con) {
#ifdef HAVE_SSL
		if (con->subjectDN) {
			Free(con->subjectDN);
			con->subjectDN = 0;
		}
		if (con->issuerDN) {
			Free(con->issuerDN);
			con->issuerDN = 0;
		}
		if (con->cipher) {
			Free(con->cipher);
			con->cipher = 0;
		}
		if (con->ssl_vers) {
			Free(con->ssl_vers);
			con->ssl_vers = 0;
		}
		if (con->transpsec) {
			Free(con->transpsec);
			con->transpsec = 0;
		}
#endif
#ifdef HAVE_SASL
		if (con->sasl_mech) {
			Free(con->sasl_mech);
			con->sasl_mech = 0;
		}
		if (con->sasl) {
			sasl_dispose(&con->sasl);
			con->sasl = NULL;
		}
		con->sasl_ssf = NULL;
#endif
	}
}

static void 
spocp_req_info_free( spocp_req_info_t *sri )
{
	if (sri) {
		if (sri->hostname) {
			Free(sri->hostname);
		}
		if (sri->hostaddr) {
			Free(sri->hostaddr);
		}
		if (sri->invhost) {
			Free(sri->invhost);
		}
		if (sri->subject) {
			oct_free( sri->subject );
		}
		if (sri->sexp_subject) {
			Free( sri->sexp_subject );
		}
		memset( sri, 0, sizeof( spocp_req_info_t ));
	}
}

void
conn_reset(conn_t * conn)
{
	if (conn) {
		if (conn->fd)
			conn_close(conn);

		conn->status = CNST_FREE;
		conn->sslstatus = INACTIVE;
		conn->fd = 0;
		conn->operations = 0;
		conn_env_reset(conn);
		spocp_req_info_free( &conn->sri );

		conn_iobuf_clear(conn);
	}
}

void
conn_free(conn_t * con)
{
	if (con) {
		conn_reset(con);
		iobuf_free(con->in);
		iobuf_free(con->out);
		pthread_mutex_destroy(&con->clock);
		Free(con);
	}
}

/*
 * ---------------------------------------------------------------------- 
 */

int
conn_setup(conn_t * conn, srv_t * srv, int fd, char *hostname, char *ipaddr)
{
	conn->srv = srv;
	conn->fd = fd;
	conn->status = CNST_SETUP;
	conn->sslstatus = INACTIVE;
	conn->con_type = NATIVE;

	if (ipaddr && *ipaddr){
		conn->sri.hostaddr = Strdup(ipaddr);
	}
	else
		conn->sri.hostaddr = 0;

	if (hostname && *hostname) {
		conn->sri.hostname = Strdup(hostname);
	}
	else
		conn->sri.hostname = 0;

	if (srv->dback)
		conn->dbc.dback = srv->dback;

	conn->rs = srv->root;
	conn->layer = SPOCP_LAYER_NONE;
	conn->stop = 0;
#ifdef HAVE_SSL
	conn->ssl = NULL;
#endif

	time(&conn->last_event);

	return 0;
}

/*
 * ---------------------------------------------------------------------- 
 */

void
conn_iobuf_clear(conn_t * con)
{
	iobuf_flush(con->in);
	iobuf_flush(con->out);
}

/*
 * ---------------------------------------------------------------------- 
 */

char           *
next_line(conn_t * conn)
{
	char           *s, *b;
	spocp_iobuf_t  *io = conn->in;

	if ((s = strpbrk(io->r, "\r\n")) == 0) {	/* try to read some more */
		spocp_conn_read(conn);
		if ((s = strpbrk(io->r, "\r\n")) == 0) {	/* fail !! */
			return 0;
		}
	}

	if (*s == '\r' && *(s + 1) == '\n') {
		*s = '\0';
		s += 2;
	} else if (*s == '\n')
		*s++ = '\0';
	else
		return 0;

	b = io->r;
	io->r = s;

	return b;
}

int
spocp_send_results(conn_t * conn)
{
	return send_results(conn);
}

/*
 * ---------------------------------------------------------------------- 
 */

int
conn_writen(conn_t * ct, char *str, size_t n)
{
	int             nleft, nwritten;
	char           *sp;
	fd_set          wset;
	int             retval;
	struct timeval  to;
	int             fd = ct->fd;

	/*
	 * can happen during testing; it's hard to write to stdin so just fake 
	 * it 
	 */

	if (fd == STDIN_FILENO) {
		octet_t         oct;

		oct.val = str;
		oct.len = n;

		return (n);
	}

	errno = 0;

	/*
	 * wait max 3 seconds 
	 */
	to.tv_sec = 3;
	to.tv_usec = 0;

	sp = str;
	nleft = n;
	FD_ZERO(&wset);
	while (nleft > 0) {
		FD_SET(fd, &wset);
		retval = select(fd + 1, NULL, &wset, NULL, &to);

		if (retval) {
			if ((nwritten = write(fd, sp, nleft)) <= 0) {;
				if (errno == EINTR)
					nwritten = 0;
				else
					return -1;
			}

			nleft -= nwritten;
			sp += nwritten;
		} else {	/* timed out */
			break;
		}
	}

	/*
	 * fdatasync( fd ) ; 
	 */

	return (n);
}

/*
 * ---------------------------------------------------------------------- 
 */

int
conn_readn(conn_t * ct, char *str, size_t max)
{
	fd_set          rset;
	int             retval, n = 0;
	struct timeval  to;
	int             fd = ct->fd;

	errno = 0;

	/*
	 * wait max 0.1 seconds 
	 */
	to.tv_sec = 0;
	to.tv_usec = 100000;

	FD_ZERO(&rset);

	FD_SET(fd, &rset);
	retval = select(fd + 1, &rset, NULL, NULL, &to);

	if (retval) {
		if ((n = read(fd, str, max)) <= 0) {
			if (errno == EINTR || errno == 0)
				n = 0;
			else
				n = -1;
		}
#ifdef HAVE_SASL
    else if(ct->srv->use_sasl)
		{
			if((ct->sasl_ssf && *ct->sasl_ssf > 0) && n > 0)
			{
				const char  *plaindata;
				unsigned    plainlen;
                
				int wr = sasl_decode(ct->sasl, str, n, &plaindata, &plainlen);
				if(wr == SASL_OK)
				{
					if(plainlen <= max)
					{
						memcpy(str, plaindata, plainlen);
						n = plainlen;
					}
					else
						n = 0;
				}
				else
				{
					n = -1;
					ct->stop = 1;
				}
			}
		}
#endif

	} else {		/* timed out */
		;
	}

	/*
	 * fdatasync( fd ) ; 
	 */

	return (n);
}

/*
 * ---------------------------------------------------------------------- 
 */

int
spocp_conn_read(conn_t * conn)
{
	int             n;
	spocp_iobuf_t  *io = conn->in;

	pthread_mutex_lock(&io->lock);

#ifdef _CONN_FLAG
    traceLog(LOG_DEBUG,"is00 [buf]%p [r]%p [w]%p [left]%d",
            io->buf, io->r, io->w, io->left);
    traceLog(LOG_DEBUG, ".... r-buf:%d w-r:%d w-buf:%d", 
             io->r - io->buf, io->w - io->r, io->w - io->buf );
#endif 
    
	n = conn->readn(conn, io->w, io->left);
	if (n > 0) {
#ifdef SASL
		if(conn->sasl_ssf && *conn->sasl_ssf > 0)
		{
			char *plain_read;
			size_t plain_len;
			sasl_decode(conn->sasl,
					io->w, n,
					&plain_read, &plain_len);
			if((io->left - plain_len) >= 0)
			{
				strncpy(io->w, plain_read, plain_len);
				n = plain_len;
			}
			else
				n = 0;
		}
#endif

		io->left -= n;
		io->w += n;
		*io->w = '\0';
		/*
		   traceLog(LOG_DEBUG, "spocp_conn_read: %d %d", io->left, io->w - io->buf);
		 */
	}

	pthread_mutex_unlock(&io->lock);

	return n;
}

int
spocp_conn_write(conn_t * conn)
{
	int             n, l;
	spocp_iobuf_t  *out = conn->out;

	pthread_mutex_lock(&out->lock);
#ifdef HAVE_SASL
	if(conn->srv->use_sasl && conn->sasl_ssf && *conn->sasl_ssf > 0)
	{
		const int *maxout;
		char *plain_data = (char *) Calloc(out->w - out->r,sizeof(char));
		char *plain_point = plain_data;
		char *crypt_data = Malloc(1);        
		size_t plain_len = out->w - out->r;
		size_t tot_len = 0;
		int bufdiff;
		memcpy(plain_data, out->r, out->w - out->r);

		sasl_getprop(conn->sasl, SASL_MAXOUTBUF, (const void**) &maxout);

		do
		{
			size_t tmp_len = plain_len;
			size_t crypt_len;
			const char *tmp_data;
			if(plain_len > *maxout)
				tmp_len = *maxout;
			sasl_encode(conn->sasl,
					plain_point, tmp_len,
					(const char **)&tmp_data, &crypt_len);
			crypt_data = Realloc(crypt_data, crypt_len);
			memcpy(crypt_data, tmp_data, crypt_len);
			tot_len += crypt_len;
			plain_len -= tmp_len;
		} while(plain_len > 0);
		memcpy(out->r, crypt_data, tot_len);
		bufdiff = tot_len - (out->w - out->r);
		out->w += bufdiff;
		out->left -= bufdiff;
		Free(crypt_data);
	}
#endif

	n = conn->writen(conn, out->r, out->w - out->r);

	if (n >= 0) {
		DEBUG(SPOCP_DSRV)
			traceLog(LOG_DEBUG,"spocp_io_write wrote %d bytes", n);

		out->r += n;
		out->left -= n;

		/*
		 * have to unlock since iobuf_shift will try to lock 
		 */
		pthread_mutex_unlock(&out->lock);
		iobuf_shift(out);

		if ((l = iobuf_content(out)) == 0) {
			DEBUG(SPOCP_DSRV)
			    traceLog(LOG_DEBUG,"Wrote everything in buffer");
			pthread_cond_broadcast(&out->empty);
		} else
			DEBUG(SPOCP_DSRV) traceLog(LOG_DEBUG,"%d left in buffert", l);
	} else
		pthread_mutex_unlock(&out->lock);

	if (0)
		timestamp("Done writing");
	return n;
}

/*
 * ---------------------------------------------------------------------- 
 */

int
send_results(conn_t * conn)
{
	spocp_iobuf_t  *out = conn->out;

#ifdef AVLUS
	timestamp("Send results");
#endif

	if (gather_replies( out, conn ) == 0)
		return 1;

	LOG(SPOCP_INFO) {
		char	*tmp;

		tmp = str_esc( out->r, out->w - out->r );
		traceLog(LOG_INFO,"[%d]SEND_RESULT: [%s]", conn->fd, tmp);
		Free( tmp );
	}

#ifdef AVLUS
	timestamp("Result placed in outqueue");
#endif

	conn->status = CNST_WRITE;

	return 1;
}

/*
 * ======================================================================
 */

int init_reply_list( work_info_t *wi)
{
	reply_t *rp;
	conn_t  *conn = wi->conn; 
    
	rp = reply_new( wi );
	
	pthread_mutex_lock( &conn->rlock );
	
	conn->head = reply_push( conn->head, rp );
    
	pthread_mutex_unlock( &conn->rlock );
    
	return 1;
}

/*
 * ---------------------------------------------------------------------- 
 */

int
gather_replies( spocp_iobuf_t *out, conn_t *conn )
{
	reply_t		*rp ;
	spocp_iobuf_t	*buf;
	spocp_result_t	sr;
	int		r = 0;
    
	pthread_mutex_lock( &conn->rlock );
	
	if (conn->head == 0) {
		pthread_mutex_unlock( &conn->rlock );
		return r;
	}
    
	while ( conn->head && conn->head->buf ) {
		r++;
		rp = reply_pop( &conn->head );
		buf = rp->buf;
		sr = iobuf_addn(out, buf->r, buf->w - buf->r);
		reply_free( rp );
	}
    
	LOG(SPOCP_INFO){
		if (r) {
			char *tmp;
            
			tmp = str_esc( out->r, out->w - out->r );
			traceLog(LOG_INFO, "Replies(%d): [%s]", r, tmp );
			Free( tmp );
		}
	}
    
	pthread_mutex_unlock( &conn->rlock );
    
	return r;
}
