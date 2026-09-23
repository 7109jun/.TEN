#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "ten/sync.h"
#include "ten/memory.h"
#include "internal/error_internal.h"

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

/* ---- Mutex ---- */

struct ten_mutex {
    pthread_mutex_t handle;
};

static ten_mutex_t *ten__mutex_create(int recursive)
{
    ten_mutex_t *mutex;
    pthread_mutexattr_t attr;
    pthread_mutexattr_t *attr_ptr = NULL;
    int rc;

    mutex = (ten_mutex_t *)ten_malloc(sizeof(ten_mutex_t));
    if (mutex == NULL)
    {
        return NULL;
    }

    if (recursive)
    {
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        attr_ptr = &attr;
    }

    rc = pthread_mutex_init(&mutex->handle, attr_ptr);

    if (attr_ptr != NULL)
    {
        pthread_mutexattr_destroy(&attr);
    }

    if (rc != 0)
    {
        ten_free(mutex);
        ten__set_error(TEN_ERROR_SYSTEM);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return mutex;
}

ten_mutex_t *ten_mutex_create(void)
{
    return ten__mutex_create(0);
}

ten_mutex_t *ten_mutex_create_recursive(void)
{
    return ten__mutex_create(1);
}

void ten_mutex_destroy(ten_mutex_t *mutex)
{
    if (mutex == NULL)
    {
        return;
    }

    pthread_mutex_destroy(&mutex->handle);
    ten_free(mutex);
}

int ten_mutex_lock(ten_mutex_t *mutex)
{
    if (mutex == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (pthread_mutex_lock(&mutex->handle) != 0)
    {
        ten__set_error(TEN_ERROR_SYSTEM);
        return (int)TEN_ERROR_SYSTEM;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

int ten_mutex_try_lock(ten_mutex_t *mutex)
{
    int rc;

    if (mutex == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    rc = pthread_mutex_trylock(&mutex->handle);
    if (rc == 0)
    {
        ten__set_error(TEN_OK);
        return (int)TEN_OK;
    }

    if (rc == EBUSY)
    {
        ten__set_error(TEN_ERROR_TIMEOUT);
        return (int)TEN_ERROR_TIMEOUT;
    }

    ten__set_error(TEN_ERROR_SYSTEM);
    return (int)TEN_ERROR_SYSTEM;
}

int ten_mutex_unlock(ten_mutex_t *mutex)
{
    if (mutex == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (pthread_mutex_unlock(&mutex->handle) != 0)
    {
        ten__set_error(TEN_ERROR_SYSTEM);
        return (int)TEN_ERROR_SYSTEM;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

/* ---- Semaphore ---- */

struct ten_semaphore {
    sem_t handle;
};

ten_semaphore_t *ten_semaphore_create(unsigned int initial)
{
    ten_semaphore_t *semaphore = (ten_semaphore_t *)ten_malloc(sizeof(ten_semaphore_t));
    if (semaphore == NULL)
    {
        return NULL;
    }

    if (sem_init(&semaphore->handle, 0, initial) != 0)
    {
        ten_free(semaphore);
        ten__set_error(TEN_ERROR_SYSTEM);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return semaphore;
}

int ten_semaphore_wait(ten_semaphore_t *semaphore)
{
    if (semaphore == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (sem_wait(&semaphore->handle) != 0)
    {
        ten__set_error(TEN_ERROR_SYSTEM);
        return (int)TEN_ERROR_SYSTEM;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

int ten_semaphore_post(ten_semaphore_t *semaphore)
{
    if (semaphore == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (sem_post(&semaphore->handle) != 0)
    {
        ten__set_error(TEN_ERROR_SYSTEM);
        return (int)TEN_ERROR_SYSTEM;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

void ten_semaphore_destroy(ten_semaphore_t *semaphore)
{
    if (semaphore == NULL)
    {
        return;
    }

    sem_destroy(&semaphore->handle);
    ten_free(semaphore);
}

/* ---- Condition Variable ---- */

struct ten_cond {
    pthread_cond_t handle;
};

ten_cond_t *ten_cond_create(void)
{
    ten_cond_t *cond = (ten_cond_t *)ten_malloc(sizeof(ten_cond_t));
    if (cond == NULL)
    {
        return NULL;
    }

    if (pthread_cond_init(&cond->handle, NULL) != 0)
    {
        ten_free(cond);
        ten__set_error(TEN_ERROR_SYSTEM);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return cond;
}

void ten_cond_destroy(ten_cond_t *cond)
{
    if (cond == NULL)
    {
        return;
    }

    pthread_cond_destroy(&cond->handle);
    ten_free(cond);
}

int ten_cond_wait(ten_cond_t *cond, ten_mutex_t *mutex)
{
    if (cond == NULL || mutex == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (pthread_cond_wait(&cond->handle, &mutex->handle) != 0)
    {
        ten__set_error(TEN_ERROR_SYSTEM);
        return (int)TEN_ERROR_SYSTEM;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

int ten_cond_wait_timeout(ten_cond_t *cond, ten_mutex_t *mutex, uint64_t milliseconds)
{
    struct timespec ts;
    int rc;

    if (cond == NULL || mutex == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    clock_gettime(CLOCK_REALTIME, &ts);

    ts.tv_sec += (time_t)(milliseconds / 1000ULL);
    ts.tv_nsec += (long)((milliseconds % 1000ULL) * 1000000ULL);
    if (ts.tv_nsec >= 1000000000L)
    {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000L;
    }

    rc = pthread_cond_timedwait(&cond->handle, &mutex->handle, &ts);
    if (rc == 0)
    {
        ten__set_error(TEN_OK);
        return (int)TEN_OK;
    }

    if (rc == ETIMEDOUT)
    {
        ten__set_error(TEN_ERROR_TIMEOUT);
        return (int)TEN_ERROR_TIMEOUT;
    }

    ten__set_error(TEN_ERROR_SYSTEM);
    return (int)TEN_ERROR_SYSTEM;
}

int ten_cond_signal(ten_cond_t *cond)
{
    if (cond == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (pthread_cond_signal(&cond->handle) != 0)
    {
        ten__set_error(TEN_ERROR_SYSTEM);
        return (int)TEN_ERROR_SYSTEM;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

int ten_cond_broadcast(ten_cond_t *cond)
{
    if (cond == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (pthread_cond_broadcast(&cond->handle) != 0)
    {
        ten__set_error(TEN_ERROR_SYSTEM);
        return (int)TEN_ERROR_SYSTEM;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}
