/***************************************************************************
                           locl.h  -  description
                             -------------------
    begin                : Sat Mar 17 2004
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/utsname.h>

#include <netinet/in.h>  
#include <netinet/tcp.h>

#include <arpa/inet.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <regex.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <pthread.h>

#ifdef HAVE_SSL
#include <openssl/ssl.h>
#endif

#ifdef HAVE_LIBWRAP
#include <tcpd.h>
#endif

/*
#include "srvconf.h"
*/

#include <spocp.h>
#include <func.h>
#include <macros.h>
#include <wrappers.h>
#include <proto.h>

#include <db0.h>
#include <dback.h>
#include <plugin.h>
#include <dbapi.h>

#include <srv.h>
