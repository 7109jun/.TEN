#include <assert.h>
#include <string.h>
#include <ten/ten.h>

static void echo_handler(const ten_http_request_t *request, ten_http_response_t *response, void *userdata)
{
    int *counter = (int *)userdata;
    const char *path = ten_http_request_path(request);
    (*counter)++;

    if (strcmp(path, "/hello") == 0 && ten_http_request_method(request) == TEN_HTTP_GET)
    {
        ten_http_response_set_status(response, 200);
        ten_http_response_set_header(response, "Content-Type", "text/plain");
        ten_http_response_set_body(response, "world", 5);
    }
    else if (strcmp(path, "/echo") == 0 && ten_http_request_method(request) == TEN_HTTP_POST)
    {
        size_t body_size;
        const void *body = ten_http_request_body(request, &body_size);
        ten_http_response_set_status(response, 201);
        ten_http_response_set_body(response, body, body_size);
    }
    else
    {
        ten_http_response_set_status(response, 404);
    }
}

static void *server_run_fn(void *arg)
{
    ten_http_server_t *server = (ten_http_server_t *)arg;
    ten_http_server_run(server);
    return NULL;
}

static void test_http_roundtrip(void)
{
    int counter = 0;
    ten_http_server_t *server = ten_http_server_create("127.0.0.1", 17656, echo_handler, &counter);
    ten_thread_t *server_thread;
    ten_http_request_t *req;
    ten_http_response_t *resp;
    const void *body;
    size_t body_size;
    const char *content_type;

    assert(server != NULL);

    server_thread = ten_thread_create(server_run_fn, server);
    assert(server_thread != NULL);
    ten_thread_sleep(50);

    /* GET /hello */
    req = ten_http_request_create(TEN_HTTP_GET, "/hello");
    assert(req != NULL);
    resp = ten_http_client_send("127.0.0.1", 17656, req);
    assert(resp != NULL);
    assert(ten_http_response_status(resp) == 200);
    body = ten_http_response_body(resp, &body_size);
    assert(body_size == 5);
    assert(memcmp(body, "world", 5) == 0);
    /* 대소문자 구분 없이 헤더 조회가 되는지 확인 */
    content_type = ten_http_response_header(resp, "content-type");
    assert(content_type != NULL);
    assert(strcmp(content_type, "text/plain") == 0);
    ten_http_request_destroy(req);
    ten_http_response_destroy(resp);

    /* POST /echo (본문 왕복) */
    req = ten_http_request_create(TEN_HTTP_POST, "/echo");
    assert(ten_http_request_set_body(req, "payload-data", 12) == TEN_OK);
    resp = ten_http_client_send("127.0.0.1", 17656, req);
    assert(resp != NULL);
    assert(ten_http_response_status(resp) == 201);
    body = ten_http_response_body(resp, &body_size);
    assert(body_size == 12);
    assert(memcmp(body, "payload-data", 12) == 0);
    ten_http_request_destroy(req);
    ten_http_response_destroy(resp);

    /* 알 수 없는 경로 -> 404 */
    req = ten_http_request_create(TEN_HTTP_GET, "/nope");
    resp = ten_http_client_send("127.0.0.1", 17656, req);
    assert(resp != NULL);
    assert(ten_http_response_status(resp) == 404);
    ten_http_request_destroy(req);
    ten_http_response_destroy(resp);

    assert(counter == 3);

    ten_http_server_stop(server);
    ten_thread_join(server_thread, NULL);
    ten_http_server_destroy(server);
}

static void test_request_response_accessors(void)
{
    ten_http_request_t *req = ten_http_request_create(TEN_HTTP_PUT, "/x");
    ten_http_response_t *resp;

    assert(req != NULL);
    assert(ten_http_request_method(req) == TEN_HTTP_PUT);
    assert(strcmp(ten_http_request_path(req), "/x") == 0);

    assert(ten_http_request_set_header(req, "X-Test", "one") == TEN_OK);
    assert(strcmp(ten_http_request_header(req, "x-test"), "one") == 0); /* 대소문자 무시 */
    assert(ten_http_request_set_header(req, "X-Test", "two") == TEN_OK); /* 덮어쓰기 */
    assert(strcmp(ten_http_request_header(req, "X-Test"), "two") == 0);
    assert(ten_http_request_header(req, "missing") == NULL);

    ten_http_request_destroy(req);

    /* Response는 서버 내부에서만 생성되므로, 여기서는 접근자 자체의
       NULL 처리만 확인한다. */
    resp = NULL;
    assert(ten_http_response_status(resp) == 0);
    assert(ten_last_error() == TEN_ERROR_NULL_ARGUMENT);
}

static void test_null_args(void)
{
    assert(ten_http_request_create(TEN_HTTP_GET, NULL) == NULL);
    assert(ten_last_error() == TEN_ERROR_NULL_ARGUMENT);

    assert(ten_http_server_create(NULL, 8080, echo_handler, NULL) == NULL);
    assert(ten_last_error() == TEN_ERROR_NULL_ARGUMENT);

    /* destroy(NULL) 계열은 안전해야 한다 */
    ten_http_request_destroy(NULL);
    ten_http_response_destroy(NULL);
    ten_http_server_destroy(NULL);
    ten_http_server_stop(NULL);
}

int main(void)
{
    test_http_roundtrip();
    test_request_response_accessors();
    test_null_args();
    return 0;
}
