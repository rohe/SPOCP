/***************************************************************************
                          be_localgroup.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Stockholm university, Sweden
    email                : roland@it.su.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

/* localgroup:leifj:foo:[/etc/group] */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <grp.h>
#include <sys/types.h>

#include <be.h>
#include <plugin.h>
#include <rvapi.h>
#include "../../server/srvconf.h"

#define ETC_GROUP "/etc/group"

befunc localgroup_test ;

static int _ismember(octet_t *member,char *grp)
{
  char **p ;
  int  i, n, r = FALSE ;

  if( grp == 0 || *grp == 0 ) return FALSE ;

  p = line_split( grp, ',', 0, 0, 0, &n ) ;

  for (i = 0 ; r == FALSE && i <= n ; i++ ) {
    if (oct2strcmp(member, p[i] ) == 0 ) r = TRUE;
  }

  charmatrix_free( p ) ;

  return r ;
}

/* format of group files */
/*
   /etc/group  is an ASCII file which defines the groups to which users belong.  There
   is one entry per line, and each line has the format:

      group_name:passwd:GID:user_list

   The query arg should look like this group_name:user[:file] 

*/

spocp_result_t localgroup_test(
  element_t *qp, element_t *rp, element_t *xp, octet_t *arg, pdyn_t *dyn, octet_t *blob )
{
  spocp_result_t rc = SPOCP_DENIED ;

  FILE     *fp;
  octarr_t *argv ;
  octet_t  *oct, cb, *group, *user, loc ;
  char     *fn = ETC_GROUP, *cp;
  int       done, cv ;
  char      buf[BUFSIZ] ;
  becon_t   *bc = 0 ;

  if( arg == 0 ) return SPOCP_MISSING_ARG ;

  if(( oct = element_atom_sub( arg, xp )) == 0 ) return SPOCP_SYNTAXERROR ;

  cv = cached( dyn->cv, oct, &cb ) ;

  if( cv ) {
    /* traceLog( "localgroup: cache hit" ) ; */

    if( cv == EXPIRED ) {
      cached_rm( dyn->cv, oct ) ;
      cv = 0 ;
    }
  }

  if( cv == 0 ) {
    argv = oct_split( oct, ':', 0, 0, 0 ) ;
  
    /* LOG( SPOCP_DEBUG ) traceLog("n=%d",n); */
  
    if ( argv->n == 3 ) fn = oct2strdup( argv->arr[2], 0 ) ;
  
    /* LOG( SPOCP_DEBUG ) traceLog("groupfile: %s",fn); */
  
    oct_assign( &loc, fn ) ;
  
    if( ( bc = becon_get( &loc, dyn->bcp )) == 0 ) {
      if (( fp = fopen(fn,"r")) == 0) rc = SPOCP_UNAVAILABLE ;
      else if( dyn->size ) {
        if( !dyn->bcp ) dyn->bcp = becpool_new( dyn->size, 0 ) ;
        bc = becon_push( &loc, &P_fclose, (void *) fp, dyn->bcp ) ;
      }
    }
    else {
      fp = ( FILE * ) bc->con ;
      if( fseek( fp, 0L, SEEK_SET) == -1 ) {
        /* try to reopen */
        fp = fopen(fn, "r") ;
        if( fp == 0 ) { /* not available */
          becon_rm( dyn->bcp, bc ) ;
          bc = 0 ;
          rc = SPOCP_UNAVAILABLE ;
        }
        else becon_update( bc, (void *) fp ) ;
      }
    }
    
    /* LOG( SPOCP_DEBUG ) traceLog("open...%s",fn); */
  
    if( rc == SPOCP_DENIED || argv->n < 2 ) {
      done = 0;
      group = argv->arr[0] ;
      user  = argv->arr[1] ;
  
      while ( !done && fgets( buf, BUFSIZ, fp ) ) {
        rmcrlf( buf ) ;
        if(( cp = index( buf, ':' )) == 0 ) continue ; /* incorrect line */
    
        *cp++ = '\0' ;
    
        /* LOG( SPOCP_DEBUG ) traceLog("grp: %s", buf ); */
    
        if(( oct2strcmp(group, buf)) == 0) {
          /* skip passwd */
          if(( cp = index( cp, ':' )) == 0 ) continue ; /* incorrect line */ 
          cp++ ;
          /* skip GID */
          if(( cp = index( cp, ':' )) == 0 ) continue ; /* incorrect line */ 
          cp++ ;
    
          if( _ismember(user,cp) == TRUE ) rc = SPOCP_SUCCESS ;
          done++;
        }
    
      }
    }
    
    if( bc ) becon_return( bc ) ;
    else if( fp ) fclose(fp);
  
    if( fn != ETC_GROUP ) free( fn ) ;
  
    octarr_free( argv );
  }

  if( cv == (CACHED|SPOCP_SUCCESS) ) {
    if( cb.len ) octln( blob, &cb ) ;
    rc = SPOCP_SUCCESS ;
  }
  else if( cv == ( CACHED|SPOCP_DENIED ) ) {
    rc = SPOCP_DENIED ;
  }
  else {
    if( dyn->ct && ( rc == SPOCP_SUCCESS || rc == SPOCP_DENIED )) {
      time_t t ;
      t = cachetime_set( oct, dyn->ct ) ;
      dyn->cv = cache_value( dyn->cv, oct, t, (rc|CACHED) , 0 ) ;
    }
  }

  if( oct != arg ) oct_free( oct ) ;
  
  return rc;
}

/* The callbacks to get configuration information is just there as examples */

spocp_result_t localgroup_init( confgetfn *cgf, void *conf, becpool_t *bcp )
{
  char      *value = 0 ;
  int       *num = 0, i ;
  octarr_t  *on = 0 ;
  void      *vp ;

  /* using vp to avoid warnings about type-punned pointers */
  cgf( conf, PLUGIN, "localgroup", &vp ) ;
  if( vp ) on = ( octarr_t *) vp ;

  LOG( SPOCP_DEBUG ) {
    for( i = 0 ; i < on->n ; i++ ) {
      traceLog( "localgroup key[%d] : [%s]", i, on->arr[i]->val ) ;
    }
  }
  octarr_free( on ) ;

  cgf( conf, TIMEOUT, 0, &vp ) ;
  if( vp ) num = ( int *) vp ;

/*
  traceLog( "TIMEOUT: [%d]", *num ) ;
*/
  
  cgf( conf, CALIST, 0, &vp ) ;
  if( vp ) value = ( char *) vp ;

/*
  if( value ) traceLog( "CALIST: [%s]", value ) ;
*/

  return SPOCP_SUCCESS ;
}

