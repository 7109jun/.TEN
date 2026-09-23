#ifndef TEN_SYNC_H
#define TEN_SYNC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mutex.
 *
 * ten_mutex_try_lock()의 반환값 중 TEN_ERROR_TIMEOUT은 "지금 당장은
 * 잠글 수 없음"(다른 소유자가 보유 중)을 뜻한다 — 전용 코드가 enum에
 * 없어 의미가 가장 가까운 TIMEOUT을 재사용했다(문서화된 결정).
 */
typedef struct ten_mutex ten_mutex_t;

ten_mutex_t *ten_mutex_create(void);

/*
 * 재귀적으로(같은 스레드가 여러 번) 잠글 수 있는 mutex를 만든다.
 * 사양서 24절이 "지원: ... Recursive Mutex"라고만 명시하고 별도
 * 생성자 시그니처를 정하지 않아 구현자가 추가한 API다.
 */
ten_mutex_t *ten_mutex_create_recursive(void);

void ten_mutex_destroy(ten_mutex_t *mutex);
int ten_mutex_lock(ten_mutex_t *mutex);
int ten_mutex_try_lock(ten_mutex_t *mutex);
int ten_mutex_unlock(ten_mutex_t *mutex);

/*
 * Semaphore.
 */
typedef struct ten_semaphore ten_semaphore_t;

ten_semaphore_t *ten_semaphore_create(unsigned int initial);
int ten_semaphore_wait(ten_semaphore_t *semaphore);
int ten_semaphore_post(ten_semaphore_t *semaphore);
void ten_semaphore_destroy(ten_semaphore_t *semaphore);

/*
 * Condition Variable.
 *
 * 사양서 24절이 "지원: ... Condition Variable"이라고만 명시하고 함수
 * 시그니처를 정하지 않아 구현자가 pthread_cond_t/Win32
 * CONDITION_VARIABLE에 대응하는 형태로 추가했다.
 */
typedef struct ten_cond ten_cond_t;

ten_cond_t *ten_cond_create(void);
void ten_cond_destroy(ten_cond_t *cond);

/* mutex는 호출 전에 잠겨 있어야 하며, 대기 중 잠시 해제되었다가
   반환 전에 다시 잠긴 상태로 돌아온다(pthread_cond_wait와 동일한 관례). */
int ten_cond_wait(ten_cond_t *cond, ten_mutex_t *mutex);

/*
 * @return TEN_OK             signal/broadcast로 깨어남
 * @return TEN_ERROR_TIMEOUT  milliseconds 동안 신호를 받지 못함
 */
int ten_cond_wait_timeout(ten_cond_t *cond, ten_mutex_t *mutex, uint64_t milliseconds);

int ten_cond_signal(ten_cond_t *cond);
int ten_cond_broadcast(ten_cond_t *cond);

#ifdef __cplusplus
}
#endif

#endif /* TEN_SYNC_H */
