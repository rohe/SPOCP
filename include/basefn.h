#ifndef __BASEFN_H
#define __BASEFN_H

typedef void   *item_t;
typedef char   *Key;

/*
 * free function 
 */
typedef void    (ffunc) (item_t vp);

/*
 * duplicate function 
 */
typedef         item_t(dfunc) (item_t a, item_t b);

/*
 * copy function 
 */
typedef int     (cfunc) (item_t dest, item_t src);

/*
 * compare function 
 */
typedef int     (cmpfn) (item_t a, item_t b);

/*
 * print function 
 */
typedef char   *(pfunc) (item_t vp);

/*
 * key function, normally a hashfunction 
 */
typedef char   *(kfunc) (item_t);

#endif
