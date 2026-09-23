#ifndef TEN_TIME_H
#define TEN_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Thread Safety: THREAD_SAFE
 */

/* 1970-01-01 UTC epoch 기준 현재 시각(밀리초). */
uint64_t ten_time_now_ms(void);

/* 1970-01-01 UTC epoch 기준 현재 시각(마이크로초). */
uint64_t ten_time_now_us(void);

/* 임의 기준점부터의 단조 증가(monotonic) 시각(밀리초). 시스템 시계
   조정의 영향을 받지 않으므로 경과 시간 측정에 적합하다. */
uint64_t ten_time_monotonic_ms(void);

/* 1970-01-01 UTC epoch 기준 현재 시각(초). */
int64_t ten_time_unix(void);

#ifdef __cplusplus
}
#endif

#endif /* TEN_TIME_H */
