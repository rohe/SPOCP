/***************************************************************************
                          extref.c  -  description
                             -------------------
    begin                : Fri Nov 1 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>
#include <sys/types.h>

#include <stdlib.h>
#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif

#include <struct.h>
#include <func.h>
#include <spocp.h>
#include <macros.h>

parr_t *erset_match( erset_t *es, element_t *ep, parr_t *ava, becpool_t *bcp )
{
  size_t    i ;
  parr_t   *pa = 0 ;

  /* walk through all the boundary conditions */

  for(  ; es ; es = es->other ) {
    for( i = 0 ; i < es->n ; i++ ) {
      if( do_ref( es->ref[i], ava, bcp, 0 ) == TRUE )
        pa = parr_add( pa, (void *) es->next ) ;
    }
  }

  return pa ;
}


