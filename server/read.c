#include <string.h>

#include <spocp.h>
#include <struct.h>
#include <wrappers.h>
#include <macros.h>
#include <func.h>

#include <srv.h>

typedef struct _ptree {
  int list ;
  octet_t val ;
  struct _ptree *next ;
  struct _ptree *part ;
} ptree_t ;


/* ----------------------------------------------------------------------------- */

static void ptree_free( ptree_t *ptp )
{
  if( ptp ) {
    if( ptp->part ) ptree_free( ptp->part ) ;
    if( ptp->next ) ptree_free( ptp->next ) ;
    if( ptp->val.size ) free( ptp->val.val ) ;
    free( ptp ) ;
  }
}
/* ----------------------------------------------------------------------------- */

static ptree_t *parse_sexp( octet_t *sexp , keyval_t **globals )
{
  ptree_t *ptp, *ntp = 0, *ptr ;
  octet_t  oct ;

  if( *sexp->val == '(' ) {
    ptp = ( ptree_t * ) calloc ( 1, sizeof( ptree_t )) ;
    ptp->list = 1 ;
    sexp->val++ ;
    sexp->len-- ;
    while( sexp->len && *sexp->val != ')' ) {
      if(( ptr = parse_sexp( sexp, globals )) == 0 ) {
        ptree_free( ptp ) ;
        return 0;
      }

      if( ptp->part == 0 ) ntp = ptp->part = ptr ;
      else {
        ntp->next = ptr ;
        ntp = ntp->next ;
      }
    }
    if( *sexp->val == ')' ) {
      sexp->val++ ;
      sexp->len-- ;
    }
    else { /* error */
      ptree_free( ptp ) ;
      return 0 ;
    }
  }
  else if( strncmp( sexp->val, "$$(", 3 ) == 0 ) {
    if( oct_expand( sexp, *globals, 1 ) != SPOCP_SUCCESS ) {
      return 0 ;
    }
    ptp = parse_sexp( sexp, globals ) ;    
  }
  else {
    ptp = ( ptree_t * ) calloc ( 1, sizeof( ptree_t )) ;
    if( get_str( sexp, &ptp->val ) != SPOCP_SUCCESS ) {
      ptree_free( ptp ) ;
      return 0 ;
    }
    if( octstr( &ptp->val, "$$(" ) >= 0 ) {
      oct.size = 0 ;
      octcpy( &oct, &ptp->val ) ;
      if( oct_expand( &oct, *globals, 1 ) != SPOCP_SUCCESS ) {
        ptree_free( ptp ) ;
        return 0 ;
      }
      ptp->val.val = oct.val ;
      ptp->val.size = oct.size ;
      ptp->val.len = oct.len ;
    }
  }
    
  return ptp ;
}

static int length_sexp ( ptree_t *ptp )
{
  int len = 0 ;
  ptree_t *pt ;

  if( ptp->list ) {
    len = 2 ;
    for( pt = ptp->part ; pt ; pt = pt->next ) len += length_sexp( pt ) ;
  }
  else {
    len = DIGITS( ptp->val.len ) ;
    len++ ;
    len += ptp->val.len ;
  }

  return len ;
}

static void recreate_sexp ( octet_t *o, ptree_t *ptp ) 
{
  ptree_t *pt ;
  int     len = 0 ;

  if( ptp->list ) {
    *o->val++ = '(' ;
    o->len++ ;
    for( pt = ptp->part ; pt ; pt = pt->next ) recreate_sexp( o, pt ) ;
    *o->val++ = ')' ;
    o->len++ ;
  }
  else {
    sprintf( o->val, "%d:", ptp->val.len ) ;
    len = DIGITS( ptp->val.len ) ;
    len++ ;
    o->len += len ;
    o->val += len ;
    strncpy( o->val, ptp->val.val, ptp->val.len ) ;
    o->len += ptp->val.len; 
    o->val += ptp->val.len; 
  }
}

/*
static octet_t *expand_sexp( octet_t *sexp, keyval_t **globals )
{
  int len ;
  ptree_t *ptp ;
  octet_t *oct ;
  char    *str ;

  if(( ptp = parse_sexp( sexp, globals )) == 0 ) return 0 ;
  len = length_sexp( ptp ) ;

  oct = ( octet_t * ) malloc ( sizeof( octet_t )) ;
  oct->val = str = ( char * ) calloc ( len, sizeof( char )) ;
  oct->size = len ;
  oct->len = 0 ;

  recreate_sexp( oct, ptp ) ;

  oct->val = str ;
  oct->size = 0 ;
  oct->len = strlen(str) ;

  ptree_free( ptp ) ;

  return oct ;
}
*/

/* ----------------------------------------------------------------------------- */

int spocp_add_global( keyval_t **kvpp, octet_t *key, octet_t *val ) 
{
  keyval_t *kvp ;

  if( key == 0 || key->len == 0 ) return 0 ;

  if( *kvpp == 0 ) {
    kvp = ( keyval_t *) Malloc( sizeof( keyval_t )) ;
    kvp->next = 0 ;
    *kvpp = kvp ;
  }
  else {
    for( kvp = *kvpp ; kvp->next ; kvp = kvp->next ) {
      if( octcmp( kvp->key, key ) == 0 ) return -1 ;
    }

    /* last check */
    if( octcmp( kvp->key, key ) == 0 ) {
      if( kvp->val ) oct_free( kvp->val ) ;  
      if( val ) kvp->val = octdup( val ) ;
      return 1 ;
    }

    kvp->next = ( keyval_t *) Malloc( sizeof( keyval_t )) ;
    kvp = kvp->next ;
  }

  kvp->key = octdup( key ) ;

  if( val && val->len != 0 ) kvp->val = octdup( val ) ;
  else kvp->val = 0 ;

  kvp->next = 0 ;

  return 1 ;
}

/* The format of the rule file

  spec       = line *( BSLASH CR line ) CR
  line       = ( rule / comment / "" / definition / bconddef / include ) 
  rule       = [ path ] sexp [ bcond [ blob ]]
  element    = sexp / atom / variable
  sexp       = "(" atom *element ")"
  blob       = atom 
  atom       = len ":" val
  variable   = "$$(" key ")"
  key        = 1*(%x2A-7E)
  comment    = "#" *UTF8  
  definition = key *SP "=" *SP 1*( sexp )
  bconddef   = key *SP ":=" *SP 1*( sexp )
  SP         = %x20 / %x09                      ; space or tab
  DIGIT      = %x30-39
  len        = %x31-39 *( DIGIT )
  val        = 1*( UTF8 / pair )
  include    = ";include" filename              ; reference to another rule file

  pair       = "\" ( "\" / hexpair )
  hexpair    = hexchar hexchar
  hexchar    = DIGIT / "A" / "B" / "C" / "D" / "E" / "F"
               / "a" / "b" / "c" / "d" / "e" / "f"

  path       = "/" *( dir "/" ) ;
  dir        = 1*pchar 
  pchar      = DIGIT / 0x41-5A / 0x61-7A / %x2E

  bcond      = bcexp / bcomp
  bcomp      = and / or / not / ref
  ref        = "(3:ref" bcname ")"
  and        = "(3:and" 2*( bcomp ) ")"
  or         = "(2:or" 2*( bcomp ) ")"
  not        = "(3:not" bcomp ")"
  CR         = %x0D
  BSLASH     = %x5C         
 */

void *read_rules( void *vp, char *file, int *rc, keyval_t **globals )
{
  FILE        *fp ;
  char        *sp, *cp, *tmp, buf[ BUFSIZ ] ;
  int         n = 0, f = 0, r, ln = 0, g ;
  size_t      l ;
  octet_t     oct, key, val, rsn, blob, bcond, *exp = 0, prev ;
  octarr_t    *oa = 0;
  ruleset_t   *rs = 0, *trs ;
  bcdef_t     *bcd = 0 ;

  if(( fp = fopen( file, "r" )) == 0 ) {
    LOG(SPOCP_EMERG) traceLog( "couldn't open rule file \"%s\"", file) ;
    sp = getcwd( oct.val, oct.size ) ;
    LOG(SPOCP_EMERG) traceLog("I'm in \"%s\"", sp ) ;
    *rc = -1 ;
    free( oct.val ) ;
    return vp ;
  }

  /* The default ruleset should already be set */

  if( vp == 0 ) {
    rs = ruleset_new( 0 ) ;
    vp = ( void * ) rs ;
  }
  else rs = (ruleset_t * ) vp ;

  prev.len = prev.size = 0 ;
  prev.val = 0 ;

  /* have to escape CR since fgets stops reading when it hits a newline
     NUL also has to be escaped since I have problem otherwise finding 
     the length of the 'string'.
     '\' hex hex is probably going to be the choice
   */
  while( fgets( buf, BUFSIZ, fp ) ) {
    exp = 0 ;
    oct_assign( &oct, buf ) ;

    sp = oct.val ;

    ln++ ;

    if( *sp == '#' ) continue ; /* comment */ 
    for( cp = sp + oct.len - 1 ; *cp == '\r' || *cp == '\n' ; cp-- ) {
      oct.len-- ;
      if( oct.len == 0 ) break ;
    }

    if( oct.len == 0 ) continue ;

    if( oct.val[ oct.len - 1 ] == '\\' ) {
      oct.len-- ;
      if( prev.size == 0 ) octcpy( &prev, &oct ) ;
      else octcat( &prev, oct.val, oct.len ) ;
      continue ;
    }

    if( prev.size ) {
      octcat( &prev, oct.val, oct.len ) ;
      octln( &oct, &prev ) ;
      sp = oct.val ;
    }

    LOG( SPOCP_DEBUG ) {
      tmp = oct2strdup( &oct, '%' ) ;
      traceLog( "line(%d): \"%s\"", ln, tmp ) ;
      free( tmp ) ;
    }

    if( oct_de_escape( &oct ) < 0 ) {
      traceLog( "Error in escape sequence" ) ;
      f++ ;
      octclr( &prev ) ;
      continue ;
    }
    oct.val[oct.len] = 0 ;

    if( oct2strncmp( &oct, ";include ", 9 ) == 0 ) { /* include file */
      LOG(SPOCP_DEBUG) traceLog( "include directive \"%s\"", sp+9) ;
      vp = read_rules( vp, sp+9, rc, globals ) ;
      octclr( &prev ) ;
      continue ;
    }

    if(( g = octstr( &oct, "$$(" )) >= 0 ) { 
      /* expand if possible */
      if( *globals ) {
        exp = octdup( &oct ) ;
        if( oct_expand( exp, *globals, 0 ) == SPOCP_SUCCESS ) {
          exp->val[exp->len] = 0 ;
          LOG( SPOCP_DEBUG ) traceLog("Expanded to: \"%s\"", exp->val ) ;
        }
        else  {
          LOG( SPOCP_WARNING ) traceLog("Failed to expand: \"%s\"", exp->val ) ;
          oct_free( exp ) ;
          f++ ;
          octclr( &prev ) ;
          continue ;
        }
        octln( &oct, exp ) ;
        sp = oct.val ;
      }
      else { /* check if expansion was needed */
        key.val = oct.val + g ;
        key.len = oct.len - g ;
        if( octchr( &key, ')' ) >= 0 ) {
          LOG( SPOCP_WARNING ) traceLog("Failed to expand: \"%s\"", oct.val ) ;
          f++ ;
          octclr( &prev ) ;
          continue ;
        }
      }
    }

    if( *sp == '/' ) {
      if( ruleset_name( &oct, &rsn ) != SPOCP_SUCCESS ) {
        LOG( SPOCP_EMERG ) traceLog( "Strange line \"%s\"", oct.val ) ;
        if( exp ) oct_free( exp ) ;
        octclr( &prev ) ;
        return 0 ;
      }

      trs = ruleset_create( &rsn, &rs ) ;
      LOG( SPOCP_DEBUG ) traceLog( "ruleset name: \"%s\"", trs->name ) ;
      sp = oct.val ;
    }
    else trs = rs ;

    while( *sp == ' ' || *sp == '\t' ) sp++ ;

    if( *sp == '(' ) {
      LOG(SPOCP_DEBUG) traceLog( "rule: \"%s\"", sp) ;

      if(( l = sexp_len( &oct )) == 0 ) {
        traceLog( "Error in the rulefile" ) ;
        ruleset_free( vp ) ;
        if( exp ) oct_free( exp ) ;
        octclr( &prev ) ;
        return 0 ;
      } 

      val.len = l ;
      val.val = oct.val ;
      oa = octarr_add( oa, octdup(&val)) ;

      if( oct.len > l ) { /* bcond ref and or blob too */
        for( sp = oct.val+l ; *sp == ' ' || *sp == '\t' ; sp++, l++ ) ; 
        bcond.val = oct.val + l ;
        bcond.len = oct.len - l ; 
        bcond.size = 0 ; 
        
	if( oct2strcmp( &bcond, "NULL") == 0 ) bcd = 0 ;
        else {
          unsigned int bl, d ;

          if(( bl = sexp_len( &bcond )) == 0 ) {
            traceLog( "Error in the boundarycondition definition" ) ;
            ruleset_free( vp ) ;
            if( exp ) oct_free( exp ) ;
            octclr( &prev ) ;
            return 0 ;
          } 

          if( bcond.len > bl ) { /* blob too */
            d = bcond.len - bl ;
            bcond.len = bl ;
            for( sp = bcond.val+bl ; *sp == ' ' || *sp == '\t' ; sp++, bl++ ) d-- ; 
            blob.val = bcond.val + bl ;
            blob.len = bcond.len - d ; 
            blob.size = 0 ; 
        
            oa = octarr_add( oa, octdup( &bcond )) ;
            oa = octarr_add( oa, octdup( &blob )) ;
          }
          else 
            oa = octarr_add( oa, octdup( &bcond )) ;

        }
      }

      if(( r = spocp_add_rule( (void **) &(rs->db), oa )) == SPOCP_SUCCESS ) n++ ;
      else {
        LOG( SPOCP_WARNING ) traceLog("Failed to add rule: \"%s\"", oct.val ) ;
        f++ ;
      }

      octarr_free( oa ) ;
      oa = 0 ;
    }
    else { /* ought to be a global or bcond def */
      int l, d ;

      octln( &key, &oct ) ;

      l = octchr( &key, '=' ) ;

      if( l < 0 ) {
        LOG( SPOCP_WARNING) { traceLog("Mysterious line: \"%s\"", oct.val ) ; }
        if( exp ) oct_free( exp ) ;
        octclr( &prev ) ;
        continue ; /* skip it */
      }

      d = 1 ;

      if( key.val[l-1] == ':' ) { /* bconddef */
        d++ ;
        --l ;
      } 

      key.len = ( unsigned int ) l ;
      val.val = key.val + key.len + d ;
      val.len = oct.len - key.len - d ;

      /* spaces both before and after the '=' is allowed, I have to get
       rid of them now */
      for( cp = key.val + key.len - 1 ; *cp == ' ' || *cp == '\t' ; cp-- ) {
        key.len-- ;
        if( key.len == 0 ) break ;
      }

      if( key.len == 0 ) {
        LOG( SPOCP_WARNING) { traceLog("Mysterious line: \"%s\"", oct.val ) ; }
        if( exp ) oct_free( exp ) ;
        octclr( &prev ) ;
        continue ;
      }

      for( cp = val.val ; *cp == ' ' || *cp == '\t' ; cp++ ) {
        val.len-- ;
        val.val++ ;
        if( val.len == 0 ) break ;
      }

      if( val.len == 0 ) {
        LOG( SPOCP_WARNING) { traceLog("Mysterious line: \"%s\"", oct.val ) ; }
        if( exp ) oct_free( exp ) ;
        octclr( &prev ) ;
        continue ;
      }
        
      if( d == 1 ) {
        if( spocp_add_global( globals, &key, &val ) == -1 ) {
          LOG( SPOCP_WARNING) {
            tmp = oct2strdup( &key, 0 ) ;
            traceLog("Attempt to redefine the constant: \"%s\"", tmp) ;
            free( tmp ) ;
          }
        }
      }
      else bcdef_add( rs->db, &key, &val ) ;
    }

    if( exp ) oct_free( exp ) ;
    octclr( &prev ) ;
  }

  *rc = n ;

  LOG(SPOCP_INFO) traceLog("Stored %d rules, failed %d", n, f ) ;

  return vp ;
}

/***************************************************************

 Structure of the rule file:

 line     = ( comment | global | ruledef | include | bconddef ) 
 comment  = "#" text CR
 rule     = "(" S_exp ")" [1*SP bcond [ 1*SP blob]] CR
 global   = key "=" value
 bconddef = key ":=" value
 include  = ";include" filename

 ***************************************************************/

void *get_rules( void *vp, char *file, int *rc  )
{   
  keyval_t *globals = 0  ;

  return (void *) read_rules( vp, file, rc, &globals ) ;
}

