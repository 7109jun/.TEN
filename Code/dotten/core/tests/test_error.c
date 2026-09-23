#include <assert.h>
#include <string.h>
#include <ten/ten.h>

int main(void)
{
    ten_error_code_t code;

    /* clear 이후에는 TEN_OK */
    ten_clear_error();
    assert(ten_last_error() == TEN_OK);

    /* enum에 정의된 모든 코드는 비어있지 않은 메시지를 가져야 한다 */
    for (code = TEN_OK; code <= TEN_ERROR_NOT_INITIALIZED; code = (ten_error_code_t)(code + 1))
    {
        const char *msg = ten_error_message(code);
        assert(msg != NULL);
        assert(strlen(msg) > 0);
    }

    /* 정의되지 않은(범위를 벗어난) 코드 — 잘못된 입력 케이스 */
    assert(strcmp(ten_error_message((ten_error_code_t)9999), "Unrecognized error code") == 0);

    /* 다른 모듈의 실패가 스레드-로컬 상태에 반영되는지 확인 (NULL 인자 케이스) */
    assert(ten_set_allocator(NULL) == (int)TEN_ERROR_NULL_ARGUMENT);
    assert(ten_last_error() == TEN_ERROR_NULL_ARGUMENT);

    ten_clear_error();
    assert(ten_last_error() == TEN_OK);

    return 0;
}
