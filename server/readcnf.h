#ifndef __READCNF_H
#define __READCNF_H

#include <stdio.h>

#include <spocp.h>

typedef struct _chunk {
	octet_t        *val;
	struct _chunk  *next;
	struct _chunk  *prev;
} spocp_chunk_t;

typedef struct _ruledef {
	spocp_chunk_t	*rule;
	spocp_chunk_t *bcond;
	spocp_chunk_t *blob;
} spocp_ruledef_t;

typedef struct _char_buffert {
	char           *str;
	char           *start;
	int             size;
	FILE           *fp;
} spocp_charbuf_t;

char		*get_more(spocp_charbuf_t *);
spocp_chunk_t	*get_object(spocp_charbuf_t *, spocp_chunk_t *);
void		ruledef_return(spocp_ruledef_t *, spocp_chunk_t *);
void		chunk_free(spocp_chunk_t *);
char 		*chunk2sexp(spocp_chunk_t *);

#endif

