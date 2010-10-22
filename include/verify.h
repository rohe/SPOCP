/*
 *  verify.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/3/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __VERIFY_H
#define __VERIFY_H

#include <netinet/in.h>

#include <octet.h>

spocp_result_t  is_numeric(octet_t *op, long *num) ;
spocp_result_t  numstr(char *str, long *l) ;

spocp_result_t  is_ipv4(octet_t * op, struct in_addr * ia) ;
spocp_result_t  is_ipv4_s(char * ip, struct in_addr * ia) ;

spocp_result_t  is_ipv6(octet_t * op, struct in6_addr * ia) ;
spocp_result_t  is_ipv6_s(char *ip, struct in6_addr * ia) ;

spocp_result_t  is_date(octet_t * op);
spocp_result_t  is_time(octet_t * op);

#endif
