#ifndef TEN_HTTP_H
#define TEN_HTTP_H

#include "socket.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * HTTP/1.1 기반 API(사양서 30절). 지원 메서드: GET/POST/PUT/DELETE/
 * HEAD/OPTIONS. ten_http_handler_t 타입만 사양서에 명시되어 있고
 * request/response/server/client의 구체적인 함수는 규정되어 있지 않아
 * 구현자가 아래처럼 채웠다.
 *
 * v1.0 범위의 알려진 제약(문서화된 단순화):
 *   - HTTP/1.1이지만 keep-alive/파이프라이닝은 지원하지 않는다
 *     (연결당 요청 1개, 서버가 매 응답에 사실상 연결을 닫는 것으로 간주).
 *   - chunked transfer-encoding은 지원하지 않는다 — 본문은 항상
 *     Content-Length 기준으로만 읽는다.
 *   - 헤더 이름 조회는 대소문자를 구분하지 않지만(HTTP 표준과 일치),
 *     같은 이름의 헤더가 여러 번 오면 마지막 값만 남는다(병합하지 않음).
 *
 * Thread Safety: NOT_THREAD_SAFE
 */

typedef enum {
    TEN_HTTP_GET,
    TEN_HTTP_POST,
    TEN_HTTP_PUT,
    TEN_HTTP_DELETE,
    TEN_HTTP_HEAD,
    TEN_HTTP_OPTIONS
} ten_http_method_t;

typedef struct ten_http_request ten_http_request_t;
typedef struct ten_http_response ten_http_response_t;

typedef void (*ten_http_handler_t)(
    const ten_http_request_t *request,
    ten_http_response_t *response,
    void *userdata
);

/* ---- Request ----
 * 서버가 들어온 요청을 파싱해 만들거나(핸들러에 전달), 클라이언트가
 * 보낼 요청을 만들 때(ten_http_request_create) 양쪽에 같은 타입을 쓴다. */

ten_http_request_t *ten_http_request_create(ten_http_method_t method, const char *path);
void ten_http_request_destroy(ten_http_request_t *request);

/* 같은 이름이 이미 있으면 값을 덮어쓴다(대소문자 구분 없이 비교). */
int ten_http_request_set_header(ten_http_request_t *request, const char *name, const char *value);
int ten_http_request_set_body(ten_http_request_t *request, const void *data, size_t size);

ten_http_method_t ten_http_request_method(const ten_http_request_t *request);
const char *ten_http_request_path(const ten_http_request_t *request);

/* 없으면 NULL. Ownership: 참조이며 호출자가 해제하지 않는다. */
const char *ten_http_request_header(const ten_http_request_t *request, const char *name);

/* body가 없으면 NULL을 반환하고 *out_size에 0을 쓴다. */
const void *ten_http_request_body(const ten_http_request_t *request, size_t *out_size);

/* ---- Response ----
 * 서버 핸들러가 채우거나(create는 서버 내부에서만 함 — 핸들러는 이미
 * 만들어진 response를 받는다), 클라이언트가 ten_http_client_send()의
 * 결과로 받아 읽는다. */

int ten_http_response_set_status(ten_http_response_t *response, int status_code);
int ten_http_response_set_header(ten_http_response_t *response, const char *name, const char *value);
int ten_http_response_set_body(ten_http_response_t *response, const void *data, size_t size);

int ten_http_response_status(const ten_http_response_t *response);
const char *ten_http_response_header(const ten_http_response_t *response, const char *name);
const void *ten_http_response_body(const ten_http_response_t *response, size_t *out_size);

void ten_http_response_destroy(ten_http_response_t *response);

/* ---- Client ---- */

/*
 * host:port로 연결해 request를 보내고 응답을 읽어 돌려준다(동기).
 * request는 이 호출이 소유권을 가져가지 않는다 — 호출자가 그대로
 * ten_http_request_destroy()로 해제한다.
 *
 * Ownership: 반환된 response는 호출자가 ten_http_response_destroy()로
 * 해제한다.
 */
ten_http_response_t *ten_http_client_send(const char *host, uint16_t port,
                                           const ten_http_request_t *request);

/* ---- Server ---- */

typedef struct ten_http_server ten_http_server_t;

ten_http_server_t *ten_http_server_create(const char *host, uint16_t port,
                                           ten_http_handler_t handler, void *userdata);

/*
 * 연결을 하나씩 accept해서 처리하는 블로킹 루프(내부적으로 짧은
 * 타임아웃으로 poll하며 ten_http_server_stop() 플래그를 주기적으로
 * 확인한다). 다른 스레드에서 ten_http_server_stop()이 호출되면 반환한다.
 */
int ten_http_server_run(ten_http_server_t *server);

/* run()에게 멈추라는 신호를 보낸다(협조적 — 다음 poll 주기 안에
   반영됨). 즉시 반환을 보장하지는 않는다 — 처리 중인 연결이 있으면
   그것부터 끝난다. run()이 실제로 반환(스레드 join)한 뒤에
   ten_http_server_destroy()를 호출한다. */
void ten_http_server_stop(ten_http_server_t *server);

void ten_http_server_destroy(ten_http_server_t *server);

#ifdef __cplusplus
}
#endif

#endif /* TEN_HTTP_H */
