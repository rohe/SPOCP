#include "locl.h"
RCSID("$Id$");

#include <basefn.h>

static pool_t  *
pool_new(void)
{
    pool_t         *p;

    p = (pool_t *) Calloc(1, sizeof(pool_t));

    /*
    pthread_mutex_init(&p->lock, NULL);
    */

    return p;
}

pool_item_t    *
pool_pop(pool_t * pool)
{
    pool_item_t    *item;

    if (pool->size == 0)
        return 0;

    item = pool->tail;

    if (item != pool->head) {
        item->prev->next = item->next;
        pool->tail = item->prev;
    } else
        pool->head = pool->tail = 0;

    if (item)
        item->next = item->prev = 0;

    pool->size--;

    return item;
}

static void 
pool_free( pool_t *p, ffunc *ff )
{
    pool_item_t *pi = 0;

    if (p) {
        while ((pi = pool_pop(p))) {
            if(ff && pi->info)
                ff(pi->info);
            Free(pi);
        }

        Free(p);
    }
}

afpool_t       *
afpool_new(void)
{
    afpool_t       *afp;

    afp = (afpool_t *) Malloc(sizeof(afpool_t));

    afp->free = pool_new();
    afp->active = pool_new();

    pthread_mutex_init(&afp->aflock, NULL);

    return afp;
}

void
afpool_free( afpool_t *a, ffunc *ff )
{
    if (a){
        afpool_lock( a );
        if (a->free)
            pool_free( a->free, ff );
        if (a->active)
            pool_free( a->active, ff);

        afpool_unlock( a );
        Free(a);
    }
}

int
afpool_lock(afpool_t * afp)
{
    return pthread_mutex_lock(&afp->aflock);
}

int
afpool_unlock(afpool_t * afp)
{
    return pthread_mutex_unlock(&afp->aflock);
}

int
afpool_free_item(afpool_t * afp)
{
    int r = 0;

    if (afp) {
        if ( afp->free && afp->free->size)
            r = 1;
    }

    return r;
}

int
afpool_active_item(afpool_t * afp)
{
    int r = 0;

    if (afp) {
        if ( afp->active && afp->active->size)
            r = 1;
    }

    return r;
}

static void
pool_rm_item(pool_t * pool, pool_item_t * item)
{
    /* got a lock higher up *
    pthread_mutex_lock(&pool->lock);
    */

    if (pool->head == item) {
        if (pool->head == pool->tail)
            pool->head = pool->tail = 0;
        else {
            item->next->prev = 0;
            pool->head = item->next;
        }
    } else if (pool->tail == item) {
        item->prev->next = 0;
        pool->tail = item->prev;
    } else {
        if (item->prev)
            item->prev->next = item->next;
        if (item->next)
            item->next->prev = item->prev;
    }

    item->prev = item->next = 0;

    pool->size--;

    /* Got a lock higher up
    pthread_mutex_unlock(&pool->lock);
    */
}

void
pool_add(pool_t * pool, pool_item_t * item)
{
    /* pthread_mutex_lock(&pool->lock); */

    if (pool->size == 0) {
        pool->tail = pool->head = item;
        item->next = item->prev = 0;
    } else {
        pool->tail->next = item;

        item->prev = pool->tail;
        item->next = 0;

        pool->tail = item;
    }

    pool->size++;

    /* pthread_mutex_unlock(&pool->lock); */
}

static int
pool_size(pool_t * pool)
{
    int             i;
    pool_item_t    *pi;

    if (pool == 0)
        return 0;

    if (pool->head == 0)
        return 0;

    /* pthread_mutex_lock(&pool->lock); */

    for (pi = pool->head, i = 0; pi; pi = pi->next)
        i++;

    /* pthread_mutex_unlock(&pool->lock); */

    return i;
}

/*
 * remove it from the active pool and place it in the free pool only done
 * from spocp_server_run(), of which there is only one running 
 */
int
item_return(afpool_t * afp, pool_item_t * item)
{
    if (afp == 0)
        return 0;

    afpool_lock( afp );
    pool_rm_item(afp->active, item);

    pool_add(afp->free, item);
    afpool_unlock( afp );

    return 1;
}

/*
 * get a item from from the free pool, place it in the active pool only done
 * from spocp_server_run(), of which there is only one running 
 *
pool_item_t    *
item_get(afpool_t * afp)
{
    pool_item_t    *item;

    if (afp == 0)
        return 0;

    if (afp->free == 0 || afp->free->head == 0)
        return 0;

     *
     * get it from the free pool 
     * 
    item = pool_pop(afp->free);

     *
     * place it in the active pool 
     *

    if (item)
        pool_add(afp->active, item);

    return item;
}
*/

void
afpool_push_item(afpool_t * afp, pool_item_t * pi)
{
    if (afp) {
        afpool_lock(afp);
        if( afp->active)
            pool_add(afp->active, pi);
        afpool_unlock(afp);
    }
}

void
afpool_push_empty(afpool_t * afp, pool_item_t * pi)
{
    if (afp) {
        afpool_lock(afp);
        if (afp->free) 
            pool_add(afp->free, pi);
        afpool_unlock(afp);
    }
}

pool_item_t    *
afpool_get_item(afpool_t * afp)
{
    pool_item_t *pi=0;

    if (afp) {
        afpool_lock(afp);
        if (afp->active && afp->active->size)
            pi = pool_pop(afp->active);
        afpool_unlock(afp);
    } 

    return pi;
}

pool_item_t    *
afpool_get_empty(afpool_t * afp)
{
    pool_item_t *pi=0;

    if (afp) {
        afpool_lock(afp);
        if (afp->free && afp->free->size)
            pi = pool_pop(afp->free);
        afpool_unlock(afp);
    } 

    return pi;
}

pool_item_t    *
afpool_first(afpool_t * afp)
{
    pool_item_t *pi=0;

    if (afp) {
        afpool_lock(afp);
        if (afp->active)
            pi = afp->active->head;
        afpool_unlock(afp);
    }
    
    return pi;
}

int
number_of_active(afpool_t * afp)
{
    int n=0;

    if (afp) {
    afpool_lock(afp);
    n = pool_size(afp->active);
    afpool_unlock(afp);
    }

    return n;
}

int
number_of_free(afpool_t * afp)
{
    int n=0;

    if (afp) {
    afpool_lock(afp);
    n = pool_size(afp->free);
    afpool_unlock(afp);
    }

    return n;
}

