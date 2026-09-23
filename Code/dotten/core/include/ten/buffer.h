#ifndef TEN_BUFFER_H
#define TEN_BUFFER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ten_buffer — 바이너리 데이터를 위한 동적 버퍼.
 *
 * Thread Safety: NOT_THREAD_SAFE
 */
typedef struct ten_buffer ten_buffer_t;

/*
 * Ownership: 호출자가 소유하며 ten_buffer_destroy()로 해제한다.
 * 실패 시(TEN_ERROR_OUT_OF_MEMORY) NULL을 반환한다.
 */
ten_buffer_t *ten_buffer_create(void);

/* buffer가 NULL이면 아무 일도 하지 않는다. */
void ten_buffer_destroy(ten_buffer_t *buffer);

/*
 * data의 size 바이트를 buffer 끝에 덧붙인다. size == 0이면 아무 일도
 *하지 않고 TEN_OK를 반환한다(이 경우 data는 NULL이어도 된다).
 *
 * @return TEN_OK
 * @return TEN_ERROR_NULL_ARGUMENT  buffer가 NULL이거나, size > 0인데
 *                                  data가 NULL인 경우
 * @return TEN_ERROR_OUT_OF_MEMORY  확장 실패
 */
int ten_buffer_write(ten_buffer_t *buffer, const void *data, size_t size);

/*
 * Ownership: buffer 내부를 가리키는 참조이며 호출자가 해제하지 않는다.
 * buffer가 write/clear로 변경되면 무효화될 수 있다. 비어 있으면 NULL일
 * 수 있다. buffer가 NULL이면 NULL을 반환하고 TEN_ERROR_NULL_ARGUMENT를
 * 설정한다.
 */
void *ten_buffer_data(ten_buffer_t *buffer);

/* buffer가 NULL이면 0을 반환하고 TEN_ERROR_NULL_ARGUMENT를 설정한다. */
size_t ten_buffer_size(const ten_buffer_t *buffer);

/*
 * size를 0으로 되돌린다. 내부에 이미 확보된 capacity는 재사용을 위해
 * 유지한다(해제하지 않는다). buffer가 NULL이면 아무 일도 하지 않는다.
 */
void ten_buffer_clear(ten_buffer_t *buffer);

#ifdef __cplusplus
}
#endif

#endif /* TEN_BUFFER_H */
