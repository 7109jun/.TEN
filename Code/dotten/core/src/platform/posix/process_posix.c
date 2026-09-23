#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
/* setenv/unsetenv는 _POSIX_C_SOURCE만으로는 노출되지 않는 배포판이
   있어 _DEFAULT_SOURCE(glibc)도 함께 정의한다. */
#if !defined(_DEFAULT_SOURCE)
#define _DEFAULT_SOURCE
#endif

#include "ten/process.h"
#include "internal/error_internal.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

uint64_t ten_process_id(void)
{
    return (uint64_t)getpid();
}

int ten_process_execute(const char *program, const char *const *argv)
{
    pid_t pid;
    int status;

    if (program == NULL || argv == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    pid = fork();
    if (pid < 0)
    {
        ten__set_error(TEN_ERROR_SYSTEM);
        return (int)TEN_ERROR_SYSTEM;
    }

    if (pid == 0)
    {
        /* 자식 프로세스: execvp가 성공하면 여기로 돌아오지 않는다. */
        execvp(program, (char *const *)argv);
        _exit(127);
    }

    if (waitpid(pid, &status, 0) < 0)
    {
        ten__set_error(TEN_ERROR_SYSTEM);
        return (int)TEN_ERROR_SYSTEM;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

const char *ten_env_get(const char *name)
{
    if (name == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return getenv(name);
}

int ten_env_set(const char *name, const char *value)
{
    if (name == NULL || value == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (setenv(name, value, 1) != 0)
    {
        ten__set_error(TEN_ERROR_SYSTEM);
        return (int)TEN_ERROR_SYSTEM;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}
