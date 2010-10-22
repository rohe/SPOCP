
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

int r_f = 0;

reply_t *
reply_new( work_info_t *wi )
{
	reply_t *rep;

	rep = ( reply_t * ) Calloc (1, sizeof( reply_t ));
	rep->wi = wi;

	return rep;
}

void 
reply_free( reply_t *r)
{
	if (r) {
        if (r->buf)
            iobuf_free( r->buf );
		Free( r );
	}
}

/*!
 * \brief Push a result item onto the end of a result item list
 * \param head Pointer to the head of the list
 * \param rep Pointer to the item that should be added
 * \return Pointer to the head of the list
 */
reply_t *
reply_push( reply_t *head, reply_t *rep )
{
	reply_t *r;

	if (head == 0)
		return rep;

	for( r = head ; r->next ; r = r->next ) ;

	r->next = rep;

	return head;
}

/*!
 * \brief Removes and returns the first repy_t instance in a 
 *  list of such instances.
 * \param head Pointer to the pointer to the first item in the linked list.
 *  This pointer will afterwards point to the second item.
 * \return Pointer to the previous first intances. Or NULL if the list was
 *  empty.
 */
reply_t *
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

/*!
 * \brief Moves a iobuf struct from the work item to the appropriate reply
 *  struct.
 * \param head The first instance in the reply list.
 * \param wi The work item from which the iobuf struct should be gotten.
 * \return Pointer to the return instance which was modified or NULL if
 *  no appropriate reply instance was found.
 */
reply_t *
reply_add( reply_t *head, work_info_t *wi )
{
	reply_t *rp;

	for( rp = head ; rp ; rp = rp->next )	
		if (rp->wi == wi) {
			rp->buf = wi->buf;

			DEBUG(SPOCP_DMATCH) {
                char	*tmp;
                tmp = str_esc( rp->buf->r, rp->buf->w - rp->buf->r );
				traceLog(LOG_INFO, "Adding reponse: [%s]", tmp );
                Free( tmp );
            }

			wi->buf = 0;
			/* so noone will overwrite this */
			rp->wi = 0;
			break;
		}

	return rp;
}

