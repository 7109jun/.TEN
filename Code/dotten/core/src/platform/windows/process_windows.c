/*
 * Windows 백엔드(미검증 — thread_windows.c 상단 주석 참고).
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ten/process.h"
#include "internal/error_internal.h"

#include <stdio.h>
#include <string.h>

uint64_t ten_process_id(void)
{
    return (uint64_t)GetCurrentProcessId();
}

/* argv를 Windows CreateProcess가 요구하는 단일 커맨드라인 문자열로
   합친다. 인자에 공백이 있으면 큰따옴표로 감싼다(내부에 이미 있는
   큰따옴표를 이스케이프하는 완전한 처리는 하지 않는다 — 일반적인
   경로/인자 수준의 사용을 목표로 한다). */
static char *ten__build_command_line(const char *program, const char *const *argv)
{
    size_t total = 0;
    size_t i;
    char *result;
    char *cursor;

    for (i = 0; argv[i] != NULL; i++)
    {
        total += strlen(argv[i]) + 3; /* 따옴표 2개 + 구분 공백 1개 */
    }

    result = (char *)malloc(total + 1);
    if (result == NULL)
    {
        return NULL;
    }

    cursor = result;
    for (i = 0; argv[i] != NULL; i++)
    {
        int need_quotes = (strchr(argv[i], ' ') != NULL) || (argv[i][0] == '\0');

        if (i > 0)
        {
            *cursor++ = ' ';
        }

        if (need_quotes)
        {
            *cursor++ = '"';
        }
        memcpy(cursor, argv[i], strlen(argv[i]));
        cursor += strlen(argv[i]);
        if (need_quotes)
        {
            *cursor++ = '"';
        }
    }
    *cursor = '\0';

    (void)program; /* CreateProcess의 lpApplicationName은 NULL로 두고
                       lpCommandLine의 argv[0](program)만 사용한다. */
    return result;
}

int ten_process_execute(const char *program, const char *const *argv)
{
    char *cmdline;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    BOOL ok;

    if (program == NULL || argv == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    cmdline = ten__build_command_line(program, argv);
    if (cmdline == NULL)
    {
        ten__set_error(TEN_ERROR_OUT_OF_MEMORY);
        return (int)TEN_ERROR_OUT_OF_MEMORY;
    }

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    ok = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    free(cmdline);

    if (!ok)
    {
        ten__set_error(TEN_ERROR_SYSTEM);
        return (int)TEN_ERROR_SYSTEM;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

const char *ten_env_get(const char *name)
{
    static char buffer[32768]; /* Windows 환경변수 최대 길이 */
    DWORD n;

    if (name == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    n = GetEnvironmentVariableA(name, buffer, (DWORD)sizeof(buffer));
    if (n == 0)
    {
        ten__set_error(TEN_OK); /* 존재하지 않음은 오류가 아니다 */
        return NULL;
    }

    ten__set_error(TEN_OK);
    return buffer;
}

int ten_env_set(const char *name, const char *value)
{
    if (name == NULL || value == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (!SetEnvironmentVariableA(name, value))
    {
        ten__set_error(TEN_ERROR_SYSTEM);
        return (int)TEN_ERROR_SYSTEM;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}
