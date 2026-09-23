#ifndef TEN_THREAD_H
#define TEN_THREAD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ten_thread ten_thread_t;
typedef void *(*ten_thread_fn)(void *userdata);

/*
 * @return TEN_ERROR_NULL_ARGUMENT  function이 NULL(NULL 반환)
 * @return TEN_ERROR_SYSTEM         OS 스레드 생성 실패(NULL 반환)
 *
 * Ownership: 호출자가 소유하며 ten_thread_join()으로 회수한다.
 */
ten_thread_t *ten_thread_create(ten_thread_fn function, void *userdata);

/*
 * 스레드가 끝날 때까지 대기하고 반환값을 result에 저장한다(result가
 * NULL이면 무시). 호출이 성공/실패하든 관계없이 thread의 내부 자원은
 * 이 호출로 회수되며, 이후 thread 포인터는 더 이상 사용할 수 없다.
 *
 * @return TEN_OK
 * @return TEN_ERROR_NULL_ARGUMENT  thread가 NULL
 * @return TEN_ERROR_SYSTEM         OS join 실패
 */
int ten_thread_join(ten_thread_t *thread, void **result);

void ten_thread_sleep(uint64_t milliseconds);

/* 현재 스레드를 식별하는 값(플랫폼 간 비교 가능성은 보장하지 않음). */
uint64_t ten_thread_current_id(void);

#ifdef __cplusplus
}
#endif

#endif /* TEN_THREAD_H */
