#ifndef TEN_LIST_H
#define TEN_LIST_H

#include "type.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ten_list — 동적 배열 기반 컬렉션.
 *
 * element 저장 정책은 core/src/internal/type_internal.h의 설명을 따른다:
 * 고정폭 스칼라 타입은 값 자체를, STRING/BUFFER/POINTER는 포인터 값
 * (참조)을 ten_type_t 크기만큼 raw byte copy로 저장한다.
 *
 * 성능 목표(사양서 17절): indexed access O(1), append amortized O(1),
 * insert/remove O(n).
 *
 * Thread Safety: NOT_THREAD_SAFE — 내부 locking을 하지 않으므로 여러
 * 스레드에서 같은 list에 접근할 경우 호출자가 동기화해야 한다(36절).
 */
typedef struct ten_list ten_list_t;

/*
 * @return 새 list. 실패 시 NULL.
 * @return TEN_ERROR_INVALID_ARGUMENT  type이 TEN_TYPE_VOID인 경우
 * @return TEN_ERROR_OUT_OF_MEMORY     할당 실패
 *
 * Ownership: 호출자가 소유하며 ten_list_destroy()로 해제한다.
 */
ten_list_t *ten_list_create(ten_type_t type);

/* list가 NULL이면 아무 일도 하지 않는다. */
void ten_list_destroy(ten_list_t *list);

/*
 * list의 끝에 value가 가리키는 element(ten_type_t 크기만큼)를 복사해
 * 추가한다.
 *
 * @return TEN_OK
 * @return TEN_ERROR_NULL_ARGUMENT  list 또는 value가 NULL인 경우
 * @return TEN_ERROR_OUT_OF_MEMORY  내부 배열 확장 실패
 */
int ten_list_add(ten_list_t *list, const void *value);

/*
 * index 위치에 value를 삽입한다. index == ten_list_count(list)이면
 * 끝에 추가하는 것과 같다.
 *
 * @return TEN_OK
 * @return TEN_ERROR_NULL_ARGUMENT  list 또는 value가 NULL인 경우
 * @return TEN_ERROR_OUT_OF_RANGE   index > count인 경우
 * @return TEN_ERROR_OUT_OF_MEMORY  내부 배열 확장 실패
 */
int ten_list_insert(ten_list_t *list, size_t index, const void *value);

/*
 * @return TEN_OK
 * @return TEN_ERROR_NULL_ARGUMENT  list가 NULL인 경우
 * @return TEN_ERROR_OUT_OF_RANGE   index >= count인 경우
 */
int ten_list_remove_at(ten_list_t *list, size_t index);

/*
 * index 위치 element의 내부 저장소에 대한 포인터를 반환한다.
 *
 * Ownership: list 내부를 가리키는 참조이며 호출자가 해제하지 않는다.
 * list가 수정(add/insert/remove/reserve)되면 무효화될 수 있다.
 *
 * 실패 시 NULL을 반환하고 다음 중 하나가 설정된다:
 * TEN_ERROR_NULL_ARGUMENT(list == NULL), TEN_ERROR_OUT_OF_RANGE(index >= count)
 */
void *ten_list_get(ten_list_t *list, size_t index);

/* list가 NULL이면 0을 반환하고 TEN_ERROR_NULL_ARGUMENT를 설정한다. */
size_t ten_list_count(const ten_list_t *list);

/* list가 NULL이면 0을 반환하고 TEN_ERROR_NULL_ARGUMENT를 설정한다. */
size_t ten_list_capacity(const ten_list_t *list);

/*
 * 내부 배열이 최소 capacity개의 element를 담을 수 있도록 미리 확보한다.
 * capacity가 현재 capacity 이하이면 아무 일도 하지 않고 TEN_OK를 반환한다.
 *
 * @return TEN_OK
 * @return TEN_ERROR_NULL_ARGUMENT  list가 NULL인 경우
 * @return TEN_ERROR_OUT_OF_MEMORY  확장 실패
 */
int ten_list_reserve(ten_list_t *list, size_t capacity);

#ifdef __cplusplus
}
#endif

#endif /* TEN_LIST_H */
