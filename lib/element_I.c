/*
 *  element_I.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/5/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#include <element.h>
#include <result.h>

/*!
 * \brief There are different types of elements this function returns a
 * indicator of what kind of element it is. 
 * \param e The element which type is asked for 
 * \return O if atom, 1 if list, 2 if set, 3 if prefix, 4 is suffix
 * 5 if range, 9 if an array and 10 if a NULL element. 
 */
int
element_type(element_t * e)
{
	if (e == 0)
		return -1;
    
	return (e->type);
}

/*!
 * The element struct is a container which within itself holds the value
 * bound to the element. This function will return a pointer to the element
 * data itself. If the element is of the type SPOCP_ATOM,SPOCP_SUFFiX or
 * SPOCP_PREFIX the returned pointer will point to a octet_t struct. If the
 * element is of the type SPOCP_RANGE it will point to a range_t struct. If
 * SPOCP_SET or SPOCP_ARRAY a varr_t struct is return. And if of type
 * SPOCP_LIST a pointer to the first element in the list will be returned.
 * \brief Returns a pointer to the element data. 
 * \param e A pointer to a element 
 * \return A void pointer 
 */
void	   *
element_data(element_t * e)
{
	if (e == 0)
		return 0;
    
	switch (e->type) {
        case SPOC_ATOM:
        case SPOC_SUFFIX:
        case SPOC_PREFIX:
            return (void *) &e->e.atom->val;
            break;
            
        case SPOC_RANGE:
            return (void *) e->e.range;
            break;
            
        case SPOC_SET:
        case SPOC_ARRAY:
            return (void *) e->e.set;
            break;
            
        case SPOC_LIST:
            return (void *) e->e.list->head;
            break;
            
        default:
            return 0;
	}
    
	return 0;		/* should never get here */
}
/*
 * ====================================================================== 
 */
/*!
 * \brief The number of elements in this element. Only returns the number of 
 * immediate elements, so if this list contains lists those lists are counted 
 * as 1 and not as the number of elements in the sublists. If the element is a 
 * list, array or set then the number of elements in those are returned, if it 
 * is a atom, 1 is returned. 
 * \param e The element in which the subelements are to be counted. 
 * \return The number of subelements in the element. 
 */
int
element_size(element_t * e)
{
	int		i = 0;
    
	if (e == 0)
		return -1;
    
	switch (e->type) {
        case SPOC_ATOM:
            i = 1;
            break;
        
        case SPOC_LIST:
            for (i = 0, e = e->e.list->head; e; e = e->next)
                i++;
            break;
            
        case SPOC_SET:
        case SPOC_ARRAY:
            i = e->e.set->n;
            break;
	}
    
	return i;
}

/*
 * ---------------------------------------------------------------------- 
 */

/*!
 * \brief Gives you the nth element in a list, or one element from a array
 * or set element 
 * \param e A pointer to the element list from which the element should be 
 *  picked 
 * \param n The index of the element to pick 
 * \return A pointer to the subelement . If the element given is a atom 
 *   element and the index was set to 0, the element itself will be returned. 
 */
element_t	*
element_nth(element_t * e, int n)
{
	int		i;
    
	if (e == 0)
		return 0;
    
	if (e->type == SPOC_LIST) {
		for (i = 0, e = e->e.list->head; i < n && e->next;
             e = e->next, i++);
        
		if (i == n) 
			return e;
		else
			return 0;
        
	} else if (e->type == SPOC_SET || e->type == SPOC_ARRAY) {
		return (element_t *) varr_nth(e->e.set, n);
	} else if (e->type == SPOC_ATOM && n == 0)
		return e;
	else
		return 0;
}


/*
 * ---------------------------------------------------------------------- 
 */

/*!
 * \brief This function compares two elements and decides whether they are
 * equal or not. Only atom or list elements can be compared at present. 
 * \param e0, e1 the two elements that are to be compared. 
 * \return Returns 0 if the elements are equal and non zero if they are not. 
 */
int
element_cmp(element_t * e0, element_t * e1, spocp_result_t *rc)
{
	int		t, i, n, r = 0;
    
    *rc = SPOCP_SUCCESS;
    
	if (e0 == 0 || e1 == 0)
		return -1;
    
	t = element_type(e0);
	i = element_type(e1);
    
	if (t != i)
		return t - i;
    
	switch (t) {
        case SPOC_ATOM:
            r = octcmp(element_data(e0), element_data(e1));
            break;
            
        case SPOC_LIST:
            n = element_size(e0);
            i = element_size(e1);
            
            if (n != i)
                return n - i;
            for (i = 0; i < n; i++) {
                if ((r = element_cmp(element_nth(e0, i),
                                     element_nth(e1, i), rc)) != 0)
                    if (*rc == SPOCP_SUCCESS)
                        break;
            }
            break;
            
        case SPOC_SET:
            r = -1;
            break;
            
        case SPOC_ARRAY:
            r = -1;
            break;
	}
    
	return r;
}
