#ifndef TEN_STRING_H
#define TEN_STRING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ten_string — 동적 문자열. 기본 문자 인코딩은 UTF-8이다(사양서 19절).
 * 내부적으로 항상 NUL로 끝나는 버퍼를 유지한다.
 *
 * Thread Safety: NOT_THREAD_SAFE
 */
typedef struct ten_string ten_string_t;

/*
 * text를 복사해 새 문자열을 만든다.
 *
 * @return TEN_ERROR_NULL_ARGUMENT  text가 NULL인 경우(NULL 반환)
 * @return TEN_ERROR_OUT_OF_MEMORY  할당 실패(NULL 반환)
 *
 * Ownership: 호출자가 소유하며 ten_string_destroy()로 해제한다.
 */
ten_string_t *ten_string_create(const char *text);

/* 빈 문자열("")을 만든다. */
ten_string_t *ten_string_empty(void);

/* string이 NULL이면 아무 일도 하지 않는다. */
void ten_string_destroy(ten_string_t *string);

/*
 * Ownership: string 내부를 가리키는 NUL-terminated 참조이며 호출자가
 * 해제하지 않는다. string이 변경되거나 파괴되면 무효화된다.
 * string이 NULL이면 NULL을 반환하고 TEN_ERROR_NULL_ARGUMENT를 설정한다.
 */
const char *ten_string_cstr(const ten_string_t *string);

/* string이 NULL이면 0을 반환하고 TEN_ERROR_NULL_ARGUMENT를 설정한다. */
size_t ten_string_length(const ten_string_t *string);

/*
 * text를 string 끝에 덧붙인다.
 *
 * @return TEN_OK
 * @return TEN_ERROR_NULL_ARGUMENT  string 또는 text가 NULL
 * @return TEN_ERROR_OUT_OF_MEMORY  확장 실패
 */
int ten_string_append(ten_string_t *string, const char *text);

/*
 * @return 1(같음)/0(다름 또는 오류).
 * a, b 중 하나라도 NULL이면 0을 반환하고 TEN_ERROR_NULL_ARGUMENT를 설정한다.
 */
int ten_string_equals(const ten_string_t *a, const ten_string_t *b);

/*
 * @return 1(포함)/0(미포함 또는 오류).
 * string 또는 text가 NULL이면 0을 반환하고 TEN_ERROR_NULL_ARGUMENT를 설정한다.
 */
int ten_string_contains(const ten_string_t *string, const char *text);

/*
 * [start, start+length) 구간의 부분 문자열을 새 ten_string_t로 반환한다.
 *
 * @return TEN_ERROR_NULL_ARGUMENT  string이 NULL(NULL 반환)
 * @return TEN_ERROR_OUT_OF_RANGE   start + length > ten_string_length(string)
 *                                  (NULL 반환)
 *
 * Ownership: 호출자가 소유하며 ten_string_destroy()로 해제한다.
 */
ten_string_t *ten_string_substring(const ten_string_t *string, size_t start, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* TEN_STRING_H */
