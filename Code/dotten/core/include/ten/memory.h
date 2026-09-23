#ifndef TEN_MEMORY_H
#define TEN_MEMORY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 기본 메모리 API.
 *
 * Ownership: 호출자가 반환된 포인터를 소유하며 ten_free()로 해제한다.
 * Thread Safety: THREAD_SAFE (기본 allocator 및 정상적인 커스텀 allocator 기준)
 *
 * 실패 시 NULL을 반환하고 ten_last_error()가 TEN_ERROR_OUT_OF_MEMORY로
 * 설정된다. size/count가 0인 호출은 구현체(libc) 동작을 그대로 따른다.
 */
void *ten_malloc(size_t size);

void *ten_calloc(size_t count, size_t size);

void *ten_realloc(void *ptr, size_t size);

/*
 * `.TEN`이 반환한(또는 ten_set_allocator로 등록된 allocator가 반환한)
 * 동적 메모리를 해제한다. ptr이 NULL이면 아무 일도 하지 않는다.
 */
void ten_free(void *ptr);

/*
 * data를 size 바이트만큼 복사한 새 버퍼를 할당해 반환한다.
 * 실패 시 NULL, ten_last_error() == TEN_ERROR_OUT_OF_MEMORY.
 * data가 NULL이면 NULL을 반환하고 TEN_ERROR_NULL_ARGUMENT를 설정한다.
 */
void *ten_memdup(const void *data, size_t size);

/*
 * 사용자 정의 Allocator.
 *
 * 네 함수 포인터 모두 채워져 있어야 한다.
 */
typedef struct {
    void *(*malloc_fn)(size_t);
    void *(*calloc_fn)(size_t, size_t);
    void *(*realloc_fn)(void *, size_t);
    void (*free_fn)(void *);
} ten_allocator_t;

/*
 * 커스텀 allocator를 등록한다. 설정된 이후 `.TEN`의 내부 동적 할당은
 * 가능한 한 이 allocator를 사용한다.
 *
 * @return TEN_OK
 * @return TEN_ERROR_NULL_ARGUMENT  allocator가 NULL이거나 함수 포인터 중
 *                                  하나라도 NULL인 경우
 *
 * Thread Safety: NOT_THREAD_SAFE — 다른 스레드가 메모리 API를 사용 중일
 * 때 호출하지 않는다. 일반적으로 ten_init() 직후 한 번만 호출한다.
 */
int ten_set_allocator(const ten_allocator_t *allocator);

#ifdef __cplusplus
}
#endif

#endif /* TEN_MEMORY_H */
