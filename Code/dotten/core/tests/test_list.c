#include <assert.h>
#include <string.h>
#include <ten/ten.h>

static void test_string_list(void)
{
    ten_list_t *list = ten_list_create(TEN_TYPE_STRING);
    const char *hello = "Hello";
    const char *ten = ".TEN";
    const char *world = "World";
    const char *value;

    assert(list != NULL);
    assert(ten_list_count(list) == 0);

    assert(ten_list_add(list, &hello) == TEN_OK);
    assert(ten_list_add(list, &ten) == TEN_OK);
    assert(ten_list_count(list) == 2);

    value = *(const char **)ten_list_get(list, 0);
    assert(strcmp(value, "Hello") == 0);
    value = *(const char **)ten_list_get(list, 1);
    assert(strcmp(value, ".TEN") == 0);

    /* 중간 삽입 */
    assert(ten_list_insert(list, 1, &world) == TEN_OK);
    assert(ten_list_count(list) == 3);
    value = *(const char **)ten_list_get(list, 1);
    assert(strcmp(value, "World") == 0);
    value = *(const char **)ten_list_get(list, 2);
    assert(strcmp(value, ".TEN") == 0);

    /* out-of-range 삽입 */
    assert(ten_list_insert(list, 100, &world) == (int)TEN_ERROR_OUT_OF_RANGE);

    /* 삭제 */
    assert(ten_list_remove_at(list, 1) == TEN_OK);
    assert(ten_list_count(list) == 2);
    value = *(const char **)ten_list_get(list, 1);
    assert(strcmp(value, ".TEN") == 0);

    /* out-of-range 조회/삭제 */
    assert(ten_list_get(list, 99) == NULL);
    assert(ten_last_error() == TEN_ERROR_OUT_OF_RANGE);
    assert(ten_list_remove_at(list, 99) == (int)TEN_ERROR_OUT_OF_RANGE);

    ten_list_destroy(list);
}

static void test_scalar_list(void)
{
    ten_list_t *list = ten_list_create(TEN_TYPE_I32);
    int i;

    assert(list != NULL);

    /* append amortized O(1) 확인을 겸한 대량 삽입 + capacity 확장 */
    for (i = 0; i < 100; i++)
    {
        assert(ten_list_add(list, &i) == TEN_OK);
    }
    assert(ten_list_count(list) == 100);
    assert(ten_list_capacity(list) >= 100);

    for (i = 0; i < 100; i++)
    {
        int *v = (int *)ten_list_get(list, (size_t)i);
        assert(v != NULL);
        assert(*v == i);
    }

    ten_list_destroy(list);
}

static void test_null_and_invalid(void)
{
    int dummy = 1;

    /* TEN_TYPE_VOID로는 list를 만들 수 없다 */
    assert(ten_list_create(TEN_TYPE_VOID) == NULL);
    assert(ten_last_error() == TEN_ERROR_INVALID_ARGUMENT);

    assert(ten_list_add(NULL, &dummy) == (int)TEN_ERROR_NULL_ARGUMENT);

    {
        ten_list_t *list = ten_list_create(TEN_TYPE_I32);
        assert(list != NULL);
        assert(ten_list_add(list, NULL) == (int)TEN_ERROR_NULL_ARGUMENT);
        assert(ten_list_count(NULL) == 0);
        assert(ten_last_error() == TEN_ERROR_NULL_ARGUMENT);
        ten_list_destroy(list);
    }

    /* destroy(NULL)은 안전해야 한다 */
    ten_list_destroy(NULL);
}

int main(void)
{
    test_string_list();
    test_scalar_list();
    test_null_and_invalid();
    return 0;
}
