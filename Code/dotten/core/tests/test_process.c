#include <assert.h>
#include <string.h>
#include <ten/ten.h>

int main(void)
{
    const char *argv[] = { "true", NULL };
    const char *value;

    assert(ten_process_id() > 0);

    /* 존재하지 않는 환경 변수 */
    assert(ten_env_get("TEN_TEST_DOES_NOT_EXIST_XYZ") == NULL);

    /* 설정 -> 조회 왕복 */
    assert(ten_env_set("TEN_TEST_VAR", "hello") == TEN_OK);
    value = ten_env_get("TEN_TEST_VAR");
    assert(value != NULL);
    assert(strcmp(value, "hello") == 0);

    /* 정상적으로 실행되는 프로그램(PATH에서 검색) */
    assert(ten_process_execute("true", argv) == TEN_OK);

    /* NULL 인자 */
    assert(ten_process_execute(NULL, argv) == (int)TEN_ERROR_NULL_ARGUMENT);
    assert(ten_process_execute("true", NULL) == (int)TEN_ERROR_NULL_ARGUMENT);
    assert(ten_env_get(NULL) == NULL);
    assert(ten_last_error() == TEN_ERROR_NULL_ARGUMENT);
    assert(ten_env_set(NULL, "x") == (int)TEN_ERROR_NULL_ARGUMENT);
    assert(ten_env_set("x", NULL) == (int)TEN_ERROR_NULL_ARGUMENT);

    return 0;
}
