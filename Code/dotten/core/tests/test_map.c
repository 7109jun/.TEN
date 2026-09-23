#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ten/ten.h>

static void test_string_key_map(void)
{
    ten_map_t *map = ten_map_create(TEN_TYPE_STRING, TEN_TYPE_I32);
    /* 일부러 별도의 버퍼에 같은 내용을 담아, 포인터가 달라도 내용으로
       매칭되는지(내용 기반 해시/비교) 검증한다. */
    char key_buf[16];
    const char *key1 = "answer";
    const char *key2;
    int value = 42;
    int other = 7;
    int *found;

    assert(map != NULL);
    assert(ten_map_count(map) == 0);

    assert(ten_map_set(map, &key1, &value) == TEN_OK);
    assert(ten_map_count(map) == 1);

    strcpy(key_buf, "answer"); /* key1과 내용은 같지만 다른 주소 */
    key2 = key_buf;
    assert(key2 != key1);

    found = (int *)ten_map_get(map, &key2);
    assert(found != NULL);
    assert(*found == 42);

    /* 덮어쓰기 */
    assert(ten_map_set(map, &key1, &other) == TEN_OK);
    assert(ten_map_count(map) == 1); /* 새 key가 아니라 갱신이어야 한다 */
    found = (int *)ten_map_get(map, &key1);
    assert(*found == 7);

    assert(ten_map_contains(map, &key1) == 1);
    assert(ten_map_remove(map, &key1) == TEN_OK);
    assert(ten_map_contains(map, &key1) == 0);
    assert(ten_map_count(map) == 0);

    /* 존재하지 않는 key */
    assert(ten_map_get(map, &key1) == NULL);
    assert(ten_last_error() == TEN_ERROR_NOT_FOUND);
    assert(ten_map_remove(map, &key1) == (int)TEN_ERROR_NOT_FOUND);

    ten_map_destroy(map);
}

static void test_scalar_key_map_and_rehash(void)
{
    ten_map_t *map = ten_map_create(TEN_TYPE_I32, TEN_TYPE_I32);
    int i;

    assert(map != NULL);

    /* 초기 bucket 수(16)를 넘는 삽입으로 rehash 경로를 검증한다 */
    for (i = 0; i < 200; i++)
    {
        int value = i * 10;
        assert(ten_map_set(map, &i, &value) == TEN_OK);
    }
    assert(ten_map_count(map) == 200);

    for (i = 0; i < 200; i++)
    {
        int *found = (int *)ten_map_get(map, &i);
        assert(found != NULL);
        assert(*found == i * 10);
    }

    ten_map_destroy(map);
}

static void test_null_and_invalid(void)
{
    int key = 1;
    int value = 1;

    assert(ten_map_create(TEN_TYPE_VOID, TEN_TYPE_I32) == NULL);
    assert(ten_last_error() == TEN_ERROR_INVALID_ARGUMENT);
    assert(ten_map_create(TEN_TYPE_I32, TEN_TYPE_VOID) == NULL);

    assert(ten_map_set(NULL, &key, &value) == (int)TEN_ERROR_NULL_ARGUMENT);

    {
        ten_map_t *map = ten_map_create(TEN_TYPE_I32, TEN_TYPE_I32);
        assert(map != NULL);
        assert(ten_map_set(map, NULL, &value) == (int)TEN_ERROR_NULL_ARGUMENT);
        assert(ten_map_set(map, &key, NULL) == (int)TEN_ERROR_NULL_ARGUMENT);
        assert(ten_map_count(NULL) == 0);
        ten_map_destroy(map);
    }

    ten_map_destroy(NULL);
}

int main(void)
{
    test_string_key_map();
    test_scalar_key_map_and_rehash();
    test_null_and_invalid();
    return 0;
}
