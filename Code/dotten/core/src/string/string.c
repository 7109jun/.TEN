#include "ten/string.h"
#include "ten/memory.h"
#include "internal/error_internal.h"

#include <string.h>

#define TEN_STRING_INITIAL_CAPACITY 16

struct ten_string {
    char *data;      /* 항상 NUL로 끝난다 */
    size_t length;   /* NUL 제외 길이 */
    size_t capacity; /* NUL을 포함한 data 버퍼 크기 */
};

/* length 바이트(+NUL) 이상을 담을 수 있는 빈 문자열 구조체를 만든다. */
static ten_string_t *ten__string_alloc(size_t min_capacity)
{
    ten_string_t *string = (ten_string_t *)ten_malloc(sizeof(ten_string_t));
    size_t capacity = TEN_STRING_INITIAL_CAPACITY;

    if (string == NULL)
    {
        return NULL;
    }

    if (capacity < min_capacity)
    {
        capacity = min_capacity;
    }

    string->data = (char *)ten_malloc(capacity);
    if (string->data == NULL)
    {
        ten_free(string);
        return NULL;
    }

    string->data[0] = '\0';
    string->length = 0;
    string->capacity = capacity;

    return string;
}

ten_string_t *ten_string_create(const char *text)
{
    ten_string_t *string;
    size_t len;

    if (text == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    len = strlen(text);
    string = ten__string_alloc(len + 1);
    if (string == NULL)
    {
        return NULL;
    }

    memcpy(string->data, text, len + 1);
    string->length = len;

    ten__set_error(TEN_OK);
    return string;
}

ten_string_t *ten_string_empty(void)
{
    ten_string_t *string = ten__string_alloc(1);
    if (string == NULL)
    {
        return NULL;
    }

    ten__set_error(TEN_OK);
    return string;
}

void ten_string_destroy(ten_string_t *string)
{
    if (string == NULL)
    {
        return;
    }

    ten_free(string->data);
    ten_free(string);
}

const char *ten_string_cstr(const ten_string_t *string)
{
    if (string == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return string->data;
}

size_t ten_string_length(const ten_string_t *string)
{
    if (string == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return 0;
    }

    ten__set_error(TEN_OK);
    return string->length;
}

int ten_string_append(ten_string_t *string, const char *text)
{
    size_t text_len;
    size_t new_len;

    if (string == NULL || text == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    text_len = strlen(text);
    new_len = string->length + text_len;

    if (new_len + 1 > string->capacity)
    {
        size_t new_capacity = (string->capacity == 0) ? TEN_STRING_INITIAL_CAPACITY : string->capacity * 2;
        char *new_data;

        while (new_capacity < new_len + 1)
        {
            new_capacity *= 2;
        }

        new_data = (char *)ten_realloc(string->data, new_capacity);
        if (new_data == NULL)
        {
            return (int)TEN_ERROR_OUT_OF_MEMORY;
        }

        string->data = new_data;
        string->capacity = new_capacity;
    }

    memcpy(string->data + string->length, text, text_len + 1);
    string->length = new_len;

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

int ten_string_equals(const ten_string_t *a, const ten_string_t *b)
{
    if (a == NULL || b == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return 0;
    }

    ten__set_error(TEN_OK);

    if (a->length != b->length)
    {
        return 0;
    }

    return memcmp(a->data, b->data, a->length) == 0;
}

int ten_string_contains(const ten_string_t *string, const char *text)
{
    if (string == NULL || text == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return 0;
    }

    ten__set_error(TEN_OK);
    return strstr(string->data, text) != NULL;
}

ten_string_t *ten_string_substring(const ten_string_t *string, size_t start, size_t length)
{
    ten_string_t *result;

    if (string == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    if (start > string->length || length > string->length - start)
    {
        ten__set_error(TEN_ERROR_OUT_OF_RANGE);
        return NULL;
    }

    result = ten__string_alloc(length + 1);
    if (result == NULL)
    {
        return NULL;
    }

    memcpy(result->data, string->data + start, length);
    result->data[length] = '\0';
    result->length = length;

    ten__set_error(TEN_OK);
    return result;
}
