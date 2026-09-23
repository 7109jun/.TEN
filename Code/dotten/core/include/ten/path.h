#ifndef TEN_PATH_H
#define TEN_PATH_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 경로 문자열 유틸리티.
 *
 * 조합(ten_path_join)은 플랫폼 separator를 사용한다: Windows는 '\\',
 * 그 외(Linux/Android)는 '/'(사양서 22절: "경로 separator는 운영체제에
 * 따라 .TEN이 처리한다"). 분해(directory/filename/extension)는 입력이
 * 어느 스타일의 separator를 쓰든('/' 또는 '\\') 마지막 separator를
 * 인식해 일관되게 동작한다.
 *
 * Ownership: 모든 반환값은 ten_malloc으로 할당되며 호출자가
 * ten_free()로 해제해야 한다(사양서 35절 예시와 동일한 정책).
 * 실패 시(NULL 인자 또는 OOM) NULL을 반환한다.
 *
 * Thread Safety: THREAD_SAFE (내부 가변 전역 상태 없음)
 */

char *ten_path_join(const char *a, const char *b);
char *ten_path_directory(const char *path);
char *ten_path_filename(const char *path);
char *ten_path_extension(const char *path);

/* path가 NULL이면 0을 반환하고 TEN_ERROR_NULL_ARGUMENT를 설정한다. */
int ten_path_is_absolute(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* TEN_PATH_H */
