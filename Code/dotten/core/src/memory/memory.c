#include "ten/memory.h"
#include "internal/error_internal.h"

#include <stdlib.h>
#include <string.h>

static void *ten_default_malloc(size_t size)
{
    return malloc(size);
}

static void *ten_default_calloc(size_t count, size_t size)
{
    return calloc(count, size);
}

static void *ten_default_realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

static void ten_default_free(void *ptr)
{
    free(ptr);
}

/*
 * 현재 사용 중인 allocator. ten_set_allocator() 호출 전에는 기본
 * allocator(libc)를 사용한다.
 */
static ten_allocator_t g_ten_allocator = {
    ten_default_malloc,
    ten_default_calloc,
    ten_default_realloc,
    ten_default_free
};

int ten_set_allocator(const ten_allocator_t *allocator)
{
    if (allocator == NULL ||
        allocator->malloc_fn == NULL ||
        allocator->calloc_fn == NULL ||
        allocator->realloc_fn == NULL ||
        allocator->free_fn == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    g_ten_allocator = *allocator;
    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

void *ten_malloc(size_t size)
{
    void *ptr = g_ten_allocator.malloc_fn(size);
    ten__set_error(ptr != NULL ? TEN_OK : TEN_ERROR_OUT_OF_MEMORY);
    return ptr;
}

void *ten_calloc(size_t count, size_t size)
{
    void *ptr = g_ten_allocator.calloc_fn(count, size);
    ten__set_error(ptr != NULL ? TEN_OK : TEN_ERROR_OUT_OF_MEMORY);
    return ptr;
}

void *ten_realloc(void *ptr, size_t size)
{
    void *result = g_ten_allocator.realloc_fn(ptr, size);

    /* size == 0 인 경우 구현체에 따라 NULL을 반환할 수 있으므로 그 경우는
       실패로 취급하지 않는다. */
    if (result == NULL && size != 0)
    {
        ten__set_error(TEN_ERROR_OUT_OF_MEMORY);
    }
    else
    {
        ten__set_error(TEN_OK);
    }

    return result;
}

void ten_free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    g_ten_allocator.free_fn(ptr);
}

void *ten_memdup(const void *data, size_t size)
{
    void *copy;

    if (data == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    copy = ten_malloc(size);
    if (copy == NULL)
    {
        /* ten_malloc이 이미 TEN_ERROR_OUT_OF_MEMORY를 설정했다. */
        return NULL;
    }

    memcpy(copy, data, size);
    ten__set_error(TEN_OK);
    return copy;
}
