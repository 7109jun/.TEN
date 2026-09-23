#include "ten/http.h"
#include "ten/buffer.h"
#include "ten/memory.h"
#include "ten/sync.h"
#include "internal/error_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEN_HTTP_MAX_HEADER_SIZE (64 * 1024)
#define TEN_HTTP_LINE_BUF 256

typedef struct {
    char *name;
    char *value;
} ten__http_header_pair_t;

struct ten_http_request {
    ten_http_method_t method;
    char *path; /* ten_malloc 소유 */
    ten__http_header_pair_t *headers;
    size_t header_count;
    size_t header_capacity;
    ten_buffer_t *body; /* NULL이면 본문 없음 */
};

struct ten_http_response {
    int status;
    ten__http_header_pair_t *headers;
    size_t header_count;
    size_t header_capacity;
    ten_buffer_t *body;
};

struct ten_http_server {
    ten_socket_t *listener;
    ten_http_handler_t handler;
    void *userdata;
    ten_mutex_t *lock;
    int stopping;
};

#define TEN_HTTP_SERVER_POLL_MS 200

/* ================= 공용 유틸 ================= */

static int ten__ci_strcmp(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0')
    {
        unsigned char ca = (unsigned char)*a;
        unsigned char cb = (unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb - 'A' + 'a');
        if (ca != cb)
        {
            return (int)ca - (int)cb;
        }
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

static int ten__http_headers_set(ten__http_header_pair_t **headers, size_t *count, size_t *capacity,
                                  const char *name, const char *value)
{
    size_t i;
    char *name_copy;
    char *value_copy;

    for (i = 0; i < *count; i++)
    {
        if (ten__ci_strcmp((*headers)[i].name, name) == 0)
        {
            value_copy = (char *)ten_memdup(value, strlen(value) + 1);
            if (value_copy == NULL)
            {
                return (int)TEN_ERROR_OUT_OF_MEMORY;
            }
            ten_free((*headers)[i].value);
            (*headers)[i].value = value_copy;
            return (int)TEN_OK;
        }
    }

    if (*count == *capacity)
    {
        size_t new_capacity = (*capacity == 0) ? 4 : (*capacity * 2);
        ten__http_header_pair_t *new_headers =
            (ten__http_header_pair_t *)ten_realloc(*headers, new_capacity * sizeof(ten__http_header_pair_t));
        if (new_headers == NULL)
        {
            return (int)TEN_ERROR_OUT_OF_MEMORY;
        }
        *headers = new_headers;
        *capacity = new_capacity;
    }

    name_copy = (char *)ten_memdup(name, strlen(name) + 1);
    if (name_copy == NULL)
    {
        return (int)TEN_ERROR_OUT_OF_MEMORY;
    }
    value_copy = (char *)ten_memdup(value, strlen(value) + 1);
    if (value_copy == NULL)
    {
        ten_free(name_copy);
        return (int)TEN_ERROR_OUT_OF_MEMORY;
    }

    (*headers)[*count].name = name_copy;
    (*headers)[*count].value = value_copy;
    (*count)++;

    return (int)TEN_OK;
}

static const char *ten__http_headers_get(ten__http_header_pair_t *headers, size_t count, const char *name)
{
    size_t i;
    for (i = 0; i < count; i++)
    {
        if (ten__ci_strcmp(headers[i].name, name) == 0)
        {
            return headers[i].value;
        }
    }
    return NULL;
}

static void ten__http_headers_free(ten__http_header_pair_t *headers, size_t count)
{
    size_t i;
    for (i = 0; i < count; i++)
    {
        ten_free(headers[i].name);
        ten_free(headers[i].value);
    }
    ten_free(headers);
}

static const char *ten__http_method_name(ten_http_method_t method)
{
    switch (method)
    {
        case TEN_HTTP_GET:     return "GET";
        case TEN_HTTP_POST:    return "POST";
        case TEN_HTTP_PUT:     return "PUT";
        case TEN_HTTP_DELETE:  return "DELETE";
        case TEN_HTTP_HEAD:    return "HEAD";
        case TEN_HTTP_OPTIONS: return "OPTIONS";
        default:                return "GET";
    }
}

static int ten__http_method_parse(const char *s, ten_http_method_t *out)
{
    if (strcmp(s, "GET") == 0)     { *out = TEN_HTTP_GET;     return 1; }
    if (strcmp(s, "POST") == 0)    { *out = TEN_HTTP_POST;    return 1; }
    if (strcmp(s, "PUT") == 0)     { *out = TEN_HTTP_PUT;     return 1; }
    if (strcmp(s, "DELETE") == 0)  { *out = TEN_HTTP_DELETE;  return 1; }
    if (strcmp(s, "HEAD") == 0)    { *out = TEN_HTTP_HEAD;    return 1; }
    if (strcmp(s, "OPTIONS") == 0) { *out = TEN_HTTP_OPTIONS; return 1; }
    return 0;
}

static const char *ten__http_status_text(int status)
{
    switch (status)
    {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 503: return "Service Unavailable";
        default:  return "Unknown";
    }
}

static const unsigned char *ten__find_crlf_crlf(const unsigned char *data, size_t size)
{
    size_t i;
    if (size < 4)
    {
        return NULL;
    }
    for (i = 0; i + 4 <= size; i++)
    {
        if (data[i] == '\r' && data[i + 1] == '\n' && data[i + 2] == '\r' && data[i + 3] == '\n')
        {
            return data + i;
        }
    }
    return NULL;
}

/* raw 소켓에서 "\r\n\r\n"이 나올 때까지 읽어 buffer에 누적한다. */
static int ten__http_read_until_headers_end(ten_socket_t *sock, ten_buffer_t *raw)
{
    char chunk[4096];

    for (;;)
    {
        const unsigned char *data = (const unsigned char *)ten_buffer_data(raw);
        size_t size = ten_buffer_size(raw);
        int64_t n;

        if (ten__find_crlf_crlf(data, size) != NULL)
        {
            return 1;
        }
        if (size > TEN_HTTP_MAX_HEADER_SIZE)
        {
            return 0;
        }

        n = ten_socket_receive(sock, chunk, sizeof(chunk));
        if (n <= 0)
        {
            return 0;
        }

        if (ten_buffer_write(raw, chunk, (size_t)n) != TEN_OK)
        {
            return 0;
        }
    }
}

/* content_length만큼 body를 채운다. body_prefix는 헤더를 읽을 때 이미
   함께 읽혀 있던 본문 앞부분이다. */
static ten_buffer_t *ten__http_read_body(ten_socket_t *sock, long content_length,
                                          const unsigned char *body_prefix, size_t body_prefix_len)
{
    ten_buffer_t *body = ten_buffer_create();
    size_t take;

    if (body == NULL)
    {
        return NULL;
    }
    if (content_length <= 0)
    {
        return body;
    }

    take = (body_prefix_len < (size_t)content_length) ? body_prefix_len : (size_t)content_length;
    if (take > 0)
    {
        ten_buffer_write(body, body_prefix, take);
    }

    while ((long)ten_buffer_size(body) < content_length)
    {
        char chunk[4096];
        size_t remaining = (size_t)content_length - ten_buffer_size(body);
        size_t to_read = (remaining < sizeof(chunk)) ? remaining : sizeof(chunk);
        int64_t n = ten_socket_receive(sock, chunk, to_read);
        if (n <= 0)
        {
            break;
        }
        ten_buffer_write(body, chunk, (size_t)n);
    }

    return body;
}

/* ================= Request ================= */

ten_http_request_t *ten_http_request_create(ten_http_method_t method, const char *path)
{
    ten_http_request_t *req;

    if (path == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    req = (ten_http_request_t *)ten_malloc(sizeof(ten_http_request_t));
    if (req == NULL)
    {
        return NULL;
    }

    req->method = method;
    req->path = (char *)ten_memdup(path, strlen(path) + 1);
    req->headers = NULL;
    req->header_count = 0;
    req->header_capacity = 0;
    req->body = NULL;

    if (req->path == NULL)
    {
        ten_free(req);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return req;
}

void ten_http_request_destroy(ten_http_request_t *request)
{
    if (request == NULL)
    {
        return;
    }
    ten_free(request->path);
    ten__http_headers_free(request->headers, request->header_count);
    ten_buffer_destroy(request->body);
    ten_free(request);
}

int ten_http_request_set_header(ten_http_request_t *request, const char *name, const char *value)
{
    if (request == NULL || name == NULL || value == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }
    return ten__http_headers_set(&request->headers, &request->header_count, &request->header_capacity, name, value);
}

int ten_http_request_set_body(ten_http_request_t *request, const void *data, size_t size)
{
    if (request == NULL || (data == NULL && size > 0))
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (request->body == NULL)
    {
        request->body = ten_buffer_create();
        if (request->body == NULL)
        {
            return (int)TEN_ERROR_OUT_OF_MEMORY;
        }
    }
    else
    {
        ten_buffer_clear(request->body);
    }

    return ten_buffer_write(request->body, data, size);
}

ten_http_method_t ten_http_request_method(const ten_http_request_t *request)
{
    if (request == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return TEN_HTTP_GET;
    }
    ten__set_error(TEN_OK);
    return request->method;
}

const char *ten_http_request_path(const ten_http_request_t *request)
{
    if (request == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }
    ten__set_error(TEN_OK);
    return request->path;
}

const char *ten_http_request_header(const ten_http_request_t *request, const char *name)
{
    if (request == NULL || name == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }
    ten__set_error(TEN_OK);
    return ten__http_headers_get(request->headers, request->header_count, name);
}

const void *ten_http_request_body(const ten_http_request_t *request, size_t *out_size)
{
    if (request == NULL || out_size == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        if (out_size != NULL) *out_size = 0;
        return NULL;
    }

    ten__set_error(TEN_OK);
    if (request->body == NULL)
    {
        *out_size = 0;
        return NULL;
    }

    *out_size = ten_buffer_size(request->body);
    return ten_buffer_data(request->body);
}

/* ================= Response ================= */

static ten_http_response_t *ten__http_response_create(void)
{
    ten_http_response_t *resp = (ten_http_response_t *)ten_malloc(sizeof(ten_http_response_t));
    if (resp == NULL)
    {
        return NULL;
    }
    resp->status = 200;
    resp->headers = NULL;
    resp->header_count = 0;
    resp->header_capacity = 0;
    resp->body = NULL;
    return resp;
}

void ten_http_response_destroy(ten_http_response_t *response)
{
    if (response == NULL)
    {
        return;
    }
    ten__http_headers_free(response->headers, response->header_count);
    ten_buffer_destroy(response->body);
    ten_free(response);
}

int ten_http_response_set_status(ten_http_response_t *response, int status_code)
{
    if (response == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }
    response->status = status_code;
    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

int ten_http_response_set_header(ten_http_response_t *response, const char *name, const char *value)
{
    if (response == NULL || name == NULL || value == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }
    return ten__http_headers_set(&response->headers, &response->header_count, &response->header_capacity, name, value);
}

int ten_http_response_set_body(ten_http_response_t *response, const void *data, size_t size)
{
    if (response == NULL || (data == NULL && size > 0))
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (response->body == NULL)
    {
        response->body = ten_buffer_create();
        if (response->body == NULL)
        {
            return (int)TEN_ERROR_OUT_OF_MEMORY;
        }
    }
    else
    {
        ten_buffer_clear(response->body);
    }

    return ten_buffer_write(response->body, data, size);
}

int ten_http_response_status(const ten_http_response_t *response)
{
    if (response == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return 0;
    }
    ten__set_error(TEN_OK);
    return response->status;
}

const char *ten_http_response_header(const ten_http_response_t *response, const char *name)
{
    if (response == NULL || name == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }
    ten__set_error(TEN_OK);
    return ten__http_headers_get(response->headers, response->header_count, name);
}

const void *ten_http_response_body(const ten_http_response_t *response, size_t *out_size)
{
    if (response == NULL || out_size == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        if (out_size != NULL) *out_size = 0;
        return NULL;
    }

    ten__set_error(TEN_OK);
    if (response->body == NULL)
    {
        *out_size = 0;
        return NULL;
    }

    *out_size = ten_buffer_size(response->body);
    return ten_buffer_data(response->body);
}

/* ================= 파싱(요청줄/상태줄 + 헤더, 제자리 NUL 치환) ================= */

/* headers_text(NUL 종단, 변형 가능)를 줄 단위로 파싱해 헤더 목록을
   채우고 Content-Length를 찾아 반환한다. 첫 줄은 별도 콜백으로 처리한다. */
typedef int (*ten__first_line_fn)(char *line, void *ctx);

static long ten__http_parse_headers_text(char *headers_text, ten__http_header_pair_t **headers,
                                          size_t *count, size_t *capacity,
                                          ten__first_line_fn first_line_fn, void *ctx)
{
    char *cursor = headers_text;
    int first_line = 1;
    long content_length = -1;

    while (cursor != NULL && *cursor != '\0')
    {
        char *line_end = strstr(cursor, "\r\n");
        char *next_cursor;

        if (line_end != NULL)
        {
            *line_end = '\0';
            next_cursor = line_end + 2;
        }
        else
        {
            next_cursor = NULL;
        }

        if (*cursor == '\0')
        {
            /* 빈 줄 — 무시 */
        }
        else if (first_line)
        {
            if (!first_line_fn(cursor, ctx))
            {
                return -2; /* 첫 줄 파싱 실패 */
            }
            first_line = 0;
        }
        else
        {
            char *colon = strchr(cursor, ':');
            if (colon != NULL)
            {
                char *value;
                *colon = '\0';
                value = colon + 1;
                while (*value == ' ')
                {
                    value++;
                }

                if (ten__ci_strcmp(cursor, "Content-Length") == 0)
                {
                    content_length = atol(value);
                }

                ten__http_headers_set(headers, count, capacity, cursor, value);
            }
        }

        cursor = next_cursor;
    }

    if (first_line)
    {
        return -2; /* 요청줄/상태줄이 아예 없었다 */
    }

    return content_length;
}

typedef struct {
    ten_http_request_t *req;
} ten__request_first_line_ctx_t;

static int ten__request_first_line(char *line, void *ctx_v)
{
    ten__request_first_line_ctx_t *ctx = (ten__request_first_line_ctx_t *)ctx_v;
    char *sp1 = strchr(line, ' ');
    char *rest;
    char *sp2;

    if (sp1 == NULL)
    {
        return 0;
    }
    *sp1 = '\0';
    rest = sp1 + 1;

    if (!ten__http_method_parse(line, &ctx->req->method))
    {
        return 0;
    }

    sp2 = strchr(rest, ' ');
    if (sp2 != NULL)
    {
        *sp2 = '\0'; /* " HTTP/1.1" 잘라내기 */
    }

    ctx->req->path = (char *)ten_memdup(rest, strlen(rest) + 1);
    return ctx->req->path != NULL;
}

static ten_http_request_t *ten__http_parse_request(const unsigned char *header_block, size_t header_len,
                                                     const unsigned char *body_prefix, size_t body_prefix_len,
                                                     ten_socket_t *sock)
{
    char *headers_text;
    ten_http_request_t *req;
    ten__request_first_line_ctx_t ctx;
    long content_length;

    headers_text = (char *)ten_malloc(header_len + 1);
    if (headers_text == NULL)
    {
        return NULL;
    }
    memcpy(headers_text, header_block, header_len);
    headers_text[header_len] = '\0';

    req = (ten_http_request_t *)ten_malloc(sizeof(ten_http_request_t));
    if (req == NULL)
    {
        ten_free(headers_text);
        return NULL;
    }
    req->method = TEN_HTTP_GET;
    req->path = NULL;
    req->headers = NULL;
    req->header_count = 0;
    req->header_capacity = 0;
    req->body = NULL;

    ctx.req = req;
    content_length =
        ten__http_parse_headers_text(headers_text, &req->headers, &req->header_count, &req->header_capacity,
                                      ten__request_first_line, &ctx);
    ten_free(headers_text);

    if (content_length == -2)
    {
        ten_http_request_destroy(req);
        return NULL;
    }

    req->body = ten__http_read_body(sock, content_length, body_prefix, body_prefix_len);
    if (req->body == NULL)
    {
        ten_http_request_destroy(req);
        return NULL;
    }

    return req;
}

typedef struct {
    int status;
} ten__response_first_line_ctx_t;

static int ten__response_first_line(char *line, void *ctx_v)
{
    ten__response_first_line_ctx_t *ctx = (ten__response_first_line_ctx_t *)ctx_v;
    char *sp1 = strchr(line, ' '); /* "HTTP/1.1" 뒤 */
    char *status_str;
    char *sp2;

    if (sp1 == NULL)
    {
        return 0;
    }
    status_str = sp1 + 1;
    sp2 = strchr(status_str, ' '); /* status 뒤 reason phrase 앞 */
    if (sp2 != NULL)
    {
        *sp2 = '\0';
    }

    ctx->status = atoi(status_str);
    return ctx->status > 0;
}

static ten_http_response_t *ten__http_parse_response(const unsigned char *header_block, size_t header_len,
                                                       const unsigned char *body_prefix, size_t body_prefix_len,
                                                       ten_socket_t *sock)
{
    char *headers_text;
    ten_http_response_t *resp;
    ten__response_first_line_ctx_t ctx;
    long content_length;

    headers_text = (char *)ten_malloc(header_len + 1);
    if (headers_text == NULL)
    {
        return NULL;
    }
    memcpy(headers_text, header_block, header_len);
    headers_text[header_len] = '\0';

    resp = ten__http_response_create();
    if (resp == NULL)
    {
        ten_free(headers_text);
        return NULL;
    }

    ctx.status = 0;
    content_length =
        ten__http_parse_headers_text(headers_text, &resp->headers, &resp->header_count, &resp->header_capacity,
                                      ten__response_first_line, &ctx);
    ten_free(headers_text);

    if (content_length == -2)
    {
        ten_http_response_destroy(resp);
        return NULL;
    }
    resp->status = ctx.status;

    resp->body = ten__http_read_body(sock, content_length, body_prefix, body_prefix_len);
    if (resp->body == NULL)
    {
        ten_http_response_destroy(resp);
        return NULL;
    }

    return resp;
}

/* ================= 직렬화 + 전송 ================= */

static int ten__http_send_all(ten_socket_t *sock, const void *data, size_t size)
{
    const unsigned char *bytes = (const unsigned char *)data;
    size_t sent = 0;

    while (sent < size)
    {
        int64_t n = ten_socket_send(sock, bytes + sent, size - sent);
        if (n <= 0)
        {
            return (int)TEN_ERROR_NETWORK;
        }
        sent += (size_t)n;
    }

    return (int)TEN_OK;
}

static void ten__http_write_headers(ten_buffer_t *out, ten__http_header_pair_t *headers, size_t count)
{
    size_t i;
    for (i = 0; i < count; i++)
    {
        ten_buffer_write(out, headers[i].name, strlen(headers[i].name));
        ten_buffer_write(out, ": ", 2);
        ten_buffer_write(out, headers[i].value, strlen(headers[i].value));
        ten_buffer_write(out, "\r\n", 2);
    }
}

static int ten__http_send_request(ten_socket_t *sock, const ten_http_request_t *request, const char *host)
{
    ten_buffer_t *out = ten_buffer_create();
    char line[TEN_HTTP_LINE_BUF];
    size_t body_size;
    int rc;

    if (out == NULL)
    {
        return (int)TEN_ERROR_OUT_OF_MEMORY;
    }

    snprintf(line, sizeof(line), "%s %s HTTP/1.1\r\n", ten__http_method_name(request->method), request->path);
    ten_buffer_write(out, line, strlen(line));

    snprintf(line, sizeof(line), "Host: %s\r\n", host);
    ten_buffer_write(out, line, strlen(line));

    ten__http_write_headers(out, request->headers, request->header_count);

    body_size = (request->body != NULL) ? ten_buffer_size(request->body) : 0;
    snprintf(line, sizeof(line), "Content-Length: %lu\r\nConnection: close\r\n\r\n", (unsigned long)body_size);
    ten_buffer_write(out, line, strlen(line));

    if (body_size > 0)
    {
        ten_buffer_write(out, ten_buffer_data(request->body), body_size);
    }

    rc = ten__http_send_all(sock, ten_buffer_data(out), ten_buffer_size(out));
    ten_buffer_destroy(out);
    return rc;
}

static int ten__http_send_response(ten_socket_t *sock, ten_http_response_t *response)
{
    ten_buffer_t *out = ten_buffer_create();
    char line[TEN_HTTP_LINE_BUF];
    size_t body_size;
    int rc;

    if (out == NULL)
    {
        return (int)TEN_ERROR_OUT_OF_MEMORY;
    }

    snprintf(line, sizeof(line), "HTTP/1.1 %d %s\r\n", response->status, ten__http_status_text(response->status));
    ten_buffer_write(out, line, strlen(line));

    ten__http_write_headers(out, response->headers, response->header_count);

    body_size = (response->body != NULL) ? ten_buffer_size(response->body) : 0;
    snprintf(line, sizeof(line), "Content-Length: %lu\r\nConnection: close\r\n\r\n", (unsigned long)body_size);
    ten_buffer_write(out, line, strlen(line));

    if (body_size > 0)
    {
        ten_buffer_write(out, ten_buffer_data(response->body), body_size);
    }

    rc = ten__http_send_all(sock, ten_buffer_data(out), ten_buffer_size(out));
    ten_buffer_destroy(out);
    return rc;
}

/* ================= Client ================= */

ten_http_response_t *ten_http_client_send(const char *host, uint16_t port, const ten_http_request_t *request)
{
    ten_socket_t *sock;
    ten_buffer_t *raw;
    ten_http_response_t *resp = NULL;

    if (host == NULL || request == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    sock = ten_tcp_connect(host, port);
    if (sock == NULL)
    {
        return NULL; /* ten_tcp_connect가 이미 오류를 설정했다 */
    }

    if (ten__http_send_request(sock, request, host) != (int)TEN_OK)
    {
        ten_socket_close(sock);
        ten__set_error(TEN_ERROR_NETWORK);
        return NULL;
    }

    raw = ten_buffer_create();
    if (raw == NULL)
    {
        ten_socket_close(sock);
        return NULL;
    }

    if (!ten__http_read_until_headers_end(sock, raw))
    {
        ten_buffer_destroy(raw);
        ten_socket_close(sock);
        ten__set_error(TEN_ERROR_NETWORK);
        return NULL;
    }

    {
        const unsigned char *data = (const unsigned char *)ten_buffer_data(raw);
        size_t size = ten_buffer_size(raw);
        const unsigned char *hdr_end = ten__find_crlf_crlf(data, size);
        size_t header_len = (size_t)(hdr_end - data);
        const unsigned char *body_prefix = hdr_end + 4;
        size_t body_prefix_len = size - header_len - 4;

        resp = ten__http_parse_response(data, header_len, body_prefix, body_prefix_len, sock);
    }

    ten_buffer_destroy(raw);
    ten_socket_close(sock);

    if (resp == NULL)
    {
        ten__set_error(TEN_ERROR_PARSE);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return resp;
}

/* ================= Server ================= */

ten_http_server_t *ten_http_server_create(const char *host, uint16_t port,
                                           ten_http_handler_t handler, void *userdata)
{
    ten_http_server_t *server;
    ten_socket_t *listener;

    if (host == NULL || handler == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    listener = ten_tcp_listen(host, port, 32);
    if (listener == NULL)
    {
        return NULL; /* ten_tcp_listen이 이미 오류를 설정했다 */
    }

    server = (ten_http_server_t *)ten_malloc(sizeof(ten_http_server_t));
    if (server == NULL)
    {
        ten_socket_close(listener);
        return NULL;
    }

    server->lock = ten_mutex_create();
    if (server->lock == NULL)
    {
        ten_free(server);
        ten_socket_close(listener);
        return NULL;
    }

    server->listener = listener;
    server->handler = handler;
    server->userdata = userdata;
    server->stopping = 0;

    ten__set_error(TEN_OK);
    return server;
}

static void ten__http_handle_connection(ten_http_server_t *server, ten_socket_t *client)
{
    ten_buffer_t *raw = ten_buffer_create();
    ten_http_request_t *req = NULL;

    if (raw == NULL)
    {
        return;
    }

    if (!ten__http_read_until_headers_end(client, raw))
    {
        ten_buffer_destroy(raw);
        return;
    }

    {
        const unsigned char *data = (const unsigned char *)ten_buffer_data(raw);
        size_t size = ten_buffer_size(raw);
        const unsigned char *hdr_end = ten__find_crlf_crlf(data, size);
        size_t header_len = (size_t)(hdr_end - data);
        const unsigned char *body_prefix = hdr_end + 4;
        size_t body_prefix_len = size - header_len - 4;

        req = ten__http_parse_request(data, header_len, body_prefix, body_prefix_len, client);
    }

    ten_buffer_destroy(raw);

    if (req == NULL)
    {
        ten_http_response_t *error_resp = ten__http_response_create();
        if (error_resp != NULL)
        {
            error_resp->status = 400;
            ten__http_send_response(client, error_resp);
            ten_http_response_destroy(error_resp);
        }
        return;
    }

    {
        ten_http_response_t *resp = ten__http_response_create();
        if (resp != NULL)
        {
            server->handler(req, resp, server->userdata);
            ten__http_send_response(client, resp);
            ten_http_response_destroy(resp);
        }
    }

    ten_http_request_destroy(req);
}

int ten_http_server_run(ten_http_server_t *server)
{
    if (server == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    for (;;)
    {
        int stopping;
        int ready;
        ten_socket_t *client;

        ten_mutex_lock(server->lock);
        stopping = server->stopping;
        ten_mutex_unlock(server->lock);
        if (stopping)
        {
            break;
        }

        /* accept()를 짧은 타임아웃으로 poll해서, 다른 스레드가 소켓을
           닫는 레이스 없이 stopping 플래그를 주기적으로 재확인한다. */
        ready = ten_socket_poll_readable(server->listener, TEN_HTTP_SERVER_POLL_MS);
        if (ready <= 0)
        {
            continue;
        }

        client = ten_tcp_accept(server->listener);
        if (client == NULL)
        {
            continue;
        }

        ten__http_handle_connection(server, client);
        ten_socket_close(client);
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

void ten_http_server_stop(ten_http_server_t *server)
{
    if (server == NULL)
    {
        return;
    }
    ten_mutex_lock(server->lock);
    server->stopping = 1;
    ten_mutex_unlock(server->lock);
}

void ten_http_server_destroy(ten_http_server_t *server)
{
    if (server == NULL)
    {
        return;
    }
    /* run()이 반환한 뒤(스레드 join 후) 호출한다고 가정한다 — 그렇지
       않으면 실행 중인 run()이 닫힌 소켓을 poll하게 된다. */
    ten_socket_close(server->listener);
    ten_mutex_destroy(server->lock);
    ten_free(server);
}
