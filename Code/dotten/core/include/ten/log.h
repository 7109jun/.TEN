#ifndef TEN_LOG_H
#define TEN_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TEN_LOG_TRACE,
    TEN_LOG_DEBUG,
    TEN_LOG_INFO,
    TEN_LOG_WARN,
    TEN_LOG_ERROR,
    TEN_LOG_FATAL
} ten_log_level_t;

/*
 * Thread Safety: NOT_THREAD_SAFE (M3의 Synchronization 모듈 도입 이후
 * 내부 mutex로 강화 예정 — 여러 스레드에서 동시에 로그를 남기거나
 * ten_log_set_*를 호출할 경우 현재는 호출자가 동기화해야 한다.)
 *
 * format이 NULL이면 아무 일도 하지 않고 TEN_ERROR_NULL_ARGUMENT를 설정한다.
 * level이 현재 최소 레벨(기본값 TEN_LOG_TRACE, ten_log_set_level 참고)
 * 보다 낮으면 아무 출력도 하지 않는다.
 * 메시지는 내부 고정 버퍼(4096바이트)로 포맷되며 이를 초과하면 잘린다.
 */
void ten_log(ten_log_level_t level, const char *format, ...);

void ten_log_trace(const char *format, ...);
void ten_log_debug(const char *format, ...);
void ten_log_info(const char *format, ...);
void ten_log_warn(const char *format, ...);
void ten_log_error(const char *format, ...);
void ten_log_fatal(const char *format, ...);

typedef void (*ten_log_callback_t)(
    ten_log_level_t level,
    const char *message,
    void *userdata
);

/*
 * 아래 ten_log_set_* 함수들은 사양서 28절이 명시한 "지원 출력: Console,
 * File, Callback"을 실제로 설정하기 위해 구현자가 추가한 API다(사양서
 * 본문에는 시그니처가 명시되어 있지 않음). 기본값: console 출력 ON,
 * file/callback 없음, 최소 레벨 TEN_LOG_TRACE(전부 출력).
 */

/* level 미만의 로그를 걸러낸다. */
void ten_log_set_level(ten_log_level_t level);

/* console(stdout/stderr) 출력을 켜고 끈다. WARN 이상은 stderr, 그
   미만은 stdout으로 나간다. */
void ten_log_set_console(int enabled);

/*
 * 로그를 append 모드로 기록할 파일을 지정한다. path가 NULL이면 기존에
 * 열려 있던 로그 파일을 닫고 file 출력을 끈다.
 *
 * @return TEN_OK
 * @return TEN_ERROR_IO  파일을 열지 못한 경우
 */
int ten_log_set_file(const char *path);

/* callback이 NULL이면 콜백 출력을 끈다. */
void ten_log_set_callback(ten_log_callback_t callback, void *userdata);

#ifdef __cplusplus
}
#endif

#endif /* TEN_LOG_H */
