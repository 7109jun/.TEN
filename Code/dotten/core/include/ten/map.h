#ifndef TEN_MAP_H
#define TEN_MAP_H

#include "type.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ten_map — hash table 기반 key-value collection.
 *
 * element 저장 정책은 ten_list와 동일하다(core/src/internal/type_internal.h):
 * 고정폭 스칼라 타입은 값 자체, STRING/BUFFER/POINTER는 포인터 값을
 * raw byte copy로 저장한다. 단, 해시/동등 비교 정책은 다음과 같다:
 *
 *   - TEN_TYPE_STRING 키: 포인터가 가리키는 C 문자열의 "내용"을 기준으로
 *     해시/비교한다(strcmp 기반) — 문자열을 키로 쓰는 일반적인 용례를
 *     지원하기 위함이다.
 *   - 그 외 타입(고정폭 스칼라, BUFFER, POINTER): 저장된 raw byte를
 *     그대로 기준으로 해시/비교한다(BUFFER/POINTER는 사실상 포인터
 *     값 자체가 비교된다 — 참조 동일성).
 *
 * 평균 성능 목표(사양서 18절): lookup/insert/remove average O(1).
 *
 * Thread Safety: NOT_THREAD_SAFE — 내부 locking을 하지 않으므로 여러
 * 스레드에서 같은 map에 접근할 경우 호출자가 동기화해야 한다(36절).
 */
typedef struct ten_map ten_map_t;

/*
 * @return 새 map. 실패 시 NULL.
 * @return TEN_ERROR_INVALID_ARGUMENT  key_type 또는 value_type이
 *                                     TEN_TYPE_VOID인 경우
 * @return TEN_ERROR_OUT_OF_MEMORY     할당 실패
 *
 * Ownership: 호출자가 소유하며 ten_map_destroy()로 해제한다.
 */
ten_map_t *ten_map_create(ten_type_t key_type, ten_type_t value_type);

/* map이 NULL이면 아무 일도 하지 않는다. */
void ten_map_destroy(ten_map_t *map);

/*
 * key에 해당하는 값을 설정한다. 이미 존재하는 key면 값을 덮어쓴다.
 *
 * @return TEN_OK
 * @return TEN_ERROR_NULL_ARGUMENT  map, key, value 중 하나라도 NULL
 * @return TEN_ERROR_OUT_OF_MEMORY  내부 할당 실패
 */
int ten_map_set(ten_map_t *map, const void *key, const void *value);

/*
 * key에 해당하는 값의 내부 저장소에 대한 포인터를 반환한다.
 *
 * Ownership: map 내부를 가리키는 참조이며 호출자가 해제하지 않는다.
 * 해당 key가 set/remove로 갱신되거나 map이 rehash되면 무효화될 수 있다.
 *
 * 실패 시 NULL을 반환하고 다음 중 하나가 설정된다:
 * TEN_ERROR_NULL_ARGUMENT(map == NULL || key == NULL), TEN_ERROR_NOT_FOUND
 */
void *ten_map_get(ten_map_t *map, const void *key);

/*
 * @return TEN_OK
 * @return TEN_ERROR_NULL_ARGUMENT  map 또는 key가 NULL
 * @return TEN_ERROR_NOT_FOUND      key가 존재하지 않는 경우
 */
int ten_map_remove(ten_map_t *map, const void *key);

/* @return 0(없음)/1(있음). map 또는 key가 NULL이면 0을 반환하고
 * TEN_ERROR_NULL_ARGUMENT를 설정한다. */
int ten_map_contains(const ten_map_t *map, const void *key);

/* map이 NULL이면 0을 반환하고 TEN_ERROR_NULL_ARGUMENT를 설정한다. */
size_t ten_map_count(const ten_map_t *map);

#ifdef __cplusplus
}
#endif

#endif /* TEN_MAP_H */
