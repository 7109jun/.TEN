#include "ten/buffer.h"
#include "ten/memory.h"
#include "internal/error_internal.h"

#include <string.h>

#define TEN_BUFFER_INITIAL_CAPACITY 16

struct ten_buffer {
    unsigned char *data;
    size_t size;
    size_t capacity;
};

ten_buffer_t *ten_buffer_create(void)
{
    ten_buffer_t *buffer = (ten_buffer_t *)ten_malloc(sizeof(ten_buffer_t));
    if (buffer == NULL)
    {
        return NULL;
    }

    buffer->data = NULL;
    buffer->size = 0;
    buffer->capacity = 0;

    ten__set_error(TEN_OK);
    return buffer;
}

void ten_buffer_destroy(ten_buffer_t *buffer)
{
    if (buffer == NULL)
    {
        return;
    }

    ten_free(buffer->data);
    ten_free(buffer);
}

int ten_buffer_write(ten_buffer_t *buffer, const void *data, size_t size)
{
    size_t new_size;

    if (buffer == NULL || (data == NULL && size > 0))
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (size == 0)
    {
        ten__set_error(TEN_OK);
        return (int)TEN_OK;
    }

    new_size = buffer->size + size;

    if (new_size > buffer->capacity)
    {
        size_t new_capacity = (buffer->capacity == 0) ? TEN_BUFFER_INITIAL_CAPACITY : buffer->capacity * 2;
        unsigned char *new_data;

        while (new_capacity < new_size)
        {
            new_capacity *= 2;
        }

        new_data = (unsigned char *)ten_realloc(buffer->data, new_capacity);
        if (new_data == NULL)
        {
            return (int)TEN_ERROR_OUT_OF_MEMORY;
        }

        buffer->data = new_data;
        buffer->capacity = new_capacity;
    }

    memcpy(buffer->data + buffer->size, data, size);
    buffer->size = new_size;

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

void *ten_buffer_data(ten_buffer_t *buffer)
{
    if (buffer == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return buffer->data;
}

size_t ten_buffer_size(const ten_buffer_t *buffer)
{
    if (buffer == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return 0;
    }

    ten__set_error(TEN_OK);
    return buffer->size;
}

void ten_buffer_clear(ten_buffer_t *buffer)
{
    if (buffer == NULL)
    {
        return;
    }

    buffer->size = 0;
}
