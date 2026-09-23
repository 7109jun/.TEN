/*
 * Windows 백엔드. 이 Linux 샌드박스에서는 컴파일 검증을 하지 못했다.
 * CMake가 WIN32 빌드에서만 이 파일을 포함하므로 Linux/Android 빌드에는
 * 관여하지 않는다.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h> /* _beginthreadex */

#include "ten/thread.h"
#include "ten/memory.h"
#include "internal/error_internal.h"

struct ten_thread {
    HANDLE handle;
    ten_thread_fn function;
    void *userdata;
    void *result;
};

static unsigned __stdcall ten__thread_trampoline(void *arg)
{
    ten_thread_t *thread = (ten_thread_t *)arg;
    thread->result = thread->function(thread->userdata);
    return 0;
}

ten_thread_t *ten_thread_create(ten_thread_fn function, void *userdata)
{
    ten_thread_t *thread;

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

    thread->function = function;
    thread->userdata = userdata;
    thread->result = NULL;

    thread->handle = (HANDLE)_beginthreadex(NULL, 0, ten__thread_trampoline, thread, 0, NULL);
    if (thread->handle == NULL)
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
    if (thread == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (WaitForSingleObject(thread->handle, INFINITE) != WAIT_OBJECT_0)
    {
        CloseHandle(thread->handle);
        ten_free(thread);
        ten__set_error(TEN_ERROR_SYSTEM);
        return (int)TEN_ERROR_SYSTEM;
    }

    if (result != NULL)
    {
        *result = thread->result;
    }

    CloseHandle(thread->handle);
    ten_free(thread);

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

void ten_thread_sleep(uint64_t milliseconds)
{
    Sleep((DWORD)milliseconds);
}

uint64_t ten_thread_current_id(void)
{
    return (uint64_t)GetCurrentThreadId();
}
