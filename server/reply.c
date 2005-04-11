
/***************************************************************************
                           reply.c  -  description
                             -------------------
    begin                : Mon Oct 25 2004
    copyright            : (C) 2004 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 **************************************************************************/

#include "locl.h"
RCSID("$Id$");

int r_f = 0;

static reply_t *
reply_new( work_info_t *wi )
{
	reply_t *rep;

	rep = ( reply_t * ) Calloc (1, sizeof( reply_t ));
	rep->wi = wi;

	return rep;
}

static void 
reply_free( reply_t *r)
{
	if (r) {
		iobuf_free( r->buf );
		Free( r );
	}
}

static reply_t *
reply_push( reply_t *head, reply_t *rep )
{
	reply_t *r;

	if (head == 0)
		return rep;

	for( r = head ; r->next ; r = r->next ) ;

	r->next = rep;

	return head;
}

static reply_t *
reply_pop( reply_t **head )
{
	reply_t	*r;

	if (*head == NULL)
		return NULL;

	r = *head ;
	*head = r->next;
	r->next = NULL;

	return r;
}

reply_t *
reply_add( reply_t *head, work_info_t *wi )
{
	reply_t *rp;
	char	*tmp;

	for( rp = head ; rp ; rp = rp->next )	
		if (rp->wi == wi) {
			rp->buf = wi->buf;

			tmp = str_esc( rp->buf->r, rp->buf->w - rp->buf->r );
			DEBUG(SPOCP_DMATCH)
				traceLog(LOG_INFO, "Adding reponse: [%s]", tmp );
			Free( tmp );

			wi->buf = 0;
			/* so noone will overwrite this */
			rp->wi = 0;
			break;
		}

	return rp;
}

int
gather_replies( spocp_iobuf_t *out, conn_t *conn )
{
	reply_t		*rp ;
	spocp_iobuf_t	*buf;
	spocp_result_t	sr;
	int		r = 0;

	if( r_f) 
		timestamp("Getting Read lock on connection");

	pthread_mutex_lock( &conn->rlock );
	
	if( r_f) 
		timestamp("Got it");

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

int add_reply_queuer( conn_t *conn, work_info_t *wi)
{
	reply_t 	*rp;
	
	rp = reply_new( wi );
	
	pthread_mutex_lock( &conn->rlock );
	
	conn->head = reply_push( conn->head, rp );

	pthread_mutex_unlock( &conn->rlock );

	return 1;
}


