
/***************************************************************************
                          rdwr.c  -  description
                             -------------------
    begin                : Thu May 2 2002
    copyright            : (C) 2002 by Roland Hedberg
    email                : roland@catalogix.se
 ***************************************************************************/

#include <stdlib.h>

#include <plugin.h>
#include <wrappers.h>

#if defined HAVE_LIBPTHREAD || defined HAVE_PTHREAD_H

#include <rdwr.h>

int
pthread_rdwr_init(pthread_rdwr_t * rdwrp)
{
	rdwrp->readers_reading = 0;
	rdwrp->writer_writing = 0;
	pthread_mutex_init(&(rdwrp->mutex), NULL);
	pthread_cond_init(&(rdwrp->lock_free), NULL);
	return 0;
}

int
pthread_rdwr_rlock(pthread_rdwr_t * rdwrp)
{
	pthread_mutex_lock(&rdwrp->mutex);

	while (rdwrp->writer_writing)
		pthread_cond_wait(&rdwrp->lock_free, &rdwrp->mutex);

	rdwrp->readers_reading++;

	pthread_mutex_unlock(&rdwrp->mutex);

	return 0;
}

int
pthread_rdwr_wlock(pthread_rdwr_t * rdwrp)
{
	pthread_mutex_lock(&rdwrp->mutex);

	while (rdwrp->writer_writing || rdwrp->readers_reading)
		pthread_cond_wait(&rdwrp->lock_free, &rdwrp->mutex);


	rdwrp->writer_writing++;

	pthread_mutex_unlock(&rdwrp->mutex);

	return 0;
}

int
pthread_rdwr_runlock(pthread_rdwr_t * rdwrp)
{
	pthread_mutex_lock(&rdwrp->mutex);

	if (rdwrp->readers_reading == 0) {
		pthread_mutex_unlock(&rdwrp->mutex);
		return -1;
	} else {
		rdwrp->readers_reading--;

		if (rdwrp->readers_reading == 0)
			pthread_cond_signal(&rdwrp->lock_free);

		pthread_mutex_unlock(&rdwrp->mutex);

		return 0;
	}
}

int
pthread_rdwr_wunlock(pthread_rdwr_t * rdwrp)
{
	pthread_mutex_lock(&rdwrp->mutex);

	if (rdwrp->writer_writing == 0) {
		pthread_mutex_unlock(&rdwrp->mutex);
		return -1;
	} else {
		rdwrp->writer_writing = 0;

		pthread_cond_broadcast(&rdwrp->lock_free);

		pthread_mutex_unlock(&rdwrp->mutex);

		return 0;
	}
}

int
pthread_rdwr_destroy(pthread_rdwr_t * p)
{
	int             r;

	if (p) {
		if ((r = pthread_mutex_destroy(&(p->mutex))) != 0)
			return r;
		if ((r = pthread_cond_destroy(&(p->lock_free))) != 0)
			return r;

		Free(p);
	}

	return 0;
}
#endif
