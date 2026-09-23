#include <assert.h>
#include <string.h>
#include <ten/ten.h>

static void test_parse_scalars(void)
{
    ten_json_t *j;

    j = ten_json_parse("null");
    assert(j != NULL);
    assert(ten_json_type(j) == TEN_JSON_NULL);
    ten_json_destroy(j);

    j = ten_json_parse("true");
    assert(ten_json_type(j) == TEN_JSON_BOOL);
    assert(ten_json_get_bool(j) == 1);
    ten_json_destroy(j);

    j = ten_json_parse("false");
    assert(ten_json_get_bool(j) == 0);
    ten_json_destroy(j);

    j = ten_json_parse("  42  ");
    assert(ten_json_type(j) == TEN_JSON_NUMBER);
    assert(ten_json_get_number(j) == 42.0);
    ten_json_destroy(j);

    j = ten_json_parse("-3.5e2");
    assert(ten_json_get_number(j) == -350.0);
    ten_json_destroy(j);

    j = ten_json_parse("\"hello\\nworld\"");
    assert(ten_json_type(j) == TEN_JSON_STRING);
    assert(strcmp(ten_json_get_string(j), "hello\nworld") == 0);
    ten_json_destroy(j);

    /* \uXXXX 이스케이프(한글 "가" = U+AC00) */
    j = ten_json_parse("\"\\uac00\"");
    assert(strcmp(ten_json_get_string(j), "\xea\xb0\x80") == 0);
    ten_json_destroy(j);

    /* surrogate pair(😀 = U+1F600) */
    j = ten_json_parse("\"\\ud83d\\ude00\"");
    assert(strcmp(ten_json_get_string(j), "\xf0\x9f\x98\x80") == 0);
    ten_json_destroy(j);
}

static void test_parse_array_and_object(void)
{
    ten_json_t *j;
    ten_json_t *elem;

    j = ten_json_parse("[1, 2, 3]");
    assert(j != NULL);
    assert(ten_json_type(j) == TEN_JSON_ARRAY);
    assert(ten_json_array_count(j) == 3);
    elem = ten_json_array_get(j, 1);
    assert(ten_json_get_number(elem) == 2.0);
    ten_json_destroy(j);

    j = ten_json_parse("[]");
    assert(ten_json_array_count(j) == 0);
    ten_json_destroy(j);

    j = ten_json_parse("{\"name\": \"ten\", \"version\": 1, \"stable\": true}");
    assert(j != NULL);
    assert(ten_json_type(j) == TEN_JSON_OBJECT);
    assert(ten_json_object_count(j) == 3);
    assert(strcmp(ten_json_get_string(ten_json_object_get(j, "name")), "ten") == 0);
    assert(ten_json_get_number(ten_json_object_get(j, "version")) == 1.0);
    assert(ten_json_get_bool(ten_json_object_get(j, "stable")) == 1);

    /* 삽입 순서 보존 확인 */
    assert(strcmp(ten_json_object_key_at(j, 0), "name") == 0);
    assert(strcmp(ten_json_object_key_at(j, 1), "version") == 0);
    assert(strcmp(ten_json_object_key_at(j, 2), "stable") == 0);

    ten_json_destroy(j);

    /* 중첩 */
    j = ten_json_parse("{\"list\": [1, [2, 3], {\"x\": null}]}");
    assert(j != NULL);
    {
        ten_json_t *list = ten_json_object_get(j, "list");
        ten_json_t *nested_array = ten_json_array_get(list, 1);
        ten_json_t *nested_object = ten_json_array_get(list, 2);
        assert(ten_json_array_count(nested_array) == 2);
        assert(ten_json_type(ten_json_object_get(nested_object, "x")) == TEN_JSON_NULL);
    }
    ten_json_destroy(j);
}

static void test_parse_errors(void)
{
    assert(ten_json_parse(NULL) == NULL);
    assert(ten_last_error() == TEN_ERROR_NULL_ARGUMENT);

    assert(ten_json_parse("") == NULL);
    assert(ten_last_error() == TEN_ERROR_PARSE);

    assert(ten_json_parse("{invalid}") == NULL);
    assert(ten_last_error() == TEN_ERROR_PARSE);

    assert(ten_json_parse("[1, 2,]") == NULL); /* 트레일링 콤마는 허용 안 함 */
    assert(ten_last_error() == TEN_ERROR_PARSE);

    assert(ten_json_parse("null null") == NULL); /* 후행 쓰레기 */
    assert(ten_last_error() == TEN_ERROR_PARSE);

    assert(ten_json_parse("{\"a\": 1") == NULL); /* 닫는 중괄호 없음 */
}

static void test_build_and_stringify(void)
{
    ten_json_t *obj;
    ten_json_t *arr;
    char *text;
    ten_json_t *reparsed;

    obj = ten_json_create_object();
    assert(obj != NULL);

    assert(ten_json_object_set(obj, "name", ten_json_create_string(".TEN")) == TEN_OK);
    assert(ten_json_object_set(obj, "version", ten_json_create_number(1.0)) == TEN_OK);

    arr = ten_json_create_array();
    assert(ten_json_array_add(arr, ten_json_create_string("Windows")) == TEN_OK);
    assert(ten_json_array_add(arr, ten_json_create_string("Linux")) == TEN_OK);
    assert(ten_json_array_add(arr, ten_json_create_string("Android")) == TEN_OK);
    assert(ten_json_object_set(obj, "platforms", arr) == TEN_OK);

    /* 같은 key로 다시 set하면 교체되어야 한다(개수 그대로) */
    assert(ten_json_object_set(obj, "version", ten_json_create_number(2.0)) == TEN_OK);
    assert(ten_json_object_count(obj) == 3);
    assert(ten_json_get_number(ten_json_object_get(obj, "version")) == 2.0);

    text = ten_json_stringify(obj);
    assert(text != NULL);

    /* 왕복: 다시 파싱해서 값이 같은지 확인(정확한 텍스트 형태보다 구조를 확인) */
    reparsed = ten_json_parse(text);
    assert(reparsed != NULL);
    assert(strcmp(ten_json_get_string(ten_json_object_get(reparsed, "name")), ".TEN") == 0);
    assert(ten_json_get_number(ten_json_object_get(reparsed, "version")) == 2.0);
    assert(ten_json_array_count(ten_json_object_get(reparsed, "platforms")) == 3);

    ten_free(text);
    ten_json_destroy(reparsed);
    ten_json_destroy(obj);
}

static void test_null_and_invalid(void)
{
    ten_json_t *arr = ten_json_create_array();
    ten_json_t *str = ten_json_create_string("x");

    assert(ten_json_stringify(NULL) == NULL);
    assert(ten_last_error() == TEN_ERROR_NULL_ARGUMENT);

    assert(ten_json_array_add(NULL, str) == (int)TEN_ERROR_NULL_ARGUMENT);
    assert(ten_json_array_add(arr, NULL) == (int)TEN_ERROR_NULL_ARGUMENT);

    /* array가 아닌 값에 array API를 쓰면 오류 */
    assert(ten_json_array_count(str) == 0);
    assert(ten_last_error() == TEN_ERROR_INVALID_STATE);

    ten_json_destroy(str);
    ten_json_destroy(arr);

    /* destroy(NULL)은 안전해야 한다 */
    ten_json_destroy(NULL);
}

int main(void)
{
    test_parse_scalars();
    test_parse_array_and_object();
    test_parse_errors();
    test_build_and_stringify();
    test_null_and_invalid();
    return 0;
}
