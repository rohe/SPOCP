
/***************************************************************************
                          wrappers.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#define _GNU_SOURCE
#include <string.h>

#include <wrappers.h>
#include <func.h>

/* #define DEBUG_XYZ 1 */

void	*
Malloc(size_t size)
{
#ifdef DEBUG_XYZ
	void	*vp = xMalloc(size);
#else
	void	*vp = malloc(size);
#endif

	if (vp == 0)
		FatalError("Out of memory", 0, 0);

	return vp;
}


void	*
Calloc(size_t n, size_t size)
{
#ifdef DEBUG_XYZ
	void	*vp = xCalloc(n, size);
#else
	void	*vp = calloc(n, size);
#endif

	if (vp == 0)
		FatalError("Out of memory", 0, 0);

	return vp;
}


void           *
Recalloc(void *vp, size_t n, size_t size)
{
#ifdef DEBUG_XYZ
	void	*nvp = xRecalloc(vp, n, size);
#else
	void	*nvp = realloc(vp, n * size);
#endif

	if (nvp == 0)
		FatalError("Out of memory", 0, 0);

	return nvp;
}

void
Free( void *v )
{
#ifdef DEBUG_XYZ
	xFree(v);
#else
	free(v);
#endif
}

void           *
Realloc(void *vp, size_t n)
{
#ifdef DEBUG_XYZ
	void	*nvp = xRealloc(vp, n);
#else
	void	*nvp = realloc(vp, n);
#endif

	if (nvp == 0)
		FatalError("Out of memory", 0, 0);

	return nvp;
}

char           *
Strdup(char *s)
{
#ifdef DEBUG_XYZ
	char	*sp = xStrdup(s);
#else
	char	*sp = strdup(s);
#endif

	if (sp == 0)
		FatalError("Out of memory", 0, 0);

	return sp;
}

char           *
Strcat(char *dest, char *src, int *size)
{
	char           *tmp;
	int             dl, sl;

	if (src == 0 || *size <= 0)
		return 0;

	dl = strlen(dest);
	sl = strlen(src);

	if (sl + dl > *size) {
		*size = sl + dl + 32;
		tmp = Realloc(dest, *size);
		dest = tmp;
	}

	/* Flawfinder: ignore */
	strcat(dest, src);

	return dest;
}

#ifndef HAVE_STRNLEN
size_t          strnlen(const char *s, size_t len);

size_t
strnlen(const char *s, size_t len)
{
	size_t          i;

	for (i = 0; i < len && s[i]; i++);

	return i;
}
#endif

#ifndef HAVE_STRLCPY
size_t
strlcpy(char *dst, const char *s, size_t size)
{
	size_t          i = strlen(s);

	if( i < size-1)
		strcpy( dst, s) ;
	else {
		strncpy( dst, s, size-1);
		dst[size-1] = '\0';
	}

	return i;
}
#endif

#ifndef HAVE_STRLCAT
size_t
strlcat(char *dst, const char *s, size_t size)
{
	size_t	a = strlen(dst);
	size_t	b = strlen(s);

	if( (a+b) < size-1 )
		strcat( dst, s) ;
	else {
		b = size - a - 1;
		strncat( dst, s, b);
		dst[size-1] = '\0';
	}

	return b;
}
#endif

#ifndef HAVE_STRNDUP
char           *
strndup(const char *old, size_t sz)
{
	size_t          len = strnlen(old, sz);
	char           *t = malloc(len + 1);

	if (t != NULL) {
		memcpy(t, old, len);
		t[len] = '\0';
	}

	return t;
}
#endif

char           *
Strndup(char *s, size_t n)
{
	char	*sp;

	if (s == 0) 
		return 0;

	sp = strndup(s, n);

	if (sp == 0)
		FatalError("Out of memory", 0, 0);

	return sp;
}

/*
 * -------------- special debugging helps ------------ 
 */

void
xFree(void *vp)
{
	traceLog(LOG_DEBUG,"%p-free", vp);
	free(vp);
}

void           *
xMalloc(size_t size)
{
	void           *vp = malloc(size);

	traceLog(LOG_DEBUG,"%p(%d)-malloc", vp, size);

	if (vp == 0)
		FatalError("Out of memory", 0, 0);

	return vp;
}


void           *
xCalloc(size_t n, size_t size)
{
	void           *vp = calloc(n, size);

	traceLog(LOG_DEBUG,"%p(%d,%d)-calloc", vp, n, size);

	if (vp == 0)
		FatalError("Out of memory", 0, 0);

	return vp;
}


void           *
xRecalloc(void *vp, size_t n, size_t size)
{
	void           *nvp = realloc(vp, n * size);

	traceLog(LOG_DEBUG,"%p->%p(%d)-recalloc", vp, nvp, n * size);

	if (nvp == 0)
		FatalError("Out of memory", 0, 0);

	return nvp;
}


void           *
xRealloc(void *vp, size_t n)
{
	void           *nvp = realloc(vp, n);

	traceLog(LOG_DEBUG,"%p->%p(%d)-realloc", vp, nvp, n);

	if (nvp == 0)
		FatalError("Out of memory", 0, 0);

	return nvp;
}

char           *
xStrdup(char *s)
{
	char           *sp = strdup(s);

	traceLog(LOG_DEBUG,"%p(%d)-strdup\n", sp, strlen(s));

	if (sp == 0)
		FatalError("Out of memory", 0, 0);

	return sp;
}

char           *
xStrcat(char *dest, char *src, int *size)
{
	char           *tmp;
	int             dl, sl;

	dl = strlen(dest);
	sl = strlen(src);

	if (sl + dl > *size) {
		*size = sl + dl + 32;
		tmp = xRealloc(dest, *size);
		dest = tmp;
	}

	/* Flawfinder: ignore */
	strcat(dest, src);

	return dest;
}

char           *
xStrndup(char *s, size_t n)
{
	char           *sp = strndup(s, n);

	traceLog(LOG_DEBUG,"%p(%d)-strndup", sp, n);

	if (sp == 0)
		FatalError("Out of memory", 0, 0);

	return sp;
}

int
P_fclose(void *vp)
{
	return fclose((FILE *) vp);
}
