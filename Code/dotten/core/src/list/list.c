#include "ten/list.h"
#include "ten/memory.h"
#include "internal/error_internal.h"
#include "internal/type_internal.h"

#include <string.h>

#define TEN_LIST_INITIAL_CAPACITY 4

struct ten_list {
    ten_type_t type;
    size_t element_size;
    unsigned char *data;
    size_t count;
    size_t capacity;
};

ten_list_t *ten_list_create(ten_type_t type)
{
    ten_list_t *list;
    size_t element_size = ten__type_size(type);

    if (element_size == 0)
    {
        ten__set_error(TEN_ERROR_INVALID_ARGUMENT);
        return NULL;
    }

    list = (ten_list_t *)ten_malloc(sizeof(ten_list_t));
    if (list == NULL)
    {
        return NULL; /* ten_malloc이 이미 TEN_ERROR_OUT_OF_MEMORY를 설정 */
    }

    list->type = type;
    list->element_size = element_size;
    list->data = NULL;
    list->count = 0;
    list->capacity = 0;

    ten__set_error(TEN_OK);
    return list;
}

void ten_list_destroy(ten_list_t *list)
{
    if (list == NULL)
    {
        return;
    }

    ten_free(list->data);
    ten_free(list);
}

/* capacity를 최소 min_capacity개의 element를 담을 수 있도록 키운다. */
static int ten__list_grow(ten_list_t *list, size_t min_capacity)
{
    size_t new_capacity;
    unsigned char *new_data;

    if (min_capacity <= list->capacity)
    {
        return (int)TEN_OK;
    }

    new_capacity = (list->capacity == 0) ? TEN_LIST_INITIAL_CAPACITY : list->capacity * 2;
    if (new_capacity < min_capacity)
    {
        new_capacity = min_capacity;
    }

    new_data = (unsigned char *)ten_realloc(list->data, new_capacity * list->element_size);
    if (new_data == NULL)
    {
        return (int)TEN_ERROR_OUT_OF_MEMORY;
    }

    list->data = new_data;
    list->capacity = new_capacity;
    return (int)TEN_OK;
}

int ten_list_add(ten_list_t *list, const void *value)
{
    return ten_list_insert(list, list != NULL ? list->count : 0, value);
}

int ten_list_insert(ten_list_t *list, size_t index, const void *value)
{
    int rc;
    unsigned char *slot;

    if (list == NULL || value == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (index > list->count)
    {
        ten__set_error(TEN_ERROR_OUT_OF_RANGE);
        return (int)TEN_ERROR_OUT_OF_RANGE;
    }

    rc = ten__list_grow(list, list->count + 1);
    if (rc != (int)TEN_OK)
    {
        ten__set_error((ten_error_code_t)rc);
        return rc;
    }

    slot = list->data + index * list->element_size;

    if (index < list->count)
    {
        memmove(slot + list->element_size, slot, (list->count - index) * list->element_size);
    }

    memcpy(slot, value, list->element_size);
    list->count++;

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

int ten_list_remove_at(ten_list_t *list, size_t index)
{
    unsigned char *slot;

    if (list == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (index >= list->count)
    {
        ten__set_error(TEN_ERROR_OUT_OF_RANGE);
        return (int)TEN_ERROR_OUT_OF_RANGE;
    }

    slot = list->data + index * list->element_size;

    if (index + 1 < list->count)
    {
        memmove(slot, slot + list->element_size, (list->count - index - 1) * list->element_size);
    }

    list->count--;
    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

void *ten_list_get(ten_list_t *list, size_t index)
{
    if (list == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    if (index >= list->count)
    {
        ten__set_error(TEN_ERROR_OUT_OF_RANGE);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return list->data + index * list->element_size;
}

size_t ten_list_count(const ten_list_t *list)
{
    if (list == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return 0;
    }

    ten__set_error(TEN_OK);
    return list->count;
}

size_t ten_list_capacity(const ten_list_t *list)
{
    if (list == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return 0;
    }

    ten__set_error(TEN_OK);
    return list->capacity;
}

int ten_list_reserve(ten_list_t *list, size_t capacity)
{
    int rc;

    if (list == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    rc = ten__list_grow(list, capacity);
    ten__set_error((ten_error_code_t)rc);
    return rc;
}
