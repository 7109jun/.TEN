#include "ten/core.h"
#include "internal/error_internal.h"

/*
 * 초기화 플래그 자체는 스레드 간에 공유되어야 하는 전역 상태이므로
 * thread-local로 두지 않는다(오류 코드와는 다름). 정확한 동시 초기화
 * 보장은 M3(Synchronization) 모듈 도입 이후 mutex로 강화한다.
 */
static int g_ten_initialized = 0;

int ten_init(void)
{
    if (g_ten_initialized)
    {
        ten__set_error(TEN_ERROR_ALREADY_INITIALIZED);
        return (int)TEN_ERROR_ALREADY_INITIALIZED;
    }

    g_ten_initialized = 1;
    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

int ten_is_initialized(void)
{
    return g_ten_initialized;
}

void ten_shutdown(void)
{
    g_ten_initialized = 0;
}
