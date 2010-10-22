/*
 *  log.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/3/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __LOG_H
#define __LOG_H

#include <sys/time.h>
#include <result.h>

void            FatalError(char *msg, char *s, int i);
void            traceLog(int priority, const char *fmt, ...);
void            print_elapsed(char *s, struct timeval start, struct timeval end);
void            timestamp(char *s);
spocp_result_t  spocp_open_log(char *file, int level);

#endif