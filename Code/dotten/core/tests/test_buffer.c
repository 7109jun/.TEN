#include <assert.h>
#include <string.h>
#include <ten/ten.h>

int main(void)
{
    ten_buffer_t *buffer = ten_buffer_create();
    unsigned char large[1000];
    size_t i;

    assert(buffer != NULL);
    assert(ten_buffer_size(buffer) == 0);

    assert(ten_buffer_write(buffer, "hello", 5) == TEN_OK);
    assert(ten_buffer_size(buffer) == 5);
    assert(memcmp(ten_buffer_data(buffer), "hello", 5) == 0);

    assert(ten_buffer_write(buffer, " world", 6) == TEN_OK);
    assert(ten_buffer_size(buffer) == 11);
    assert(memcmp(ten_buffer_data(buffer), "hello world", 11) == 0);

    /* size == 0인 write는 data가 NULL이어도 안전해야 한다 */
    assert(ten_buffer_write(buffer, NULL, 0) == TEN_OK);
    assert(ten_buffer_size(buffer) == 11);

    /* size > 0인데 data가 NULL인 경우 */
    assert(ten_buffer_write(buffer, NULL, 4) == (int)TEN_ERROR_NULL_ARGUMENT);

    /* NULL buffer */
    assert(ten_buffer_write(NULL, "x", 1) == (int)TEN_ERROR_NULL_ARGUMENT);
    assert(ten_buffer_size(NULL) == 0);
    assert(ten_last_error() == TEN_ERROR_NULL_ARGUMENT);

    /* clear 후 재사용 */
    ten_buffer_clear(buffer);
    assert(ten_buffer_size(buffer) == 0);
    assert(ten_buffer_write(buffer, "again", 5) == TEN_OK);
    assert(ten_buffer_size(buffer) == 5);
    assert(memcmp(ten_buffer_data(buffer), "again", 5) == 0);

    /* 초기 capacity(16바이트)를 넘는 큰 쓰기로 growth 경로 검증 */
    for (i = 0; i < sizeof(large); i++)
    {
        large[i] = (unsigned char)(i & 0xFF);
    }
    ten_buffer_clear(buffer);
    assert(ten_buffer_write(buffer, large, sizeof(large)) == TEN_OK);
    assert(ten_buffer_size(buffer) == sizeof(large));
    assert(memcmp(ten_buffer_data(buffer), large, sizeof(large)) == 0);

    ten_buffer_destroy(buffer);

    /* destroy(NULL)은 안전해야 한다 */
    ten_buffer_destroy(NULL);

    return 0;
}
