#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
/* -std=c99(strict)에서는 clock_gettime 등 POSIX 확장이 기본적으로
   숨겨지므로 명시적으로 노출시킨다. */
#define _POSIX_C_SOURCE 199309L
#endif

#include "ten/time.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

#ifdef _WIN32

/* Windows FILETIME(1601-01-01 epoch, 100ns 단위) -> Unix epoch 변환 상수 */
#define TEN_FILETIME_EPOCH_DIFF_100NS 116444736000000000ULL

static uint64_t ten__filetime_to_unix_100ns(void)
{
    FILETIME ft;
    ULARGE_INTEGER uli;

    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    return uli.QuadPart - TEN_FILETIME_EPOCH_DIFF_100NS;
}

uint64_t ten_time_now_ms(void)
{
    return ten__filetime_to_unix_100ns() / 10000ULL;
}

uint64_t ten_time_now_us(void)
{
    return ten__filetime_to_unix_100ns() / 10ULL;
}

uint64_t ten_time_monotonic_ms(void)
{
    return (uint64_t)GetTickCount64();
}

#else /* POSIX(Linux/Android) */

uint64_t ten_time_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

uint64_t ten_time_now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

uint64_t ten_time_monotonic_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

#endif

int64_t ten_time_unix(void)
{
    return (int64_t)time(NULL);
}
