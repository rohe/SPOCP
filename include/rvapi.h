#include <struct.h>
#include <spocp.h>

int             element_size(element_t * e);
int             element_type(element_t * e);
element_t      *element_find_list(element_t * e, octet_t * tag);
element_t      *element_first(element_t * e);
element_t      *element_last(element_t * e);
element_t      *element_nth(element_t * e, int n);
element_t      *element_next(element_t * e, element_t * ep);
void            element_reverse(element_t * e);
element_t      *element_copy(element_t * e);
element_t      *element_parent(element_t * e);

void           *element_data(element_t * e);

element_t      *element_eval(octet_t * spec, element_t * e, int *rc);

int             element_cmp(element_t * a, element_t * b);

void            element_free(element_t * e);

#define element_next( elem )  ((elem) ? (((element_t *)(elem))->next) : NULL ) ;

element_t      *element_list_add(element_t * e, element_t * a);

octet_t        *atoms_join(element_t * e, char *sep);

octet_t        *element_atom_sub(octet_t * val, element_t * ep);
