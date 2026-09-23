#ifndef TEN_ENCODING_H
#define TEN_ENCODING_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 기본 지원(사양서 32절): UTF-8, UTF-16, ASCII, Base64, Hex.
 *
 * Base64만 함수 시그니처가 명시되어 있다. Hex/UTF 변환/유효성 검사는
 * "UTF 변환 API도 제공한다"는 서술만 있고 구체적인 시그니처가 없어
 * 구현자가 Base64와 같은 관례(ten_malloc 소유 반환, NULL+오류코드)로
 * 추가했다.
 *
 * Thread Safety: THREAD_SAFE(모두 순수 함수, 내부 가변 상태 없음)
 */

/* ---- Base64 ---- */

/* Ownership: 반환 문자열은 ten_malloc 소유(ten_free로 해제). */
char *ten_base64_encode(const void *data, size_t size);

/*
 * text가 4의 배수 길이가 아니거나 유효하지 않은 문자를 포함하면 NULL을
 * 반환하고 TEN_ERROR_ENCODING을 설정한다.
 * Ownership: 반환 버퍼는 ten_malloc 소유(ten_free로 해제).
 */
void *ten_base64_decode(const char *text, size_t *size);

/* ---- Hex(구현자 추가) ---- */

char *ten_hex_encode(const void *data, size_t size);

/* text 길이가 홀수이거나 16진수가 아닌 문자가 있으면 NULL +
   TEN_ERROR_ENCODING. */
void *ten_hex_decode(const char *text, size_t *size);

/* ---- UTF 변환(구현자 추가) ---- */

/*
 * UTF-8 -> UTF-16(host endianness). 잘못된 UTF-8 시퀀스면 NULL +
 * TEN_ERROR_ENCODING. out_length에 NUL 제외 UTF-16 코드 유닛 수를 쓴다.
 * Ownership: 반환 버퍼(NUL 종단)는 ten_malloc 소유.
 */
uint16_t *ten_utf8_to_utf16(const char *utf8, size_t *out_length);

/*
 * UTF-16(length 코드 유닛) -> UTF-8. 짝이 맞지 않는 surrogate가 있으면
 * NULL + TEN_ERROR_ENCODING.
 * Ownership: 반환 문자열(NUL 종단)은 ten_malloc 소유.
 */
char *ten_utf16_to_utf8(const uint16_t *utf16, size_t length);

/* ---- 유효성 검사(구현자 추가) ---- */

/* size 바이트가 모두 0x00~0x7F이면 1, 아니면 0. */
int ten_ascii_validate(const char *text, size_t size);

/* size 바이트가 유효한 UTF-8이면 1, 아니면 0. */
int ten_utf8_validate(const char *text, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* TEN_ENCODING_H */
