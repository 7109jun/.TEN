/*
 * Windows 백엔드(미검증 — thread_windows.c 상단 주석 참고).
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ten/sync.h"
#include "ten/memory.h"
#include "internal/error_internal.h"

/* ---- Mutex ----
 * CRITICAL_SECTION은 원래 같은 스레드의 재귀적 잠금을 지원하므로,
 * 일반 mutex와 recursive mutex 모두 동일하게 구현한다. */

struct ten_mutex {
    CRITICAL_SECTION handle;
};

static ten_mutex_t *ten__mutex_create(void)
{
    ten_mutex_t *mutex = (ten_mutex_t *)ten_malloc(sizeof(ten_mutex_t));
    if (mutex == NULL)
    {
        return NULL;
    }

    InitializeCriticalSection(&mutex->handle);
    ten__set_error(TEN_OK);
    return mutex;
}

ten_mutex_t *ten_mutex_create(void)
{
    return ten__mutex_create();
}

ten_mutex_t *ten_mutex_create_recursive(void)
{
    return ten__mutex_create();
}

void ten_mutex_destroy(ten_mutex_t *mutex)
{
    if (mutex == NULL)
    {
        return;
    }

    DeleteCriticalSection(&mutex->handle);
    ten_free(mutex);
}

int ten_mutex_lock(ten_mutex_t *mutex)
{
    if (mutex == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    EnterCriticalSection(&mutex->handle);
    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

int ten_mutex_try_lock(ten_mutex_t *mutex)
{
    if (mutex == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (!TryEnterCriticalSection(&mutex->handle))
    {
        ten__set_error(TEN_ERROR_TIMEOUT);
        return (int)TEN_ERROR_TIMEOUT;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

int ten_mutex_unlock(ten_mutex_t *mutex)
{
    if (mutex == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    LeaveCriticalSection(&mutex->handle);
    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

/* ---- Semaphore ---- */

struct ten_semaphore {
    HANDLE handle;
};

ten_semaphore_t *ten_semaphore_create(unsigned int initial)
{
    ten_semaphore_t *semaphore = (ten_semaphore_t *)ten_malloc(sizeof(ten_semaphore_t));
    if (semaphore == NULL)
    {
        return NULL;
    }

    semaphore->handle = CreateSemaphoreA(NULL, (LONG)initial, LONG_MAX, NULL);
    if (semaphore->handle == NULL)
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

    if (WaitForSingleObject(semaphore->handle, INFINITE) != WAIT_OBJECT_0)
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

    if (!ReleaseSemaphore(semaphore->handle, 1, NULL))
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

    CloseHandle(semaphore->handle);
    ten_free(semaphore);
}

/* ---- Condition Variable ---- */

struct ten_cond {
    CONDITION_VARIABLE handle;
};

ten_cond_t *ten_cond_create(void)
{
    ten_cond_t *cond = (ten_cond_t *)ten_malloc(sizeof(ten_cond_t));
    if (cond == NULL)
    {
        return NULL;
    }

    InitializeConditionVariable(&cond->handle);
    ten__set_error(TEN_OK);
    return cond;
}

void ten_cond_destroy(ten_cond_t *cond)
{
    /* CONDITION_VARIABLE은 명시적으로 해제할 커널 자원이 없다. */
    if (cond == NULL)
    {
        return;
    }

    ten_free(cond);
}

int ten_cond_wait(ten_cond_t *cond, ten_mutex_t *mutex)
{
    if (cond == NULL || mutex == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (!SleepConditionVariableCS(&cond->handle, &mutex->handle, INFINITE))
    {
        ten__set_error(TEN_ERROR_SYSTEM);
        return (int)TEN_ERROR_SYSTEM;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

int ten_cond_wait_timeout(ten_cond_t *cond, ten_mutex_t *mutex, uint64_t milliseconds)
{
    if (cond == NULL || mutex == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (!SleepConditionVariableCS(&cond->handle, &mutex->handle, (DWORD)milliseconds))
    {
        if (GetLastError() == ERROR_TIMEOUT)
        {
            ten__set_error(TEN_ERROR_TIMEOUT);
            return (int)TEN_ERROR_TIMEOUT;
        }

        ten__set_error(TEN_ERROR_SYSTEM);
        return (int)TEN_ERROR_SYSTEM;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

int ten_cond_signal(ten_cond_t *cond)
{
    if (cond == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    WakeConditionVariable(&cond->handle);
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

    WakeAllConditionVariable(&cond->handle);
    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}
