/*!
 * \file sexptool.h
 */
#ifndef __SEXPTOOL_H
#define __SEXPTOOL_H

/*! function prototype for a function that can grab some information from a 
 * connection */
typedef char   *(sexpargfunc) (void * comarg);

/*! A structure that represents one part of a S-expression or represents 
 * the link between a name and a function */
typedef struct {
	/*! A string to put in the place, or a name on the function */
	char           *var;
	/*! A function that can return a string to put in the place */
	sexpargfunc    *af;
	/*! The type of string, one of two 'a' or 'l' */
	char            type;
	/*! whether the string was dynamically allocated (1) or not (0)*/
	char            dynamic;
} sexparg_t;

sexparg_t	** parse_format(const char *, const sexparg_t [], int );
char		* sexp_constr( void *, sexparg_t ** );
void 		sexparg_free( sexparg_t **sa ) ;

#endif

