#ifndef TEN_JSON_H
#define TEN_JSON_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 지원 값 종류(사양서 31절): null, boolean, number, string, array, object.
 *
 * 사양서는 ten_json_parse/ten_json_stringify/ten_json_destroy만 명시하고
 * 값을 만들거나 들여다보는 API는 규정하지 않았다. accessor/생성 없이는
 * "List/Map/String/Buffer와 연계"(31절)가 실질적으로 불가능하므로
 * 구현자가 아래 create/get/array/object API를 추가했다. 내부적으로
 * object는 ten_map(빠른 조회) + ten_list(삽입 순서 보존)를, array는
 * ten_list를, 문자열 값은 ten_string을 사용한다.
 *
 * Thread Safety: NOT_THREAD_SAFE
 */
typedef enum {
    TEN_JSON_NULL,
    TEN_JSON_BOOL,
    TEN_JSON_NUMBER,
    TEN_JSON_STRING,
    TEN_JSON_ARRAY,
    TEN_JSON_OBJECT
} ten_json_type_t;

typedef struct ten_json ten_json_t;

/*
 * text를 파싱한다.
 * @return TEN_ERROR_NULL_ARGUMENT  text가 NULL(NULL 반환)
 * @return TEN_ERROR_PARSE          문법 오류 또는 후행 쓰레기 문자(NULL 반환)
 * Ownership: 호출자가 소유하며 ten_json_destroy()로 해제한다.
 */
ten_json_t *ten_json_parse(const char *text);

/*
 * Ownership: 반환 문자열은 ten_malloc 소유(ten_free로 해제).
 * json이 NULL이면 NULL을 반환하고 TEN_ERROR_NULL_ARGUMENT를 설정한다.
 */
char *ten_json_stringify(const ten_json_t *json);

/* json이 NULL이면 아무 일도 하지 않는다. array/object의 모든 자식
   요소도 함께 재귀적으로 해제한다. */
void ten_json_destroy(ten_json_t *json);

/* ---- 생성(구현자 추가) ---- */

ten_json_t *ten_json_create_null(void);
ten_json_t *ten_json_create_bool(int value);
ten_json_t *ten_json_create_number(double value);

/* value를 복사해 문자열 값을 만든다. value가 NULL이면 NULL +
   TEN_ERROR_NULL_ARGUMENT. */
ten_json_t *ten_json_create_string(const char *value);

ten_json_t *ten_json_create_array(void);
ten_json_t *ten_json_create_object(void);

/* ---- 조회(구현자 추가) ---- */

/* json이 NULL이면 TEN_JSON_NULL을 반환하고 TEN_ERROR_NULL_ARGUMENT를
   설정한다(값 자체가 TEN_JSON_NULL인 경우와 구분하려면 ten_last_error()
   를 함께 확인한다). */
ten_json_type_t ten_json_type(const ten_json_t *json);

/* json이 NULL이거나 타입이 다르면 0/0.0/NULL을 반환하고
   TEN_ERROR_INVALID_STATE를 설정한다. */
int ten_json_get_bool(const ten_json_t *json);
double ten_json_get_number(const ten_json_t *json);

/* Ownership: json 내부를 가리키는 참조이며 호출자가 해제하지 않는다. */
const char *ten_json_get_string(const ten_json_t *json);

/* ---- Array(구현자 추가) ---- */

size_t ten_json_array_count(const ten_json_t *json);

/* Ownership: array 내부를 가리키는 참조이며 호출자가 해제하지 않는다. */
ten_json_t *ten_json_array_get(const ten_json_t *json, size_t index);

/*
 * value를 배열 끝에 추가한다. 성공하면 value의 소유권을 array가
 * 가져간다(이후 ten_json_destroy(array)가 함께 해제). 실패하면
 * 소유권은 호출자에게 남는다.
 *
 * @return TEN_OK
 * @return TEN_ERROR_NULL_ARGUMENT  json/value가 NULL이거나 json이
 *                                  array가 아닌 경우
 */
int ten_json_array_add(ten_json_t *json, ten_json_t *value);

/* ---- Object(구현자 추가) ---- */

size_t ten_json_object_count(const ten_json_t *json);

/* 존재하지 않으면 NULL을 반환하고 TEN_ERROR_NOT_FOUND를 설정한다.
   Ownership: object 내부를 가리키는 참조이며 호출자가 해제하지 않는다. */
ten_json_t *ten_json_object_get(const ten_json_t *json, const char *key);

/*
 * key를 복사해 저장한다(원본 key 문자열은 호출 후 자유롭게 해제해도
 * 된다). 이미 같은 key가 있으면 기존 값을 해제하고 교체한다(삽입
 * 순서는 최초 등장 위치를 유지한다). 성공하면 value의 소유권을 object가
 * 가져간다. 실패하면 소유권은 호출자에게 남는다.
 *
 * @return TEN_OK
 * @return TEN_ERROR_NULL_ARGUMENT  json/key/value가 NULL이거나 json이
 *                                  object가 아닌 경우
 */
int ten_json_object_set(ten_json_t *json, const char *key, ten_json_t *value);

/* 삽입 순서 기준 index번째 key. 범위를 벗어나면 NULL을 반환한다
   (ten_json_stringify가 내부적으로 쓰는 것과 동일한 순회 방법을
   호출자에게도 제공한다). */
const char *ten_json_object_key_at(const ten_json_t *json, size_t index);

#ifdef __cplusplus
}
#endif

#endif /* TEN_JSON_H */
