/*
 *  boundary.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/3/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __BOUNDARY_H
#define __BOUNDARY_H

#include <config.h>
#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <octet.h>

/*! \brief Representing one side of a range */
typedef struct _boundary {
	/*! The range type */
	int             type;
    int             dynamic;
    
	/*! Where the boundary value is stored */
	union {
		/*! integer boundary value, number or time */
		long int        num;
		/*! IP version 4 representation */
		struct in_addr  v4;
#ifdef USE_IPV6
		/*! IP version 6 representation */
		struct in6_addr v6;
#endif
		/*! byte string */
		octet_t         val;
	} v;
    
} boundary_t;

boundary_t      *boundary_new(int type);
spocp_result_t  set_limit(boundary_t * bp, octet_t * op);
int             boundary_cpy(boundary_t * dest, boundary_t * src);
void            boundary_free(boundary_t * bp);
int             boundary_oprint(octet_t *oct, boundary_t *b);
boundary_t      *boundary_dup(boundary_t * bp);
int             boundary_xcmp(boundary_t * b1p, boundary_t * b2p);
int             boundarycmp(boundary_t * b1p, boundary_t * b2p, 
                            spocp_result_t *rc);
char            *boundary_prints(boundary_t * bp);

#endif /* __BOUNDARY_H */