#ifndef TEN_ERROR_H
#define TEN_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * .TEN 사양서 13.1절 오류 코드.
 */
typedef enum {
    TEN_OK = 0,

    TEN_ERROR_UNKNOWN,
    TEN_ERROR_INVALID_ARGUMENT,
    TEN_ERROR_NULL_ARGUMENT,
    TEN_ERROR_OUT_OF_MEMORY,
    TEN_ERROR_OUT_OF_RANGE,
    TEN_ERROR_NOT_FOUND,
    TEN_ERROR_ALREADY_EXISTS,
    TEN_ERROR_ACCESS_DENIED,
    TEN_ERROR_NOT_SUPPORTED,
    TEN_ERROR_INVALID_STATE,
    TEN_ERROR_TIMEOUT,
    TEN_ERROR_CANCELLED,
    TEN_ERROR_IO,
    TEN_ERROR_NETWORK,
    TEN_ERROR_PARSE,
    TEN_ERROR_ENCODING,
    TEN_ERROR_SYSTEM,
    TEN_ERROR_ALREADY_INITIALIZED,
    TEN_ERROR_NOT_INITIALIZED
} ten_error_code_t;

/*
 * 현재 스레드의 마지막 오류 코드를 조회한다.
 *
 * 오류 상태는 스레드별로 분리된다(사양서 13절). 각 public API 호출은
 * 성공/실패와 무관하게 자신의 결과를 이 스레드-로컬 상태에 반영한다.
 *
 * Thread Safety: THREAD_SAFE (호출 스레드 자신의 상태만 조회)
 */
ten_error_code_t ten_last_error(void);

/*
 * 오류 코드에 대한 사람이 읽을 수 있는 메시지를 반환한다.
 *
 * Ownership: 정적 문자열이며 호출자가 해제하지 않는다.
 * Thread Safety: THREAD_SAFE
 * 정의되지 않은 코드가 들어오면 "Unrecognized error code"를 반환한다.
 */
const char *ten_error_message(ten_error_code_t code);

/*
 * 현재 스레드의 마지막 오류 상태를 TEN_OK로 초기화한다.
 *
 * Thread Safety: THREAD_SAFE
 */
void ten_clear_error(void);

#ifdef __cplusplus
}
#endif

#endif /* TEN_ERROR_H */
