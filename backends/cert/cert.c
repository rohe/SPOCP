/***************************************************************************
                          be_cert.c  -  description
                             -------------------
    begin                : Thu Sep 9 2003
    copyright            : (C) 2003 by Karolinska Institutet, Sweden
    email                : ola.gustafsson@itc.ki.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#ifdef HAVE_LIBSSL

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <spocp.h>
#include <srvconf.h>
#include <wrappers.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>


int cb(int ok, X509_STORE_CTX *ctx);
void decode_base64(char *,char *);
X509_STORE *initCertificateCheck(char *ca_dir);

spocp_result_t check_cert(X509_STORE *ctx, char *data);

befunc cert_test;

/*
  type         = "cert"
  typespecific = base64pemcert
  
  returns true if the certificate is issued by someone we believe in. this is determined by a 
  directory with pem certificates of our trusted CA's. At the moment, this dir is hardcoded to
  /usr/local/spocp/conf/cert ... but I have to find a better way of doing this.

  The inputline should be in base64 WITHOUT newlines.
 */

void decode_base64(char *inbuf,char *outbuf) {
  BIO *bio, *b64;
  int inlen;
  
  DEBUG( SPOCP_DBCOND ){
    traceLog("Creating base64-filter");
  }
  b64 = BIO_new(BIO_f_base64());
  DEBUG( SPOCP_DBCOND ){
    traceLog("Setting flags");
  }
  BIO_set_flags(b64,BIO_FLAGS_BASE64_NO_NL);

  DEBUG( SPOCP_DBCOND ){
    traceLog("Creating memory buffer");
  }
  bio = BIO_new_mem_buf(inbuf, -1);
  DEBUG( SPOCP_DBCOND ){
    traceLog("Pushing base64-filter");
  }
  bio = BIO_push(b64, bio);
  DEBUG( SPOCP_DBCOND ){
    traceLog("Reading into outbuffer");
  }
  while((inlen = BIO_read(bio, outbuf, 65000)) > 0) {
    DEBUG( SPOCP_DBCOND ){
      traceLog("Have read %d bytes",inlen);
    }
  }

  
  DEBUG( SPOCP_DBCOND ){
    traceLog("Freeing all.");
  }
  BIO_free_all(bio);
}


X509_STORE *initCertificateCheck(char *ca_dir) {
  X509_STORE *cert_ctx = NULL;
  X509_LOOKUP *lookup=NULL;
  int i;

  DEBUG( SPOCP_DBCOND ){
    traceLog("Creating the cert context");
  }

  cert_ctx = X509_STORE_new();

  if (cert_ctx == NULL) {
    DEBUG( SPOCP_DBCOND ){
      traceLog("Couldn't init cert_ctx");
    }
    return NULL;
  }

  DEBUG( SPOCP_DBCOND ){
    traceLog("Setting the callback function");
  }
  X509_STORE_set_verify_cb_func(cert_ctx,cb);

  /*
  DEBUG( SPOCP_DBCOND ){
    traceLog("Adding lookup of file");
  }
  lookup=X509_STORE_add_lookup(cert_ctx,X509_LOOKUP_file());
  if (lookup == NULL) {
    DEBUG( SPOCP_DBCOND ){
      traceLog("Couldn't init lookup_file");
    }
    return NULL;
  }
  X509_LOOKUP_load_file(lookup,NULL,X509_FILETYPE_DEFAULT);
  */

  DEBUG( SPOCP_DBCOND ){
    traceLog("Adding lookup of dir");
  }
  lookup=X509_STORE_add_lookup(cert_ctx,X509_LOOKUP_hash_dir());
  if (lookup == NULL) {
    DEBUG( SPOCP_DBCOND ){
      traceLog("Couldn't init lookup_dir");
    }
    return NULL;
  }
  i=X509_LOOKUP_add_dir(lookup,ca_dir,X509_FILETYPE_PEM);
  if(!i) {
    DEBUG( SPOCP_DBCOND ){
      traceLog("Couldn't add ca_dir to lookup");
    }
    return NULL;
  }

  DEBUG( SPOCP_DBCOND ){
    traceLog("Clearing errors.");
  }
  ERR_clear_error();

  return cert_ctx;
}

int cb(int ok, X509_STORE_CTX *ctx) {
  if (!ok) {
    if (ctx->error != X509_V_ERR_CERT_SIGNATURE_FAILURE) {
      DEBUG( SPOCP_DBCOND ){
	traceLog("Got error: %d",ctx->error);
      }
    }
    if (ctx->error == X509_V_ERR_PATH_LENGTH_EXCEEDED) ok=1;
    if (ctx->error == X509_V_ERR_INVALID_PURPOSE) ok=1;
    if (ctx->error == X509_V_ERR_CERT_SIGNATURE_FAILURE) ok=1;
  }
  return(ok);
}

spocp_result_t check_cert(X509_STORE *ctx, char *data) 
{
  X509 *x=NULL;
  BIO *in=NULL;
  X509_STORE_CTX *csc;

  int i=0 ;
  spocp_result_t ret ;
  
  in = BIO_new_mem_buf(data, -1);
  if (in == NULL) {
    DEBUG( SPOCP_DBCOND ){
      traceLog("Problem with opening buffer with data");
    }
    /*
    if (x != NULL) {
      X509_free(x);
    }
    if (in != NULL) {
      BIO_free(in);
    }
    */
    return SPOCP_OPERATIONSERROR ;
  }
  
  x=PEM_read_bio_X509(in,NULL,NULL,NULL);

  if (x == NULL) {
    DEBUG( SPOCP_DBCOND ){
      traceLog("Problem with reading X509 certificate");
    }
    /*
    if (x != NULL) {
      X509_free(x);
    }
    if (in != NULL) {
    */
      BIO_free(in);
    /*
    }
    */
    return SPOCP_OPERATIONSERROR ;
  }

  csc = X509_STORE_CTX_new();
  if (csc == NULL) {
    DEBUG( SPOCP_DBCOND ){
      traceLog("Problem with creating new X509_STORE_CTX");
    }
    if (x != NULL) {
      X509_free(x);
    }
    if (in != NULL) {
      BIO_free(in);
    }
    return SPOCP_OPERATIONSERROR ;
  }

  X509_STORE_CTX_init(csc,ctx,x,NULL);
  i=X509_verify_cert(csc);
  X509_STORE_CTX_free(csc);

  if (i) ret = SPOCP_SUCCESS ;
  else   ret = SPOCP_DENIED ;

  return(ret);
}

X509_STORE *cert_ctx=NULL;

spocp_result_t cert_test(
  element_t *qp, element_t *rp, element_t *xp, octet_t *arg, pdyn_t *bcp, octet_t *blob ) 
{
  spocp_result_t answer;

  char *cert_b64 = "";
  char *decoded_cert = "";

  DEBUG( SPOCP_DBCOND ){
    traceLog("Entering cert_test");
  }

  arg->val[arg->len] = 0;
  cert_b64 = arg->val;
  DEBUG( SPOCP_DBCOND ){
    traceLog("Allocating memory for decoded part. Memory needed: %d",arg->len);
  }

  DEBUG( SPOCP_DBCOND ){
    traceLog("Decoding base64.");
  }
  decoded_cert = (char *)Malloc(arg->len * sizeof(char));

  decode_base64(cert_b64,decoded_cert);
  DEBUG( SPOCP_DBCOND ){
    traceLog("Checking certificate.");
  }

  answer = check_cert(cert_ctx,decoded_cert);
  
  DEBUG( SPOCP_DBCOND ){
    traceLog("Answer from check: %d.",answer);
  }

  if (cert_ctx != NULL) X509_STORE_free(cert_ctx);
  free(decoded_cert);

  return answer;
}

spocp_result_t cert_syntax( char *arg)
{
  return SPOCP_SUCCESS ;
}


spocp_result_t cert_init( confgetfn *cgf, void *conf, becpool_t *bcp ) {
  octarr_t   *oa = 0 ;
  void       *vp = 0;
  char       *path = "/usr/local/spocp/conf/cert";
  struct stat sts;
  int         i ;

  LOG( SPOCP_DEBUG) traceLog( "cert_init" ) ;

  /* If you are loking up a plugin configuration attribute, cfg will always
     returns a pointer to a octarr struct */
  /* using vp to avoid warnings about type-punned pointers */
  cgf( conf, PLUGIN, "cert", "cadir", &vp ) ;
  if( vp ) on = ( octnode_t *) vp ;
  
  for( i ; i < oa->n ; i ) {
    path = oa->arr[i]->val ;
    if (stat(path, &sts) == -1 && errno == ENOENT) {
      DEBUG( SPOCP_DBCOND ){
        traceLog("The path specified: [%s] does not exist.",path);
      }
    }
    else break ; 
  }

  if( vp && i == oa->n ) return SPOCP_UNAVAILABLE ;

  cert_ctx = initCertificateCheck(path);

  if(cert_ctx == NULL) {
    DEBUG( SPOCP_DBCOND ){
      traceLog("Cert_ctx is NULL");
    }
    return SPOCP_UNAVAILABLE ;
  }
  DEBUG( SPOCP_DBCOND ){
    traceLog("Have inited certificate check with dir [%s].",path);
  }

  return SPOCP_SUCCESS ;
}


#else
int spocp_no_ssl ;
#endif
