/*!
 * \file spocp.h
 * \brief Spocp basic definitions
 */
#ifndef __SPOCP_H
#define __SPOCP_H

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <octet.h>
#include <result.h>
/*
 * --------------------------------------------------------------------------- 
 */

/*! \brief Information about the operation requestor, not just information about the
 *	client but also about the subject using the client.
 */
typedef struct {
	/*! FQDN of the client */
	char	*hostname;
	/*! IP address of the client */
	char	*hostaddr;
	/*! inverted representation of the clients FQDN */
	char	*invhost;
	/*! Information about the subject using the client, normally in the form
 	 *  of a S-expression */
	octet_t	*subject;
	/*! This is just a string representation of the subjecct */
	char	*sexp_subject;
} spocp_req_info_t;

/*! \brief information about a piece of a S-expression, used for parsing */
typedef struct _chunk {
	/*! The piece */
	octet_t        *val;
	/*! Next chunk */
	struct _chunk  *next;
	/*! Previouos chunk */
	struct _chunk  *prev;
} spocp_chunk_t;

/*! \brief Input buffer used by the S-expression parsing functions */
typedef struct {
	/*! Pointer to the string to parse */
	char           *str;
	/*! Where parsing should start */
	char           *start;
	/*! The size of the memory area where the string is stored */
	int             size;
	/*! Pointer to a file descriptor where more bytes can be gotten */
	FILE           *fp;
} spocp_charbuf_t;

/*
 * ===============================================================================
 */

int             get_len(octet_t * oct, spocp_result_t *rc);
spocp_result_t	get_str(octet_t * oct, octet_t * str);
int             sexp_len(octet_t * sexp);

char            *sexp_printa(char *sexp, unsigned int *bsize, char *fmt,
                             void **argv);
char            *sexp_printv(char *sexp, unsigned int *bsize, char *fmt, ...);

spocp_result_t  octseq2octarr(octet_t *arg, octarr_t **oa);

/*
 * =============================================================================== 
 */

spocp_chunk_t	*chunk_new(octet_t *);
spocp_charbuf_t *charbuf_new( FILE *fp, size_t size );
void            charbuf_free( spocp_charbuf_t * );

void            chunk_free(spocp_chunk_t *);
char            charbuf_peek( spocp_charbuf_t *cb );
char            *get_more(spocp_charbuf_t *);
octet_t         *chunk2sexp(spocp_chunk_t *);
spocp_chunk_t	*get_sexp( spocp_charbuf_t *ib, spocp_chunk_t *pp );
spocp_chunk_t	*get_chunk(spocp_charbuf_t * io);
spocp_chunk_t	*chunk_add( spocp_chunk_t *pp, spocp_chunk_t *np );

spocp_chunk_t	*get_sexp_from_oct(octet_t *);
octet_t         *sexp_normalize_oct(octet_t *);
spocp_chunk_t	*get_sexp_from_str(char *);
octet_t         *sexp_normalize(char *);


/*
 * =============================================================================== 
 */

/*! The current log level */
extern int	spocp_loglevel;
/*! The debug level */ 
extern int	spocp_debug;
/* whether syslog is used or not */
extern int	log_syslog;

/*
 * =============================================================================== 
 */

/*! A macro that tests whether the log level is high enough to cause loggin */
#define LOG(x)	if( spocp_loglevel >= (x) )
/*! A macro that tests whether the debug level is high enough to cause loggin */
#define DEBUG(x) if( spocp_loglevel == SPOCP_DEBUG && ((x) & spocp_debug))

/*
 * =============================================================================== 
 */

#endif
