#ifndef TEN_INTERNAL_ERROR_INTERNAL_H
#define TEN_INTERNAL_ERROR_INTERNAL_H

#include "ten/error.h"

/*
 * 내부 전용 API.
 *
 * core/include/ten/ 아래에 설치되지 않으며, .TEN 내부 모듈 구현에서만
 * 사용한다. 각 public API 구현은 성공/실패 결과를 여기로 기록해서
 * ten_last_error()가 최신 상태를 반영하도록 한다.
 */
void ten__set_error(ten_error_code_t code);

#endif /* TEN_INTERNAL_ERROR_INTERNAL_H */
