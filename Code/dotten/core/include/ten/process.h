#ifndef TEN_PROCESS_H
#define TEN_PROCESS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 현재 프로세스 ID. */
uint64_t ten_process_id(void);

/*
 * program을 argv(NULL로 끝나야 함)와 함께 실행하고, 자식 프로세스가
 * 종료할 때까지 대기한다(동기 실행).
 *
 * 반환값은 실행 성공 여부만 나타낸다 — 시그니처가 사양서에 고정되어
 * 있어(int 하나) 자식 프로세스의 실제 종료 코드를 함께 돌려줄 자리가
 * 없다. 종료 코드가 필요하면 v1.0 이후 별도 API(예:
 * ten_process_execute_ex)로 확장한다.
 *
 * @return TEN_OK             자식 프로세스가 실행되어 끝까지 마침(종료
 *                             코드 값과 무관하게 성공으로 취급)
 * @return TEN_ERROR_NULL_ARGUMENT  program 또는 argv가 NULL
 * @return TEN_ERROR_SYSTEM   프로세스를 만들거나 기다리는 데 실패
 *                             (program을 찾지 못한 경우 포함 — 자식이
 *                             exec 실패 시 종료 코드 127로 끝나므로
 *                             ten_process_execute() 자체는 TEN_OK를
 *                             반환할 수 있다; 존재 확인은 ten_file_exists
 *                             등으로 호출자가 사전에 하는 것을 권장한다)
 */
int ten_process_execute(const char *program, const char *const *argv);

/* 존재하지 않는 환경 변수면 NULL을 반환한다(오류는 아니다). */
const char *ten_env_get(const char *name);

/*
 * @return TEN_OK
 * @return TEN_ERROR_NULL_ARGUMENT  name 또는 value가 NULL
 * @return TEN_ERROR_SYSTEM         OS 호출 실패
 */
int ten_env_set(const char *name, const char *value);

#ifdef __cplusplus
}
#endif

#endif /* TEN_PROCESS_H */
