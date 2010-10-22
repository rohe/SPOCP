
/***************************************************************************
                           iobuf.c  -  description
                             -------------------
    copyright            : (C) 2010 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 **************************************************************************/

#include "locl.h"

/*!
 * \brief Creates a iobuf struct
 * \param size The size of the character buffer of the io struct
 * \return A newly created spocp_iobuf_t struct.
 */
spocp_iobuf_t  *
iobuf_new(size_t size)
{
	spocp_iobuf_t  *io;

	io = (spocp_iobuf_t *) Calloc(1, sizeof(spocp_iobuf_t));

	io->w = io->r = io->buf = (char *) Calloc(size, sizeof(char));

	io->left = size - 1;	/* leave place for a terminating '\0' */
	io->bsize = size;

	io->end = io->buf + io->bsize;
	*io->w = '\0';

	pthread_mutex_init(&io->lock, NULL);
	pthread_cond_init(&io->empty, NULL);

	return io;
}

void 
iobuf_free( spocp_iobuf_t *io)
{
	if (io) {
		if (io->bsize)
			Free( io->buf );
		Free(io);
	}
}

/*!
 * \brief Shifts what's unread to the beginning of the buffer.
 * \param io Pointer to the iobuf struct
 */

void
iobuf_shift(spocp_iobuf_t * io)
{
	int             len;

	pthread_mutex_lock(&io->lock);

    /* why do I need this ?? */
	while (io->r <= io->w && WHITESPACE(*io->r))
		io->r++;

	if (io->r >= io->w) {	/* nothing in buffer */
		io->r = io->w = io->buf;
		io->left = io->bsize - 1;
		*io->w = 0;
	} else {		/* something in the buffer, shift it to the front */
		len = io->w - io->r;
		memmove(io->buf, io->r, len);
		io->r = io->buf;
		io->w = io->buf + len;
		io->left = io->bsize - len - 1;
		*io->w = '\0';
	}

	DEBUG(SPOCP_DSRV)
		traceLog(LOG_DEBUG,"DONE Shifting buffer b:%p r:%p w:%p left:%d",
		    io->buf, io->r, io->w, io->left);

	pthread_mutex_unlock(&io->lock);

}

spocp_result_t
iobuf_resize(spocp_iobuf_t * io, int increase, int lock)
{
	int             nr, nw;
	char           *tmp;

	if (io == 0)
		return SPOCP_OPERATIONSERROR;

	if (io->bsize == SPOCP_MAXBUF)
		return SPOCP_BUF_OVERFLOW;

	if (lock)
		pthread_mutex_lock(&io->lock);

	nr = io->bsize;

	if (increase) {
		if (io->bsize + increase > SPOCP_MAXBUF)
			return SPOCP_BUF_OVERFLOW;
		else if (increase < SPOCP_IOBUFSIZE)
			io->bsize += SPOCP_IOBUFSIZE;
		else
			io->bsize += increase;
	} else {
		if (io->bsize + SPOCP_IOBUFSIZE > SPOCP_MAXBUF)
			io->bsize += SPOCP_MAXBUF;
		else
			io->bsize += SPOCP_IOBUFSIZE;
	}

	/*
	 * the increase should be usable 
	 */
	io->left += io->bsize - nr;

	nr = io->r - io->buf;
	nw = io->w - io->buf;

	tmp = Realloc(io->buf, io->bsize);

	io->buf = tmp;
	io->end = io->buf + io->bsize -1;
	*io->end = '\0';
	io->r = io->buf + nr;
	io->w = io->buf + nw;

	if (lock)
		pthread_mutex_unlock(&io->lock);

	return SPOCP_SUCCESS;
}

static spocp_result_t
_add(spocp_iobuf_t * io, char *s, int n)
{
	spocp_result_t  rc = SPOCP_SUCCESS;

	pthread_mutex_lock(&io->lock);
    
    /*
     * are the place enough ? If not make some 
     */
    if ((int) io->left < n) {
        if ((rc = iobuf_resize(io, n - io->left, 0)) != SPOCP_SUCCESS) {
            pthread_mutex_unlock(&io->lock);
            return rc;
        }
    }

    memcpy(io->w, s, n);

    io->w += n;
    *io->w = '\0';
    io->left -= n;


    pthread_mutex_unlock(&io->lock);
    return rc;
}

spocp_result_t
iobuf_add(spocp_iobuf_t * io, char *s)
{
	if (s == 0 || *s == 0)
		return SPOCP_SUCCESS;	/* no error */

	if (io == 0)
		return SPOCP_SUCCESS;   /* Eh ? */

    return _add(io, s, strlen(s));
}

spocp_result_t
iobuf_addn(spocp_iobuf_t * io, char *s, size_t n)
{
	if (s == 0 || *s == 0)
		return SPOCP_SUCCESS;	/* no error */

    return _add(io, s, n);
}

spocp_result_t
iobuf_add_octet(spocp_iobuf_t * io, octet_t * s)
{
	spocp_result_t  rc;
	char            lenfield[8];

	if (s == 0 || s->len == 0)
		return SPOCP_SUCCESS;	/* adding nothing is no error */

	if (io == 0)
		return SPOCP_SUCCESS; /* happens with native_server */ 

	snprintf(lenfield, 8, "%u:", (unsigned int) s->len);

    if ((rc = _add(io, lenfield, strlen(lenfield))) != SPOCP_SUCCESS)
        return rc;
    
    return _add(io, s->val, s->len);
}

/*!
 * \brief resets the iobuf buffer
 * \param Pointer to a iobuf struct
 */

void
iobuf_flush(spocp_iobuf_t * io)
{
	pthread_mutex_lock(&io->lock);

	io->r = io->w = io->buf;
	*io->w = 0;
	io->left = io->bsize - 1;

	pthread_mutex_unlock(&io->lock);

}

/*!
 * \brief returns a value designating whether there is something in the
 *  iobuf or not.
 * \param io Pointer to the iobuf struct.
 * \return -1 if there is not iobuf struct, 0 if the buffer is empty and
 *  a value bigger than 0 if there is something in the buffer.
 */

int
iobuf_content(spocp_iobuf_t * io)
{
	int             n;

	if (io == 0)
		return -1;

	pthread_mutex_lock(&io->lock);
	n = (io->w - io->r);
	pthread_mutex_unlock(&io->lock);

	return n;
}

void 
iobuf_print( char *tag, spocp_iobuf_t *io )
{
	octet_t o;

	if (io != 0 ) {
		o.val = io->r;
		o.len = io->w - io->r;
		oct_print( LOG_DEBUG, tag, &o );
	}
	else 
		traceLog(LOG_DEBUG,"%s: %s", tag, "Empty");

}

void
iobuf_info( spocp_iobuf_t *io )
{
	traceLog(LOG_DEBUG,"----iobuf info----");
	if (io) {
		traceLog(LOG_DEBUG,"bsize: %d", io->bsize );
		traceLog(LOG_DEBUG,"left: %d", io->left);
		traceLog(LOG_DEBUG,"left: %d", io->w - io->r);

		traceLog(LOG_DEBUG,"buf: %p", io->buf);
		traceLog(LOG_DEBUG,"r: %p", io->r);
		traceLog(LOG_DEBUG,"w: %p", io->w);
		traceLog(LOG_DEBUG,"end: %p", io->end);
	}
	else {
		traceLog(LOG_DEBUG,"NULL");
	}
	traceLog(LOG_DEBUG,"----end----");
}

