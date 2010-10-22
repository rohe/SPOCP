/*
 *  octet.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/3/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __OCTET_H
#define __OCTET_H

#include <stdlib.h>

#include <result.h>

/*
 * ------------------------------------- 
 */

/*! \brief The really basic struct, where byte arrays are kept */
typedef struct {
	/*! The size of the byte array */
	size_t	len;
	/*! The size of the memory area allocated */
	size_t	size;
	/*! Pointer into the byte array, these are not strings hence not NULL 
     terminated */
	char	*val;
    /*! Points to the start of byte array, should not be touched!!! */
    char    *base;
} octet_t;

/*
 * ------------------------------------- 
 */

/*! \brief A struct representing a array of byte arrays */
typedef struct {
	/*! The current number of byte arrays */
	int	n;
	/*! The maximum number of byte arrays that can be assigned */
	int	size;
	/*! The array of byte arrays, not NULL terminated */
	octet_t	**arr;
} octarr_t;

/*
 * =========================================================================== 
 */

void        position_val_pointer( octet_t *active, octet_t *base);
char        *optimized_allocation(octet_t *base, int *size);
octet_t		*octdup(octet_t * oct);
int         octcmp(octet_t * a, octet_t * b);
int         octrcmp(octet_t * a, octet_t * b);
void		octmove(octet_t * a, octet_t * b);
int         octncmp(octet_t * a, octet_t * b, size_t i);
int         octstr(octet_t * val, char *needle);
int         octchr(octet_t * o, char c);
int         octpbrk(octet_t * o, octet_t * acc);
octet_t		*oct_new(size_t len, char *val);
octarr_t	*oct_split(octet_t * o, char c, char ec, int flag, int max);
void		oct_free(octet_t * o);
void		oct_freearr(octet_t ** o);
int         oct2strcmp(octet_t * o, char *s);
int         oct2strncmp(octet_t * o, char *s, size_t l);
char		*oct2strdup(octet_t * op, char ec);
int         oct2strcpy(octet_t * op, char *str, size_t len, char ec);
void		octset( octet_t *, char *, int );
char		*str_esc( char *, int );

octet_t		*str2oct( char *, int );
int         octtoi(octet_t * o);

int         oct_de_escape(octet_t * op, char ec);
int         oct_find_balancing(octet_t * p, char left, char right);
void		oct_assign(octet_t * o, char *s);

spocp_result_t  oct_resize(octet_t *, size_t);
spocp_result_t  octcpy(octet_t *, octet_t *);
int             octln(octet_t *, octet_t *);
octet_t         *octcln(octet_t *);
void            octclr(octet_t *);
spocp_result_t  octcat(octet_t *, char *, size_t);
int             oct_step(octet_t *, int);
void            oct_print( int p, char *tag, octet_t *o);
void            print_oct_info(char *label, octet_t *op);

octarr_t	*octarr_new(size_t n);
octarr_t	*octarr_add(octarr_t * oa, octet_t * o);
octarr_t	*octarr_dup(octarr_t * oa);
octarr_t	*octarr_extend(octarr_t * a, octarr_t * b);
void		octarr_mr(octarr_t * oa, size_t n);
void		octarr_free(octarr_t * oa);
void		octarr_half_free(octarr_t * oa);
octet_t		*octarr_pop(octarr_t * oa);
octet_t		*octarr_rpop(octarr_t * oa);
octet_t		*octarr_rm(octarr_t * oa, int n);
int         octarr_len(octarr_t * oa);

void		octarr_print( int p, octarr_t *oa);

/* ----------- strfunc.c ------------------- */

char		**line_split(char *s, char c, char ec, int flag, int max,
                         int *parts);

void		rmcrlf(char *s);
char		*rmlt(char *s, int shift);
char		*find_balancing(char *p, char left, char right);
void		charmatrix_free( char **m );
char		*normalize(char *);

#endif /* __OCTET_H */
