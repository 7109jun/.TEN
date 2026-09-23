#ifndef TEN_PLUGIN_H
#define TEN_PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 동적 라이브러리 로딩(사양서 33절). 플랫폼별 형식: Windows → .dll,
 * Linux/Android → .so. 내부적으로 POSIX는 dlopen/dlsym/dlclose,
 * Windows는 LoadLibraryA/GetProcAddress/FreeLibrary를 사용한다.
 *
 * Thread Safety: NOT_THREAD_SAFE
 */
typedef struct ten_plugin ten_plugin_t;

/*
 * Ownership: 호출자가 소유하며 ten_plugin_unload()로 해제한다.
 * @return TEN_ERROR_NULL_ARGUMENT  path가 NULL(NULL 반환)
 * @return TEN_ERROR_IO             라이브러리를 열지 못함(NULL 반환)
 */
ten_plugin_t *ten_plugin_load(const char *path);

/*
 * name 심볼의 주소를 반환한다. 없으면 NULL + TEN_ERROR_NOT_FOUND.
 * Ownership: 라이브러리 내부를 가리키는 참조이며 plugin이 unload되기
 * 전까지만 유효하다.
 */
void *ten_plugin_symbol(ten_plugin_t *plugin, const char *name);

/* plugin이 NULL이면 아무 일도 하지 않는다. */
void ten_plugin_unload(ten_plugin_t *plugin);

/* ---- Plugin ABI(사양서 34절) ---- */

#define TEN_PLUGIN_ABI_VERSION 1

/*
 * 진입점 계약(콜백 시그니처)이다 — **`.TEN` 코어 라이브러리 자체는 이
 * 함수들을 구현하지 않는다.** 플러그인(.dll/.so) 저자가 정확히 이
 * 시그니처로 "ten_plugin_init"/"ten_plugin_shutdown"이라는 이름의
 * 함수를 자신의 플러그인 소스에서 구현해 내보내야(export) 한다. host
 * 쪽은 로드 후 ten_plugin_symbol()로 이 이름을 찾아 아래 타입으로
 * 캐스팅해서 호출한다(POSIX dlsym이 함수 포인터로의 캐스팅을 명시적으로
 * 지원하는 관용적인 패턴이다).
 */
typedef int (*ten_plugin_init_fn)(void);
typedef void (*ten_plugin_shutdown_fn)(void);

/*
 * 편의 함수(구현자 추가): plugin에서 "ten_plugin_init" 심볼을 찾아
 * 있으면 호출하고 그 반환값을 돌려준다.
 *
 * @return TEN_ERROR_NULL_ARGUMENT  plugin이 NULL
 * @return TEN_ERROR_NOT_SUPPORTED  플러그인이 ten_plugin_init을
 *                                  내보내지 않음
 * 그 외의 경우 플러그인의 ten_plugin_init()이 반환한 값 그대로.
 */
int ten_plugin_call_init(ten_plugin_t *plugin);

/* "ten_plugin_shutdown" 심볼이 있으면 호출한다(없어도 오류 아님). */
void ten_plugin_call_shutdown(ten_plugin_t *plugin);

#ifdef __cplusplus
}
#endif

#endif /* TEN_PLUGIN_H */
