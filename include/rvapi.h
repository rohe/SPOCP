#ifndef __RVAPI_H
#define __RVAPI_H

#include <element.h>
#include <spocp.h>

element_t       *element_new_atom( octet_t *oct );
element_t       *new_member( element_t *parent, element_t *original);

octarr_t        *parse_path(octet_t * o);
char            *parse_intervall(octet_t * o, int *start, int *end);

element_t       *element_dup(element_t * ep);

#endif