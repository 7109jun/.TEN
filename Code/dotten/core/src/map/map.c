#include "ten/map.h"
#include "ten/memory.h"
#include "internal/error_internal.h"
#include "internal/type_internal.h"

#include <string.h>

#define TEN_MAP_INITIAL_BUCKETS 16

typedef struct ten_map_entry {
    struct ten_map_entry *next;
    unsigned long hash;
    unsigned char *key;   /* key_size 바이트, ten_malloc으로 소유하는 복사본 */
    unsigned char *value; /* value_size 바이트, ten_malloc으로 소유하는 복사본 */
} ten_map_entry_t;

struct ten_map {
    ten_type_t key_type;
    ten_type_t value_type;
    size_t key_size;
    size_t value_size;
    ten_map_entry_t **buckets;
    size_t bucket_count;
    size_t count;
};

static unsigned long ten__fnv1a(const unsigned char *data, size_t len)
{
    unsigned long hash = 2166136261UL;
    size_t i;

    for (i = 0; i < len; i++)
    {
        hash ^= data[i];
        hash *= 16777619UL;
    }

    return hash;
}

static unsigned long ten__map_hash(const ten_map_t *map, const void *key)
{
    if (map->key_type == TEN_TYPE_STRING)
    {
        const char *s;
        memcpy(&s, key, sizeof(s));
        return ten__fnv1a((const unsigned char *)s, strlen(s));
    }

    return ten__fnv1a((const unsigned char *)key, map->key_size);
}

static int ten__map_key_equals(const ten_map_t *map, const void *stored_key, const void *query_key)
{
    if (map->key_type == TEN_TYPE_STRING)
    {
        const char *a;
        const char *b;
        memcpy(&a, stored_key, sizeof(a));
        memcpy(&b, query_key, sizeof(b));
        return strcmp(a, b) == 0;
    }

    return memcmp(stored_key, query_key, map->key_size) == 0;
}

static ten_map_entry_t *ten__map_find(const ten_map_t *map, const void *key)
{
    unsigned long hash = ten__map_hash(map, key);
    size_t bucket = (size_t)(hash % map->bucket_count);
    ten_map_entry_t *entry = map->buckets[bucket];

    while (entry != NULL)
    {
        if (entry->hash == hash && ten__map_key_equals(map, entry->key, key))
        {
            return entry;
        }
        entry = entry->next;
    }

    return NULL;
}

static int ten__map_rehash(ten_map_t *map, size_t new_bucket_count)
{
    ten_map_entry_t **new_buckets;
    size_t i;

    new_buckets = (ten_map_entry_t **)ten_calloc(new_bucket_count, sizeof(ten_map_entry_t *));
    if (new_buckets == NULL)
    {
        return (int)TEN_ERROR_OUT_OF_MEMORY;
    }

    for (i = 0; i < map->bucket_count; i++)
    {
        ten_map_entry_t *entry = map->buckets[i];
        while (entry != NULL)
        {
            ten_map_entry_t *next = entry->next;
            size_t new_bucket = (size_t)(entry->hash % new_bucket_count);
            entry->next = new_buckets[new_bucket];
            new_buckets[new_bucket] = entry;
            entry = next;
        }
    }

    ten_free(map->buckets);
    map->buckets = new_buckets;
    map->bucket_count = new_bucket_count;
    return (int)TEN_OK;
}

ten_map_t *ten_map_create(ten_type_t key_type, ten_type_t value_type)
{
    ten_map_t *map;
    size_t key_size = ten__type_size(key_type);
    size_t value_size = ten__type_size(value_type);

    if (key_size == 0 || value_size == 0)
    {
        ten__set_error(TEN_ERROR_INVALID_ARGUMENT);
        return NULL;
    }

    map = (ten_map_t *)ten_malloc(sizeof(ten_map_t));
    if (map == NULL)
    {
        return NULL;
    }

    map->buckets = (ten_map_entry_t **)ten_calloc(TEN_MAP_INITIAL_BUCKETS, sizeof(ten_map_entry_t *));
    if (map->buckets == NULL)
    {
        ten_free(map);
        return NULL;
    }

    map->key_type = key_type;
    map->value_type = value_type;
    map->key_size = key_size;
    map->value_size = value_size;
    map->bucket_count = TEN_MAP_INITIAL_BUCKETS;
    map->count = 0;

    ten__set_error(TEN_OK);
    return map;
}

void ten_map_destroy(ten_map_t *map)
{
    size_t i;

    if (map == NULL)
    {
        return;
    }

    for (i = 0; i < map->bucket_count; i++)
    {
        ten_map_entry_t *entry = map->buckets[i];
        while (entry != NULL)
        {
            ten_map_entry_t *next = entry->next;
            ten_free(entry->key);
            ten_free(entry->value);
            ten_free(entry);
            entry = next;
        }
    }

    ten_free(map->buckets);
    ten_free(map);
}

int ten_map_set(ten_map_t *map, const void *key, const void *value)
{
    ten_map_entry_t *entry;
    unsigned long hash;
    size_t bucket;

    if (map == NULL || key == NULL || value == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    entry = ten__map_find(map, key);
    if (entry != NULL)
    {
        memcpy(entry->value, value, map->value_size);
        ten__set_error(TEN_OK);
        return (int)TEN_OK;
    }

    if (map->count + 1 > (map->bucket_count * 3) / 4)
    {
        int rc = ten__map_rehash(map, map->bucket_count * 2);
        if (rc != (int)TEN_OK)
        {
            ten__set_error((ten_error_code_t)rc);
            return rc;
        }
    }

    entry = (ten_map_entry_t *)ten_malloc(sizeof(ten_map_entry_t));
    if (entry == NULL)
    {
        return (int)TEN_ERROR_OUT_OF_MEMORY;
    }

    entry->key = (unsigned char *)ten_malloc(map->key_size);
    if (entry->key == NULL)
    {
        ten_free(entry);
        return (int)TEN_ERROR_OUT_OF_MEMORY;
    }
    memcpy(entry->key, key, map->key_size);

    entry->value = (unsigned char *)ten_malloc(map->value_size);
    if (entry->value == NULL)
    {
        ten_free(entry->key);
        ten_free(entry);
        return (int)TEN_ERROR_OUT_OF_MEMORY;
    }
    memcpy(entry->value, value, map->value_size);

    hash = ten__map_hash(map, key);
    bucket = (size_t)(hash % map->bucket_count);
    entry->hash = hash;
    entry->next = map->buckets[bucket];
    map->buckets[bucket] = entry;
    map->count++;

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

void *ten_map_get(ten_map_t *map, const void *key)
{
    ten_map_entry_t *entry;

    if (map == NULL || key == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    entry = ten__map_find(map, key);
    if (entry == NULL)
    {
        ten__set_error(TEN_ERROR_NOT_FOUND);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return entry->value;
}

int ten_map_remove(ten_map_t *map, const void *key)
{
    unsigned long hash;
    size_t bucket;
    ten_map_entry_t *entry;
    ten_map_entry_t *prev;

    if (map == NULL || key == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    hash = ten__map_hash(map, key);
    bucket = (size_t)(hash % map->bucket_count);
    entry = map->buckets[bucket];
    prev = NULL;

    while (entry != NULL)
    {
        if (entry->hash == hash && ten__map_key_equals(map, entry->key, key))
        {
            if (prev == NULL)
            {
                map->buckets[bucket] = entry->next;
            }
            else
            {
                prev->next = entry->next;
            }

            ten_free(entry->key);
            ten_free(entry->value);
            ten_free(entry);
            map->count--;

            ten__set_error(TEN_OK);
            return (int)TEN_OK;
        }

        prev = entry;
        entry = entry->next;
    }

    ten__set_error(TEN_ERROR_NOT_FOUND);
    return (int)TEN_ERROR_NOT_FOUND;
}

int ten_map_contains(const ten_map_t *map, const void *key)
{
    if (map == NULL || key == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return 0;
    }

    ten__set_error(TEN_OK);
    return ten__map_find(map, key) != NULL;
}

size_t ten_map_count(const ten_map_t *map)
{
    if (map == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return 0;
    }

    ten__set_error(TEN_OK);
    return map->count;
}
