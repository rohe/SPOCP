/*
 *  ruledef.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 2/16/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

typedef struct _ruledef {
	spocp_chunk_t	*rule;
	spocp_chunk_t	*bcond;
	spocp_chunk_t	*blob;
} spocp_ruledef_t;