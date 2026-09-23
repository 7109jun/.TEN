#include "ten/json.h"
#include "ten/list.h"
#include "ten/map.h"
#include "ten/string.h"
#include "ten/memory.h"
#include "internal/error_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ten_json {
    ten_json_type_t type;
    union {
        int boolean;
        double number;
        ten_string_t *string; /* TEN_JSON_STRING */
        ten_list_t *array;    /* TEN_JSON_ARRAY: TEN_TYPE_POINTER, 원소는 ten_json_t*(소유) */
        struct {
            /* TEN_TYPE_STRING(char*) -> TEN_TYPE_POINTER(ten_json_t*, 소유) */
            ten_map_t *entries;
            /* TEN_TYPE_POINTER, 각 원소는 ten_string_t*(entries의 key와 동일 객체,
               소유는 여기 key_order 쪽에서 한다) — 삽입 순서 보존용 */
            ten_list_t *key_order;
        } object;
    } value;
};

/* ================= 생성/해제 ================= */

static ten_json_t *ten__json_alloc(ten_json_type_t type)
{
    ten_json_t *json = (ten_json_t *)ten_malloc(sizeof(ten_json_t));
    if (json == NULL)
    {
        return NULL;
    }
    json->type = type;
    return json;
}

ten_json_t *ten_json_create_null(void)
{
    ten_json_t *json = ten__json_alloc(TEN_JSON_NULL);
    if (json != NULL)
    {
        ten__set_error(TEN_OK);
    }
    return json;
}

ten_json_t *ten_json_create_bool(int value)
{
    ten_json_t *json = ten__json_alloc(TEN_JSON_BOOL);
    if (json == NULL)
    {
        return NULL;
    }
    json->value.boolean = value ? 1 : 0;
    ten__set_error(TEN_OK);
    return json;
}

ten_json_t *ten_json_create_number(double value)
{
    ten_json_t *json = ten__json_alloc(TEN_JSON_NUMBER);
    if (json == NULL)
    {
        return NULL;
    }
    json->value.number = value;
    ten__set_error(TEN_OK);
    return json;
}

ten_json_t *ten_json_create_string(const char *value)
{
    ten_json_t *json;
    ten_string_t *s;

    if (value == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    s = ten_string_create(value);
    if (s == NULL)
    {
        return NULL;
    }

    json = ten__json_alloc(TEN_JSON_STRING);
    if (json == NULL)
    {
        ten_string_destroy(s);
        return NULL;
    }

    json->value.string = s;
    ten__set_error(TEN_OK);
    return json;
}

ten_json_t *ten_json_create_array(void)
{
    ten_json_t *json = ten__json_alloc(TEN_JSON_ARRAY);
    if (json == NULL)
    {
        return NULL;
    }

    json->value.array = ten_list_create(TEN_TYPE_POINTER);
    if (json->value.array == NULL)
    {
        ten_free(json);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return json;
}

ten_json_t *ten_json_create_object(void)
{
    ten_json_t *json = ten__json_alloc(TEN_JSON_OBJECT);
    if (json == NULL)
    {
        return NULL;
    }

    json->value.object.entries = ten_map_create(TEN_TYPE_STRING, TEN_TYPE_POINTER);
    json->value.object.key_order = ten_list_create(TEN_TYPE_POINTER);

    if (json->value.object.entries == NULL || json->value.object.key_order == NULL)
    {
        ten_map_destroy(json->value.object.entries);
        ten_list_destroy(json->value.object.key_order);
        ten_free(json);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return json;
}

void ten_json_destroy(ten_json_t *json)
{
    if (json == NULL)
    {
        return;
    }

    switch (json->type)
    {
        case TEN_JSON_STRING:
            ten_string_destroy(json->value.string);
            break;

        case TEN_JSON_ARRAY:
        {
            size_t i;
            size_t count = ten_list_count(json->value.array);
            for (i = 0; i < count; i++)
            {
                ten_json_t *element = *(ten_json_t **)ten_list_get(json->value.array, i);
                ten_json_destroy(element);
            }
            ten_list_destroy(json->value.array);
            break;
        }

        case TEN_JSON_OBJECT:
        {
            size_t i;
            size_t count = ten_list_count(json->value.object.key_order);
            for (i = 0; i < count; i++)
            {
                ten_string_t *key = *(ten_string_t **)ten_list_get(json->value.object.key_order, i);
                const char *key_cstr = ten_string_cstr(key);
                void *slot = ten_map_get(json->value.object.entries, &key_cstr);
                if (slot != NULL)
                {
                    ten_json_destroy(*(ten_json_t **)slot);
                }
                ten_string_destroy(key);
            }
            ten_map_destroy(json->value.object.entries);
            ten_list_destroy(json->value.object.key_order);
            break;
        }

        default:
            break; /* NULL/BOOL/NUMBER는 별도 정리가 필요 없다 */
    }

    ten_free(json);
}

/* ================= 조회 ================= */

ten_json_type_t ten_json_type(const ten_json_t *json)
{
    if (json == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return TEN_JSON_NULL;
    }
    ten__set_error(TEN_OK);
    return json->type;
}

int ten_json_get_bool(const ten_json_t *json)
{
    if (json == NULL || json->type != TEN_JSON_BOOL)
    {
        ten__set_error(TEN_ERROR_INVALID_STATE);
        return 0;
    }
    ten__set_error(TEN_OK);
    return json->value.boolean;
}

double ten_json_get_number(const ten_json_t *json)
{
    if (json == NULL || json->type != TEN_JSON_NUMBER)
    {
        ten__set_error(TEN_ERROR_INVALID_STATE);
        return 0.0;
    }
    ten__set_error(TEN_OK);
    return json->value.number;
}

const char *ten_json_get_string(const ten_json_t *json)
{
    if (json == NULL || json->type != TEN_JSON_STRING)
    {
        ten__set_error(TEN_ERROR_INVALID_STATE);
        return NULL;
    }
    ten__set_error(TEN_OK);
    return ten_string_cstr(json->value.string);
}

size_t ten_json_array_count(const ten_json_t *json)
{
    if (json == NULL || json->type != TEN_JSON_ARRAY)
    {
        ten__set_error(TEN_ERROR_INVALID_STATE);
        return 0;
    }
    ten__set_error(TEN_OK);
    return ten_list_count(json->value.array);
}

ten_json_t *ten_json_array_get(const ten_json_t *json, size_t index)
{
    void *slot;

    if (json == NULL || json->type != TEN_JSON_ARRAY)
    {
        ten__set_error(TEN_ERROR_INVALID_STATE);
        return NULL;
    }

    slot = ten_list_get(json->value.array, index);
    if (slot == NULL)
    {
        return NULL; /* ten_list_get이 이미 OUT_OF_RANGE를 설정했다 */
    }

    ten__set_error(TEN_OK);
    return *(ten_json_t **)slot;
}

int ten_json_array_add(ten_json_t *json, ten_json_t *value)
{
    if (json == NULL || value == NULL || json->type != TEN_JSON_ARRAY)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    return ten_list_add(json->value.array, &value);
}

size_t ten_json_object_count(const ten_json_t *json)
{
    if (json == NULL || json->type != TEN_JSON_OBJECT)
    {
        ten__set_error(TEN_ERROR_INVALID_STATE);
        return 0;
    }
    ten__set_error(TEN_OK);
    return ten_map_count(json->value.object.entries);
}

ten_json_t *ten_json_object_get(const ten_json_t *json, const char *key)
{
    void *slot;

    if (json == NULL || key == NULL || json->type != TEN_JSON_OBJECT)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    slot = ten_map_get(json->value.object.entries, &key);
    if (slot == NULL)
    {
        return NULL; /* ten_map_get이 이미 NOT_FOUND를 설정했다 */
    }

    ten__set_error(TEN_OK);
    return *(ten_json_t **)slot;
}

/* key의 소유권을 항상 가져간다(성공/실패 무관 — 폐기하거나 저장한다).
   value의 소유권은 성공했을 때만 가져간다. */
static int ten__json_object_set_owned(ten_json_t *json, ten_string_t *key, ten_json_t *value)
{
    const char *key_cstr = ten_string_cstr(key);
    void *slot = ten_map_get(json->value.object.entries, &key_cstr);

    if (slot != NULL)
    {
        ten_json_t *old_value = *(ten_json_t **)slot;
        ten_json_t *new_value_ptr = value;

        if (ten_map_set(json->value.object.entries, &key_cstr, &new_value_ptr) != (int)TEN_OK)
        {
            ten_string_destroy(key);
            return (int)TEN_ERROR_OUT_OF_MEMORY;
        }

        ten_json_destroy(old_value);
        ten_string_destroy(key); /* 기존 key_order의 문자열을 그대로 쓰므로 새로 만든 건 폐기 */
        ten__set_error(TEN_OK);
        return (int)TEN_OK;
    }
    else
    {
        ten_json_t *value_ptr = value;

        if (ten_map_set(json->value.object.entries, &key_cstr, &value_ptr) != (int)TEN_OK)
        {
            ten_string_destroy(key);
            return (int)TEN_ERROR_OUT_OF_MEMORY;
        }

        if (ten_list_add(json->value.object.key_order, &key) != (int)TEN_OK)
        {
            ten_map_remove(json->value.object.entries, &key_cstr);
            ten_string_destroy(key);
            return (int)TEN_ERROR_OUT_OF_MEMORY;
        }

        ten__set_error(TEN_OK);
        return (int)TEN_OK;
    }
}

int ten_json_object_set(ten_json_t *json, const char *key, ten_json_t *value)
{
    ten_string_t *key_copy;

    if (json == NULL || key == NULL || value == NULL || json->type != TEN_JSON_OBJECT)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    key_copy = ten_string_create(key);
    if (key_copy == NULL)
    {
        return (int)TEN_ERROR_OUT_OF_MEMORY;
    }

    return ten__json_object_set_owned(json, key_copy, value);
}

const char *ten_json_object_key_at(const ten_json_t *json, size_t index)
{
    void *slot;

    if (json == NULL || json->type != TEN_JSON_OBJECT)
    {
        ten__set_error(TEN_ERROR_INVALID_STATE);
        return NULL;
    }

    slot = ten_list_get(json->value.object.key_order, index);
    if (slot == NULL)
    {
        return NULL;
    }

    ten__set_error(TEN_OK);
    return ten_string_cstr(*(ten_string_t **)slot);
}

/* ================= 파서 ================= */

typedef struct {
    const char *cursor;
    const char *end;
} ten__json_parser_t;

static void ten__json_skip_ws(ten__json_parser_t *p)
{
    while (p->cursor < p->end &&
           (*p->cursor == ' ' || *p->cursor == '\t' || *p->cursor == '\n' || *p->cursor == '\r'))
    {
        p->cursor++;
    }
}

static int ten__json_hex_digit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int ten__json_parse_hex4(const char *s, unsigned int *out)
{
    int d0 = ten__json_hex_digit(s[0]);
    int d1 = ten__json_hex_digit(s[1]);
    int d2 = ten__json_hex_digit(s[2]);
    int d3 = ten__json_hex_digit(s[3]);

    if (d0 < 0 || d1 < 0 || d2 < 0 || d3 < 0)
    {
        return 0;
    }

    *out = (unsigned int)((d0 << 12) | (d1 << 8) | (d2 << 4) | d3);
    return 1;
}

static void ten__json_append_codepoint(ten_string_t *out, unsigned int cp)
{
    char buf[5];
    int n = 0;

    if (cp < 0x80)
    {
        buf[n++] = (char)cp;
    }
    else if (cp < 0x800)
    {
        buf[n++] = (char)(0xC0 | (cp >> 6));
        buf[n++] = (char)(0x80 | (cp & 0x3F));
    }
    else if (cp < 0x10000)
    {
        buf[n++] = (char)(0xE0 | (cp >> 12));
        buf[n++] = (char)(0x80 | ((cp >> 6) & 0x3F));
        buf[n++] = (char)(0x80 | (cp & 0x3F));
    }
    else
    {
        buf[n++] = (char)(0xF0 | (cp >> 18));
        buf[n++] = (char)(0x80 | ((cp >> 12) & 0x3F));
        buf[n++] = (char)(0x80 | ((cp >> 6) & 0x3F));
        buf[n++] = (char)(0x80 | (cp & 0x3F));
    }
    buf[n] = '\0';

    ten_string_append(out, buf);
}

static ten_string_t *ten__json_parse_string(ten__json_parser_t *p)
{
    ten_string_t *out;

    if (p->cursor >= p->end || *p->cursor != '"')
    {
        return NULL;
    }
    p->cursor++;

    out = ten_string_empty();
    if (out == NULL)
    {
        return NULL;
    }

    while (p->cursor < p->end && *p->cursor != '"')
    {
        unsigned char c = (unsigned char)*p->cursor;

        if (c != '\\')
        {
            char buf[2];
            buf[0] = (char)c;
            buf[1] = '\0';
            ten_string_append(out, buf);
            p->cursor++;
            continue;
        }

        p->cursor++;
        if (p->cursor >= p->end)
        {
            ten_string_destroy(out);
            return NULL;
        }

        switch (*p->cursor)
        {
            case '"':  ten_string_append(out, "\""); p->cursor++; break;
            case '\\': ten_string_append(out, "\\"); p->cursor++; break;
            case '/':  ten_string_append(out, "/");  p->cursor++; break;
            case 'b':  ten_string_append(out, "\b"); p->cursor++; break;
            case 'f':  ten_string_append(out, "\f"); p->cursor++; break;
            case 'n':  ten_string_append(out, "\n"); p->cursor++; break;
            case 'r':  ten_string_append(out, "\r"); p->cursor++; break;
            case 't':  ten_string_append(out, "\t"); p->cursor++; break;

            case 'u':
            {
                unsigned int cp;

                p->cursor++;
                if (p->end - p->cursor < 4 || !ten__json_parse_hex4(p->cursor, &cp))
                {
                    ten_string_destroy(out);
                    return NULL;
                }
                p->cursor += 4;

                if (cp >= 0xD800 && cp <= 0xDBFF)
                {
                    unsigned int low;

                    if (p->end - p->cursor < 6 || p->cursor[0] != '\\' || p->cursor[1] != 'u' ||
                        !ten__json_parse_hex4(p->cursor + 2, &low) || low < 0xDC00 || low > 0xDFFF)
                    {
                        ten_string_destroy(out);
                        return NULL;
                    }
                    p->cursor += 6;
                    cp = 0x10000u + ((cp - 0xD800u) << 10) + (low - 0xDC00u);
                }

                ten__json_append_codepoint(out, cp);
                break;
            }

            default:
                ten_string_destroy(out);
                return NULL;
        }
    }

    if (p->cursor >= p->end || *p->cursor != '"')
    {
        ten_string_destroy(out);
        return NULL;
    }
    p->cursor++;

    return out;
}

static ten_json_t *ten__json_parse_number(ten__json_parser_t *p)
{
    char *endptr = NULL;
    double value = strtod(p->cursor, &endptr);

    if (endptr == p->cursor)
    {
        return NULL;
    }

    p->cursor = endptr;
    return ten_json_create_number(value);
}

static ten_json_t *ten__json_parse_value(ten__json_parser_t *p)
{
    ten__json_skip_ws(p);
    if (p->cursor >= p->end)
    {
        return NULL;
    }

    switch (*p->cursor)
    {
        case 'n':
            if (p->end - p->cursor >= 4 && memcmp(p->cursor, "null", 4) == 0)
            {
                p->cursor += 4;
                return ten_json_create_null();
            }
            return NULL;

        case 't':
            if (p->end - p->cursor >= 4 && memcmp(p->cursor, "true", 4) == 0)
            {
                p->cursor += 4;
                return ten_json_create_bool(1);
            }
            return NULL;

        case 'f':
            if (p->end - p->cursor >= 5 && memcmp(p->cursor, "false", 5) == 0)
            {
                p->cursor += 5;
                return ten_json_create_bool(0);
            }
            return NULL;

        case '"':
        {
            ten_string_t *s = ten__json_parse_string(p);
            ten_json_t *json;

            if (s == NULL)
            {
                return NULL;
            }

            json = ten__json_alloc(TEN_JSON_STRING);
            if (json == NULL)
            {
                ten_string_destroy(s);
                return NULL;
            }
            json->value.string = s;
            return json;
        }

        case '[':
        {
            ten_json_t *arr = ten_json_create_array();
            if (arr == NULL)
            {
                return NULL;
            }

            p->cursor++;
            ten__json_skip_ws(p);
            if (p->cursor < p->end && *p->cursor == ']')
            {
                p->cursor++;
                return arr;
            }

            for (;;)
            {
                ten_json_t *element = ten__json_parse_value(p);
                if (element == NULL || ten_json_array_add(arr, element) != (int)TEN_OK)
                {
                    ten_json_destroy(element);
                    ten_json_destroy(arr);
                    return NULL;
                }

                ten__json_skip_ws(p);
                if (p->cursor >= p->end)
                {
                    ten_json_destroy(arr);
                    return NULL;
                }
                if (*p->cursor == ',')
                {
                    p->cursor++;
                    continue;
                }
                if (*p->cursor == ']')
                {
                    p->cursor++;
                    break;
                }
                ten_json_destroy(arr);
                return NULL;
            }
            return arr;
        }

        case '{':
        {
            ten_json_t *obj = ten_json_create_object();
            if (obj == NULL)
            {
                return NULL;
            }

            p->cursor++;
            ten__json_skip_ws(p);
            if (p->cursor < p->end && *p->cursor == '}')
            {
                p->cursor++;
                return obj;
            }

            for (;;)
            {
                ten_string_t *key;
                ten_json_t *value;

                ten__json_skip_ws(p);
                key = ten__json_parse_string(p);
                if (key == NULL)
                {
                    ten_json_destroy(obj);
                    return NULL;
                }

                ten__json_skip_ws(p);
                if (p->cursor >= p->end || *p->cursor != ':')
                {
                    ten_string_destroy(key);
                    ten_json_destroy(obj);
                    return NULL;
                }
                p->cursor++;

                value = ten__json_parse_value(p);
                if (value == NULL)
                {
                    ten_string_destroy(key);
                    ten_json_destroy(obj);
                    return NULL;
                }

                if (ten__json_object_set_owned(obj, key, value) != (int)TEN_OK)
                {
                    ten_json_destroy(value);
                    ten_json_destroy(obj);
                    return NULL;
                }

                ten__json_skip_ws(p);
                if (p->cursor >= p->end)
                {
                    ten_json_destroy(obj);
                    return NULL;
                }
                if (*p->cursor == ',')
                {
                    p->cursor++;
                    continue;
                }
                if (*p->cursor == '}')
                {
                    p->cursor++;
                    break;
                }
                ten_json_destroy(obj);
                return NULL;
            }
            return obj;
        }

        default:
            if (*p->cursor == '-' || (*p->cursor >= '0' && *p->cursor <= '9'))
            {
                return ten__json_parse_number(p);
            }
            return NULL;
    }
}

ten_json_t *ten_json_parse(const char *text)
{
    ten__json_parser_t p;
    ten_json_t *result;

    if (text == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    p.cursor = text;
    p.end = text + strlen(text);

    result = ten__json_parse_value(&p);
    if (result == NULL)
    {
        ten__set_error(TEN_ERROR_PARSE);
        return NULL;
    }

    ten__json_skip_ws(&p);
    if (p.cursor != p.end)
    {
        ten_json_destroy(result);
        ten__set_error(TEN_ERROR_PARSE);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return result;
}

/* ================= stringify ================= */

static void ten__json_stringify_escape_string(const char *s, ten_string_t *out)
{
    ten_string_append(out, "\"");

    for (; *s; s++)
    {
        unsigned char c = (unsigned char)*s;

        switch (c)
        {
            case '"':  ten_string_append(out, "\\\""); break;
            case '\\': ten_string_append(out, "\\\\"); break;
            case '\n': ten_string_append(out, "\\n"); break;
            case '\r': ten_string_append(out, "\\r"); break;
            case '\t': ten_string_append(out, "\\t"); break;
            default:
                if (c < 0x20)
                {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    ten_string_append(out, buf);
                }
                else
                {
                    char buf[2];
                    buf[0] = (char)c;
                    buf[1] = '\0';
                    ten_string_append(out, buf);
                }
                break;
        }
    }

    ten_string_append(out, "\"");
}

static void ten__json_stringify_value(const ten_json_t *json, ten_string_t *out)
{
    switch (json->type)
    {
        case TEN_JSON_NULL:
            ten_string_append(out, "null");
            break;

        case TEN_JSON_BOOL:
            ten_string_append(out, json->value.boolean ? "true" : "false");
            break;

        case TEN_JSON_NUMBER:
        {
            char buf[32];
            double v = json->value.number;

            if (v == (double)(long long)v && v > -1e15 && v < 1e15)
            {
                snprintf(buf, sizeof(buf), "%lld", (long long)v);
            }
            else
            {
                snprintf(buf, sizeof(buf), "%.17g", v);
            }
            ten_string_append(out, buf);
            break;
        }

        case TEN_JSON_STRING:
            ten__json_stringify_escape_string(ten_string_cstr(json->value.string), out);
            break;

        case TEN_JSON_ARRAY:
        {
            size_t i;
            size_t count = ten_list_count(json->value.array);

            ten_string_append(out, "[");
            for (i = 0; i < count; i++)
            {
                ten_json_t *element = *(ten_json_t **)ten_list_get(json->value.array, i);
                if (i > 0)
                {
                    ten_string_append(out, ",");
                }
                ten__json_stringify_value(element, out);
            }
            ten_string_append(out, "]");
            break;
        }

        case TEN_JSON_OBJECT:
        {
            size_t i;
            size_t count = ten_list_count(json->value.object.key_order);

            ten_string_append(out, "{");
            for (i = 0; i < count; i++)
            {
                ten_string_t *key = *(ten_string_t **)ten_list_get(json->value.object.key_order, i);
                const char *key_cstr = ten_string_cstr(key);
                void *slot = ten_map_get(json->value.object.entries, &key_cstr);
                ten_json_t *value = *(ten_json_t **)slot;

                if (i > 0)
                {
                    ten_string_append(out, ",");
                }
                ten__json_stringify_escape_string(key_cstr, out);
                ten_string_append(out, ":");
                ten__json_stringify_value(value, out);
            }
            ten_string_append(out, "}");
            break;
        }

        default:
            break;
    }
}

char *ten_json_stringify(const ten_json_t *json)
{
    ten_string_t *out;
    char *result;

    if (json == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    out = ten_string_empty();
    if (out == NULL)
    {
        return NULL;
    }

    ten__json_stringify_value(json, out);

    result = (char *)ten_memdup(ten_string_cstr(out), ten_string_length(out) + 1);
    ten_string_destroy(out);

    if (result == NULL)
    {
        return NULL;
    }

    ten__set_error(TEN_OK);
    return result;
}
