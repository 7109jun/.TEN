#include "ten/encoding.h"
#include "ten/memory.h"
#include "internal/error_internal.h"

#include <string.h>

/* ================= Base64 ================= */

static const char TEN_BASE64_ALPHABET[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int ten__base64_value(unsigned char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

char *ten_base64_encode(const void *data, size_t size)
{
    const unsigned char *bytes = (const unsigned char *)data;
    size_t out_len;
    char *out;
    size_t i;
    size_t j;

    if (data == NULL && size > 0)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    out_len = ((size + 2) / 3) * 4;
    out = (char *)ten_malloc(out_len + 1);
    if (out == NULL)
    {
        return NULL;
    }

    for (i = 0, j = 0; i < size; i += 3)
    {
        uint32_t a = bytes[i];
        uint32_t b = (i + 1 < size) ? bytes[i + 1] : 0;
        uint32_t c = (i + 2 < size) ? bytes[i + 2] : 0;
        uint32_t triple = (a << 16) | (b << 8) | c;

        out[j++] = TEN_BASE64_ALPHABET[(triple >> 18) & 0x3F];
        out[j++] = TEN_BASE64_ALPHABET[(triple >> 12) & 0x3F];
        out[j++] = (i + 1 < size) ? TEN_BASE64_ALPHABET[(triple >> 6) & 0x3F] : '=';
        out[j++] = (i + 2 < size) ? TEN_BASE64_ALPHABET[triple & 0x3F] : '=';
    }
    out[j] = '\0';

    ten__set_error(TEN_OK);
    return out;
}

void *ten_base64_decode(const char *text, size_t *size)
{
    size_t len;
    size_t i;
    unsigned char *out;
    size_t out_len;
    size_t out_pos = 0;

    if (text == NULL || size == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    len = strlen(text);
    if (len % 4 != 0)
    {
        ten__set_error(TEN_ERROR_ENCODING);
        return NULL;
    }

    out_len = (len / 4) * 3;
    out = (unsigned char *)ten_malloc(out_len > 0 ? out_len : 1);
    if (out == NULL)
    {
        return NULL;
    }

    for (i = 0; i < len; i += 4)
    {
        int a = ten__base64_value((unsigned char)text[i]);
        int b = ten__base64_value((unsigned char)text[i + 1]);
        int pad2 = (text[i + 2] == '=');
        int pad3 = (text[i + 3] == '=');
        int c = pad2 ? 0 : ten__base64_value((unsigned char)text[i + 2]);
        int d = pad3 ? 0 : ten__base64_value((unsigned char)text[i + 3]);

        if (a < 0 || b < 0 || (!pad2 && c < 0) || (!pad3 && d < 0) || (pad2 && !pad3))
        {
            ten_free(out);
            ten__set_error(TEN_ERROR_ENCODING);
            return NULL;
        }

        out[out_pos++] = (unsigned char)((a << 2) | (b >> 4));
        if (!pad2)
        {
            out[out_pos++] = (unsigned char)(((b & 0xF) << 4) | (c >> 2));
        }
        if (!pad3)
        {
            out[out_pos++] = (unsigned char)(((c & 0x3) << 6) | d);
        }
    }

    *size = out_pos;
    ten__set_error(TEN_OK);
    return out;
}

/* ================= Hex ================= */

static const char TEN_HEX_DIGITS[] = "0123456789abcdef";

static int ten__hex_value(unsigned char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

char *ten_hex_encode(const void *data, size_t size)
{
    const unsigned char *bytes = (const unsigned char *)data;
    char *out;
    size_t i;

    if (data == NULL && size > 0)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    out = (char *)ten_malloc(size * 2 + 1);
    if (out == NULL)
    {
        return NULL;
    }

    for (i = 0; i < size; i++)
    {
        out[i * 2] = TEN_HEX_DIGITS[bytes[i] >> 4];
        out[i * 2 + 1] = TEN_HEX_DIGITS[bytes[i] & 0xF];
    }
    out[size * 2] = '\0';

    ten__set_error(TEN_OK);
    return out;
}

void *ten_hex_decode(const char *text, size_t *size)
{
    size_t len;
    size_t i;
    unsigned char *out;

    if (text == NULL || size == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    len = strlen(text);
    if (len % 2 != 0)
    {
        ten__set_error(TEN_ERROR_ENCODING);
        return NULL;
    }

    out = (unsigned char *)ten_malloc(len / 2 > 0 ? len / 2 : 1);
    if (out == NULL)
    {
        return NULL;
    }

    for (i = 0; i < len; i += 2)
    {
        int hi = ten__hex_value((unsigned char)text[i]);
        int lo = ten__hex_value((unsigned char)text[i + 1]);

        if (hi < 0 || lo < 0)
        {
            ten_free(out);
            ten__set_error(TEN_ERROR_ENCODING);
            return NULL;
        }

        out[i / 2] = (unsigned char)((hi << 4) | lo);
    }

    *size = len / 2;
    ten__set_error(TEN_OK);
    return out;
}

/* ================= UTF-8 디코딩 헬퍼(내부 공용) ================= */

static int ten__utf8_decode_one(const unsigned char *s, size_t remaining, uint32_t *out_cp, size_t *out_len)
{
    unsigned char c = s[0];

    if (c < 0x80)
    {
        *out_cp = c;
        *out_len = 1;
        return 1;
    }

    if ((c & 0xE0) == 0xC0 && remaining >= 2 && (s[1] & 0xC0) == 0x80)
    {
        *out_cp = (uint32_t)((c & 0x1F) << 6) | (uint32_t)(s[1] & 0x3F);
        *out_len = 2;
        return *out_cp >= 0x80;
    }

    if ((c & 0xF0) == 0xE0 && remaining >= 3 && (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80)
    {
        *out_cp = ((uint32_t)(c & 0x0F) << 12) | ((uint32_t)(s[1] & 0x3F) << 6) | (uint32_t)(s[2] & 0x3F);
        *out_len = 3;
        return *out_cp >= 0x800;
    }

    if ((c & 0xF8) == 0xF0 && remaining >= 4 &&
        (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80 && (s[3] & 0xC0) == 0x80)
    {
        *out_cp = ((uint32_t)(c & 0x07) << 18) | ((uint32_t)(s[1] & 0x3F) << 12) |
                  ((uint32_t)(s[2] & 0x3F) << 6) | (uint32_t)(s[3] & 0x3F);
        *out_len = 4;
        return *out_cp >= 0x10000 && *out_cp <= 0x10FFFF;
    }

    return 0;
}

/* ================= UTF-8 <-> UTF-16 ================= */

uint16_t *ten_utf8_to_utf16(const char *utf8, size_t *out_length)
{
    size_t len;
    size_t i;
    size_t units = 0;
    uint16_t *out;
    size_t pos = 0;

    if (utf8 == NULL || out_length == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    len = strlen(utf8);

    for (i = 0; i < len;)
    {
        uint32_t cp;
        size_t clen;

        if (!ten__utf8_decode_one((const unsigned char *)utf8 + i, len - i, &cp, &clen))
        {
            ten__set_error(TEN_ERROR_ENCODING);
            return NULL;
        }

        units += (cp > 0xFFFF) ? 2 : 1;
        i += clen;
    }

    out = (uint16_t *)ten_malloc((units + 1) * sizeof(uint16_t));
    if (out == NULL)
    {
        return NULL;
    }

    for (i = 0; i < len;)
    {
        uint32_t cp;
        size_t clen;

        ten__utf8_decode_one((const unsigned char *)utf8 + i, len - i, &cp, &clen); /* 이미 검증됨 */

        if (cp > 0xFFFF)
        {
            uint32_t v = cp - 0x10000;
            out[pos++] = (uint16_t)(0xD800 + (v >> 10));
            out[pos++] = (uint16_t)(0xDC00 + (v & 0x3FF));
        }
        else
        {
            out[pos++] = (uint16_t)cp;
        }

        i += clen;
    }
    out[pos] = 0;

    *out_length = units;
    ten__set_error(TEN_OK);
    return out;
}

char *ten_utf16_to_utf8(const uint16_t *utf16, size_t length)
{
    size_t i;
    size_t bytes = 0;
    char *out;
    size_t pos = 0;

    if (utf16 == NULL && length > 0)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    for (i = 0; i < length; i++)
    {
        uint32_t cp;
        uint16_t u = utf16[i];

        if (u >= 0xD800 && u <= 0xDBFF)
        {
            if (i + 1 >= length || utf16[i + 1] < 0xDC00 || utf16[i + 1] > 0xDFFF)
            {
                ten__set_error(TEN_ERROR_ENCODING);
                return NULL;
            }
            cp = 0x10000u + (((uint32_t)u - 0xD800u) << 10) + ((uint32_t)utf16[i + 1] - 0xDC00u);
            i++;
        }
        else if (u >= 0xDC00 && u <= 0xDFFF)
        {
            ten__set_error(TEN_ERROR_ENCODING); /* 짝 없는 low surrogate */
            return NULL;
        }
        else
        {
            cp = u;
        }

        if (cp < 0x80) bytes += 1;
        else if (cp < 0x800) bytes += 2;
        else if (cp < 0x10000) bytes += 3;
        else bytes += 4;
    }

    out = (char *)ten_malloc(bytes + 1);
    if (out == NULL)
    {
        return NULL;
    }

    for (i = 0; i < length; i++)
    {
        uint32_t cp;
        uint16_t u = utf16[i];

        if (u >= 0xD800 && u <= 0xDBFF)
        {
            cp = 0x10000u + (((uint32_t)u - 0xD800u) << 10) + ((uint32_t)utf16[i + 1] - 0xDC00u);
            i++;
        }
        else
        {
            cp = u;
        }

        if (cp < 0x80)
        {
            out[pos++] = (char)cp;
        }
        else if (cp < 0x800)
        {
            out[pos++] = (char)(0xC0 | (cp >> 6));
            out[pos++] = (char)(0x80 | (cp & 0x3F));
        }
        else if (cp < 0x10000)
        {
            out[pos++] = (char)(0xE0 | (cp >> 12));
            out[pos++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            out[pos++] = (char)(0x80 | (cp & 0x3F));
        }
        else
        {
            out[pos++] = (char)(0xF0 | (cp >> 18));
            out[pos++] = (char)(0x80 | ((cp >> 12) & 0x3F));
            out[pos++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            out[pos++] = (char)(0x80 | (cp & 0x3F));
        }
    }
    out[pos] = '\0';

    ten__set_error(TEN_OK);
    return out;
}

/* ================= 유효성 검사 ================= */

int ten_ascii_validate(const char *text, size_t size)
{
    size_t i;

    if (text == NULL && size > 0)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return 0;
    }

    for (i = 0; i < size; i++)
    {
        if ((unsigned char)text[i] > 0x7F)
        {
            ten__set_error(TEN_OK);
            return 0;
        }
    }

    ten__set_error(TEN_OK);
    return 1;
}

int ten_utf8_validate(const char *text, size_t size)
{
    size_t i = 0;

    if (text == NULL && size > 0)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return 0;
    }

    while (i < size)
    {
        uint32_t cp;
        size_t clen;

        if (!ten__utf8_decode_one((const unsigned char *)text + i, size - i, &cp, &clen))
        {
            ten__set_error(TEN_OK);
            return 0;
        }

        i += clen;
    }

    ten__set_error(TEN_OK);
    return 1;
}
