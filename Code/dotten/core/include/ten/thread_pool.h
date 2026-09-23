#ifndef TEN_THREAD_POOL_H
#define TEN_THREAD_POOL_H

#include "thread.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ten_thread, ten_mutex, ten_cond 위에 구현된 고정 크기 워커 풀 +
 * 작업 큐. 플랫폼 종속 코드가 없는 portable 구현이다.
 */
typedef struct ten_thread_pool ten_thread_pool_t;

/*
 * worker_count개의 워커 스레드를 미리 생성한다.
 *
 * @return TEN_ERROR_INVALID_ARGUMENT  worker_count == 0(NULL 반환)
 * @return TEN_ERROR_SYSTEM            워커 스레드 생성 실패(NULL 반환)
 *
 * Ownership: 호출자가 소유하며 ten_thread_pool_destroy()로 해제한다.
 */
ten_thread_pool_t *ten_thread_pool_create(size_t worker_count);

/*
 * function(userdata)를 작업 큐에 넣는다. 유휴 워커가 있으면 즉시
 * 실행되고, 없으면 대기열에서 기다린다.
 *
 * @return TEN_OK
 * @return TEN_ERROR_NULL_ARGUMENT  pool 또는 function이 NULL
 * @return TEN_ERROR_INVALID_STATE  pool이 이미 destroy 중/후인 경우
 * @return TEN_ERROR_OUT_OF_MEMORY  큐 노드 할당 실패
 */
int ten_thread_pool_submit(ten_thread_pool_t *pool, ten_thread_fn function, void *userdata);

/*
 * 모든 워커를 종료하고 자원을 해제한다. pool이 NULL이면 아무 일도
 * 하지 않는다. 이미 큐에 들어간 작업은 끝까지 실행되지만, destroy
 * 호출 이후 도착하는 ten_thread_pool_submit()은 TEN_ERROR_INVALID_STATE로
 * 거부된다.
 */
void ten_thread_pool_destroy(ten_thread_pool_t *pool);

#ifdef __cplusplus
}
#endif

#endif /* TEN_THREAD_POOL_H */
