#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "ten/thread.h"
#include "ten/memory.h"
#include "internal/error_internal.h"

#include <pthread.h>
#include <time.h>

struct ten_thread {
    pthread_t handle;
};

ten_thread_t *ten_thread_create(ten_thread_fn function, void *userdata)
{
    ten_thread_t *thread;
    int rc;

    if (function == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    thread = (ten_thread_t *)ten_malloc(sizeof(ten_thread_t));
    if (thread == NULL)
    {
        return NULL;
    }

    rc = pthread_create(&thread->handle, NULL, function, userdata);
    if (rc != 0)
    {
        ten_free(thread);
        ten__set_error(TEN_ERROR_SYSTEM);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return thread;
}

int ten_thread_join(ten_thread_t *thread, void **result)
{
    void *ret = NULL;
    int rc;

    if (thread == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    rc = pthread_join(thread->handle, &ret);
    ten_free(thread);

    if (rc != 0)
    {
        ten__set_error(TEN_ERROR_SYSTEM);
        return (int)TEN_ERROR_SYSTEM;
    }

    if (result != NULL)
    {
        *result = ret;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

void ten_thread_sleep(uint64_t milliseconds)
{
    struct timespec ts;
    ts.tv_sec = (time_t)(milliseconds / 1000ULL);
    ts.tv_nsec = (long)((milliseconds % 1000ULL) * 1000000ULL);
    nanosleep(&ts, NULL);
}

uint64_t ten_thread_current_id(void)
{
    return (uint64_t)(uintptr_t)pthread_self();
}
