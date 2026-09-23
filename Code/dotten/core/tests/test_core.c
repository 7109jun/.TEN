#include <assert.h>
#include <string.h>
#include <ten/ten.h>

int main(void)
{
    /* 초기화 전 상태 */
    assert(ten_is_initialized() == 0);

    /* 정상 초기화 */
    assert(ten_init() == TEN_OK);
    assert(ten_is_initialized() != 0);

    /* 중복 초기화는 실패해야 한다 */
    assert(ten_init() == (int)TEN_ERROR_ALREADY_INITIALIZED);
    assert(ten_last_error() == TEN_ERROR_ALREADY_INITIALIZED);
    /* 중복 초기화 실패 후에도 이미 초기화된 상태는 유지되어야 한다 */
    assert(ten_is_initialized() != 0);

    /* 버전 정보 */
    assert(strcmp(ten_version(), "1.0.0") == 0);
    assert(ten_version_major() == 1);
    assert(ten_version_minor() == 0);
    assert(ten_version_patch() == 0);

    /* 종료 */
    ten_shutdown();
    assert(ten_is_initialized() == 0);

    /* 초기화되지 않은 상태에서 shutdown을 또 호출해도 안전해야 한다 */
    ten_shutdown();
    assert(ten_is_initialized() == 0);

    /* 종료 후 재초기화가 가능해야 한다 */
    assert(ten_init() == TEN_OK);
    ten_shutdown();

    return 0;
}
