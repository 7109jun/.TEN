#ifndef TEN_CORE_H
#define TEN_CORE_H

#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 초기화.
 *
 * Thread Safety: NOT_THREAD_SAFE (M3 Synchronization 모듈 도입 후 강화 예정)
 *
 * @return TEN_OK                         성공
 * @return TEN_ERROR_ALREADY_INITIALIZED  이미 초기화된 경우
 */
int ten_init(void);

/*
 * 초기화 상태 확인.
 *
 * @return 0이 아니면 초기화된 상태, 0이면 초기화되지 않은 상태.
 */
int ten_is_initialized(void);

/*
 * 종료.
 *
 * 초기화되지 않은 상태에서 호출해도 안전하다(no-op).
 */
void ten_shutdown(void);

#define TEN_VERSION_MAJOR 1
#define TEN_VERSION_MINOR 0
#define TEN_VERSION_PATCH 0

/*
 * 버전 문자열 조회 ("MAJOR.MINOR.PATCH").
 *
 * Ownership: 호출자가 해제할 필요 없는 정적 문자열이다.
 * Thread Safety: THREAD_SAFE
 */
const char *ten_version(void);

int ten_version_major(void);
int ten_version_minor(void);
int ten_version_patch(void);

#ifdef __cplusplus
}
#endif

#endif /* TEN_CORE_H */
