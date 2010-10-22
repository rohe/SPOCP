#ifndef __BSDRB_H_
#define __BSDRB_H_

#include <ruleinst.h>
#include <tree.h>

/* function prototypes */

void            *bsdrb_init(void);
ruleinst_t      *bsdrb_find( void *vp, char *k);
int				bsdrb_insert( void *vp, ruleinst_t *ri );
int				bsdrb_remove( void *vp, char *k );
int				bsdrb_empty( void *vp );
varr_t          *bsdrb_all( void *vp) ;
void			bsdrb_free( void *vp) ;
int             bsdrb_len(void *vp);

#endif
