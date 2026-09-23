#ifndef TEN_SOCKET_H
#define TEN_SOCKET_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * TCP/UDP 네트워크 API(사양서 29절). TCP는 시그니처가 명시되어 있고,
 * UDP는 "동일한 naming convention으로 제공한다"고만 되어 있어 구현자가
 * ten_udp_* 이름으로 아래처럼 확정했다.
 *
 * Thread Safety: NOT_THREAD_SAFE — 하나의 ten_socket_t를 여러 스레드에서
 * 동시에 send/receive하지 않는다.
 */
typedef struct ten_socket ten_socket_t;

/* ---- TCP ---- */

ten_socket_t *ten_tcp_connect(const char *host, uint16_t port);
ten_socket_t *ten_tcp_listen(const char *host, uint16_t port, int backlog);
ten_socket_t *ten_tcp_accept(ten_socket_t *server);

/* ---- UDP(구현자 추가, ten_tcp_*와 동일한 관례) ---- */

/* 수신용으로 로컬 주소에 바인딩한다. */
ten_socket_t *ten_udp_bind(const char *host, uint16_t port);

/* 바인딩 없이 송신 전용으로 쓸 소켓을 만든다. */
ten_socket_t *ten_udp_socket(void);

int64_t ten_udp_send_to(ten_socket_t *socket, const char *host, uint16_t port,
                         const void *data, size_t size);

/*
 * out_host/out_port가 NULL이 아니면 발신자 주소를 채운다(out_host는
 * out_host_size 바이트 버퍼, 항상 NUL로 끝난다).
 */
int64_t ten_udp_receive_from(ten_socket_t *socket, void *buffer, size_t size,
                              char *out_host, size_t out_host_size, uint16_t *out_port);

/* ---- 공용 ---- */

/*
 * @return 실제로 보낸/받은 바이트 수. 0은 상대가 정상적으로 연결을
 * 닫았음을 뜻한다(TCP). 실패 시 -1을 반환하고 ten_last_error()에
 * TEN_ERROR_NULL_ARGUMENT 또는 TEN_ERROR_NETWORK가 설정된다.
 */
int64_t ten_socket_send(ten_socket_t *socket, const void *data, size_t size);
int64_t ten_socket_receive(ten_socket_t *socket, void *buffer, size_t size);

/* socket이 NULL이면 아무 일도 하지 않는다. */
void ten_socket_close(ten_socket_t *socket);

/*
 * 내부 native 핸들(POSIX: fd, Windows: SOCKET)을 반환한다(구현자 추가).
 * 자체 select/poll 루프를 만들 때를 위한 탈출구다.
 */
intptr_t ten_socket_native_handle(const ten_socket_t *socket);

/*
 * socket이 timeout_ms 안에 읽기 가능(TCP listen 소켓이면 accept 가능한
 * 연결 대기, 그 외에는 recv 가능한 데이터)한 상태가 되는지 select()로
 * 확인한다(구현자 추가). 이 함수는 accept()를 블로킹한 채로 다른
 * 스레드에서 소켓을 닫아 깨우는 레이스 컨디션 없이 서버 루프를 협조적
 * (cooperative)으로 멈출 수 있게 해준다 — ten_http_server가 이 방식으로
 * stop()을 구현한다.
 *
 * @return 1   읽기 가능
 * @return 0   timeout_ms 동안 아무 일도 없었음
 * @return -1  오류(ten_last_error() 참고), socket이 NULL이면
 *             TEN_ERROR_NULL_ARGUMENT
 */
int ten_socket_poll_readable(const ten_socket_t *socket, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* TEN_SOCKET_H */
