
/***************************************************************************
                          wrappers.h  -  description
                             -------------------
    begin                : Mon Jun 10 2002
    copyright            : (C) 2002 by SPOCP
    email                : roland@catalogix.se
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *                                                                         *
 ***************************************************************************/

#ifndef __WRAPPERS_H
#define __WRAPPERS_H

#include <stdlib.h>
#include <string.h>
#include "config.h"

void           *Malloc(size_t size);

void           *Calloc(size_t n, size_t size);

void           *Recalloc(void *vp, size_t n, size_t size);

void           *Realloc(void *vp, size_t n);

char           *Strdup(char *s);

char           *Strcat(char *dest, char *src, int *size);

char           *Strndup(char *s, size_t n);

#ifndef HAVE_STRNDUP
char           *strndup(const char *old, size_t sz);
#endif

#ifndef HAVE_STRLCAT
char           *strlcat(char *dst, const char *old, size_t sz);
#endif

#ifndef HAVE_STRLCPY
char           *strlcpy(char *dst, const char *old, size_t sz);
#endif

/*================================================*/

void           *xMalloc(size_t size);

void           *xCalloc(size_t n, size_t size);

void           *xRecalloc(void *vp, size_t n, size_t size);

void           *xRealloc(void *vp, size_t n);

char           *xStrdup(char *s);

char           *xStrcat(char *dest, char *src, int *size);

char           *xStrndup(char *s, size_t n);

void            xFree(void *vp);

int             P_fclose(void *v);

#endif
