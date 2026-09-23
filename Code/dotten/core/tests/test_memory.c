#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ten/ten.h>

static int g_malloc_calls = 0;
static int g_calloc_calls = 0;
static int g_realloc_calls = 0;
static int g_free_calls = 0;

static void *counting_malloc(size_t size)
{
    g_malloc_calls++;
    return malloc(size);
}

static void *counting_calloc(size_t count, size_t size)
{
    g_calloc_calls++;
    return calloc(count, size);
}

static void *counting_realloc(void *ptr, size_t size)
{
    g_realloc_calls++;
    return realloc(ptr, size);
}

static void counting_free(void *ptr)
{
    g_free_calls++;
    free(ptr);
}

int main(void)
{
    char *buf;
    int *ints;
    char *dup;
    const char *source = "hello, .TEN";
    ten_allocator_t bad_allocator;
    ten_allocator_t counting_allocator;
    void *huge;

    /* 기본 malloc/free */
    buf = (char *)ten_malloc(16);
    assert(buf != NULL);
    assert(ten_last_error() == TEN_OK);
    memcpy(buf, "0123456789abcde", 16);
    ten_free(buf);

    /* calloc은 0으로 초기화되어야 한다 */
    ints = (int *)ten_calloc(4, sizeof(int));
    assert(ints != NULL);
    assert(ints[0] == 0 && ints[1] == 0 && ints[2] == 0 && ints[3] == 0);

    /* realloc은 기존 내용을 보존해야 한다 */
    ints = (int *)ten_realloc(ints, 8 * sizeof(int));
    assert(ints != NULL);
    assert(ints[0] == 0 && ints[3] == 0);
    ten_free(ints);

    /* ten_free(NULL)은 안전해야 한다 */
    ten_free(NULL);

    /* memdup: 정상 케이스 */
    dup = (char *)ten_memdup(source, strlen(source) + 1);
    assert(dup != NULL);
    assert(dup != source);
    assert(strcmp(dup, source) == 0);
    ten_free(dup);

    /* memdup: NULL 인자 케이스 */
    assert(ten_memdup(NULL, 4) == NULL);
    assert(ten_last_error() == TEN_ERROR_NULL_ARGUMENT);

    /* 매우 큰 할당 — 실패해도 크래시 없이 NULL + OUT_OF_MEMORY여야 한다 */
    huge = ten_malloc((size_t)-1);
    assert(huge == NULL);
    assert(ten_last_error() == TEN_ERROR_OUT_OF_MEMORY);

    /* ten_set_allocator: NULL 인자 케이스 */
    assert(ten_set_allocator(NULL) == (int)TEN_ERROR_NULL_ARGUMENT);

    /* ten_set_allocator: 함수 포인터 일부 누락 케이스 */
    bad_allocator.malloc_fn = counting_malloc;
    bad_allocator.calloc_fn = counting_calloc;
    bad_allocator.realloc_fn = counting_realloc;
    bad_allocator.free_fn = NULL;
    assert(ten_set_allocator(&bad_allocator) == (int)TEN_ERROR_NULL_ARGUMENT);

    /* ten_set_allocator: 정상 커스텀 allocator가 실제로 사용되는지 확인 */
    counting_allocator.malloc_fn = counting_malloc;
    counting_allocator.calloc_fn = counting_calloc;
    counting_allocator.realloc_fn = counting_realloc;
    counting_allocator.free_fn = counting_free;
    assert(ten_set_allocator(&counting_allocator) == TEN_OK);

    buf = (char *)ten_malloc(8);
    assert(buf != NULL);
    assert(g_malloc_calls == 1);

    buf = (char *)ten_realloc(buf, 16);
    assert(buf != NULL);
    assert(g_realloc_calls == 1);

    ten_free(buf);
    assert(g_free_calls == 1);

    ints = (int *)ten_calloc(2, sizeof(int));
    assert(ints != NULL);
    assert(g_calloc_calls == 1);
    ten_free(ints);
    assert(g_free_calls == 2);

    return 0;
}
