#ifndef TEN_FILE_H
#define TEN_FILE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 플랫폼 독립적인 파일 API. 내부적으로 POSIX(Linux/Android)에서는 raw
 * file descriptor를, Windows에서는 HANDLE을 사용한다(사양서 7절).
 *
 * Thread Safety: NOT_THREAD_SAFE (하나의 ten_file_t를 여러 스레드에서
 * 동시에 사용하지 않는다)
 */
typedef struct ten_file ten_file_t;

/*
 * mode는 fopen 스타일 문자열의 일부를 지원한다: "r", "r+", "w", "w+",
 * "a", "a+" (뒤에 'b'가 붙어도 무시된다 — 모든 플랫폼에서 바이너리
 * 모드로 동작한다).
 *
 * Ownership: 호출자가 소유하며 ten_file_close()로 해제한다.
 *
 * @return TEN_ERROR_NULL_ARGUMENT    path 또는 mode가 NULL(NULL 반환)
 * @return TEN_ERROR_INVALID_ARGUMENT mode를 해석할 수 없는 경우(NULL 반환)
 * @return TEN_ERROR_IO               OS 레벨에서 열기 실패(NULL 반환)
 */
ten_file_t *ten_file_open(const char *path, const char *mode);

/* file이 NULL이면 아무 일도 하지 않는다. */
void ten_file_close(ten_file_t *file);

/*
 * 최대 size 바이트를 읽어 buffer에 채운다.
 * @return 실제로 읽은 바이트 수. 0은 EOF 또는 오류를 의미하므로
 *         구분하려면 ten_last_error()를 확인한다(TEN_OK면 EOF).
 */
size_t ten_file_read(ten_file_t *file, void *buffer, size_t size);

/*
 * @return 실제로 쓴 바이트 수. size보다 작으면 오류이며
 *         ten_last_error()에 원인이 설정된다.
 */
size_t ten_file_write(ten_file_t *file, const void *buffer, size_t size);

/* origin: 0 = SEEK_SET, 1 = SEEK_CUR, 2 = SEEK_END */
int ten_file_seek(ten_file_t *file, int64_t offset, int origin);

/* 실패 시 -1을 반환하고 ten_last_error()에 원인이 설정된다. */
int64_t ten_file_size(ten_file_t *file);

/* path가 NULL이면 0을 반환하고 TEN_ERROR_NULL_ARGUMENT를 설정한다. */
int ten_file_exists(const char *path);

/*
 * @return TEN_OK
 * @return TEN_ERROR_NULL_ARGUMENT  path가 NULL
 * @return TEN_ERROR_NOT_FOUND      파일이 존재하지 않음
 * @return TEN_ERROR_IO             기타 OS 오류
 */
int ten_file_delete(const char *path);

/*
 * source를 읽어 destination에 덮어쓴다(이미 존재하면 교체).
 * @return TEN_OK
 * @return TEN_ERROR_NULL_ARGUMENT  source 또는 destination이 NULL
 * @return TEN_ERROR_IO             열기/읽기/쓰기 중 오류
 */
int ten_file_copy(const char *source, const char *destination);

/*
 * @return TEN_OK
 * @return TEN_ERROR_NULL_ARGUMENT  source 또는 destination이 NULL
 * @return TEN_ERROR_IO             OS 오류(예: 존재하지 않는 source)
 */
int ten_file_move(const char *source, const char *destination);

#ifdef __cplusplus
}
#endif

#endif /* TEN_FILE_H */
