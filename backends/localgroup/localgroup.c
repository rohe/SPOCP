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
#include <string.h>
#include <grp.h>
#include <sys/types.h>

#include <spocp.h>
#include <srvconf.h>

#define ETC_GROUP "/etc/group"

spocp_result_t localgroup_test( octet_t *arg, becpool_t *bcp, octet_t *blob ) ;

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

   The query arg should look like this group_name:user 

*/

spocp_result_t localgroup_test(octet_t *arg, becpool_t *bcp, octet_t *blob )
{
  spocp_result_t rc = SPOCP_DENIED ;

  FILE    *fp;
  octet_t **argv = NULL;
  char    *fn = ETC_GROUP, *cp;
  int     done, n ;
  char    buf[BUFSIZ] ;

  argv = oct_split(arg,':',0,0,0,&n);

  /* LOG( SPOCP_DEBUG ) traceLog("n=%d",n); */

  if (n == 3) fn = argv[2]->val;

  /* LOG( SPOCP_DEBUG ) traceLog("groupfile: %s",fn); */

  if (( fp = fopen(fn,"r")) == 0) {
    rc = SPOCP_UNAVAILABLE ;
    goto DONE;
  }
  
  /* LOG( SPOCP_DEBUG ) traceLog("open...%s",fn); */

  done = 0;

  while ( !done && fgets( buf, BUFSIZ, fp ) ) {
    rmcrlf( buf ) ;
    if(( cp = index( buf, ':' )) == 0 ) continue ; /* incorrect line */

    *cp++ = '\0' ;

    /* LOG( SPOCP_DEBUG ) traceLog("grp: %s", buf ); */

    if(( oct2strcmp(argv[0], buf)) == 0) {
      /* skip passwd */
      if(( cp = index( cp, ':' )) == 0 ) continue ; /* incorrect line */ 
      cp++ ;
      /* skip GID */
      if(( cp = index( cp, ':' )) == 0 ) continue ; /* incorrect line */ 
      cp++ ;

      if( _ismember(argv[1],cp) == TRUE ) rc = SPOCP_SUCCESS ;
      done++;
    }

  }
  
DONE:
  if( fp ) fclose(fp);

  if (argv != NULL)
    oct_freearr( argv );

  return rc;
}

/* The callbacks to get configuration information is just there as examples */

spocp_result_t localgroup_init( confgetfn *cgf, void *conf, becpool_t *bcp )
{
  char      *value = 0 ;
  int       *num = 0 ;
  octnode_t *on ;
  void      *vp ;

  /* LOG( SPOCP_DEBUG) traceLog( "localgroup_init" ) ; */

  /* using vp to avoid warnings about type-punned pointers */
  cgf( conf, PLUGIN, "localgroup", &vp ) ;
  if( vp ) on = ( octnode_t *) vp ;

/*
  for( ; on ; on = on->next ) {
    traceLog( "localgroup[%d] : [%s]", i, on->oct->val ) ;
  }
*/

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

