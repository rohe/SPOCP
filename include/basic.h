/*
 *  basic.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/3/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __BASIC_H_
#define __BASIC_H_

#include <config.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <octet.h>

int		ipv4cmp(struct in_addr *ia1, struct in_addr *ia2);

#ifdef USE_IPV6
int		ipv6cmp(struct in6_addr *ia1, struct in6_addr *ia2);
#endif

void	hms2int(octet_t * op, long int *li);
int     to_gmt(octet_t * s, octet_t * t);

#endif