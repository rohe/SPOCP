#ifndef __READCNF_H
#define __READCNF_H

#include <stdio.h>

#include <spocp.h>

typedef struct _ruledef {
	spocp_chunk_t	*rule;
	spocp_chunk_t	*bcond;
	spocp_chunk_t	*blob;
} spocp_ruledef_t;

spocp_chunk_t	*get_object(spocp_charbuf_t *, spocp_chunk_t *);
void		ruledef_return(spocp_ruledef_t *, spocp_chunk_t *);

#endif

