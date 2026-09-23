#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ten/ten.h>

static ten_log_level_t g_captured_level;
static char g_captured_message[256];
static int g_capture_count;

static void test_callback(ten_log_level_t level, const char *message, void *userdata)
{
    int *counter = (int *)userdata;
    g_captured_level = level;
    strncpy(g_captured_message, message, sizeof(g_captured_message) - 1);
    g_captured_message[sizeof(g_captured_message) - 1] = '\0';
    g_capture_count++;
    if (counter != NULL)
    {
        (*counter)++;
    }
}

int main(void)
{
    int counter = 0;
    FILE *f;
    char line[256];
    int found_in_file;

    ten_log_set_console(0); /* 테스트 출력이 stdout을 어지럽히지 않도록 끈다 */
    ten_log_set_callback(test_callback, &counter);

    /* 기본 최소 레벨(TRACE)에서는 모두 통과해야 한다 */
    ten_log_info("hello %s, %d", ".TEN", 42);
    assert(g_capture_count == 1);
    assert(counter == 1);
    assert(g_captured_level == TEN_LOG_INFO);
    assert(strcmp(g_captured_message, "hello .TEN, 42") == 0);

    /* 레벨 필터링: WARN 미만은 걸러져야 한다 */
    ten_log_set_level(TEN_LOG_WARN);
    ten_log_info("this should be filtered");
    assert(g_capture_count == 1); /* 변화 없음 */

    ten_log_error("this should pass");
    assert(g_capture_count == 2);
    assert(g_captured_level == TEN_LOG_ERROR);

    ten_log_set_level(TEN_LOG_TRACE); /* 원복 */

    /* NULL format */
    ten_log(TEN_LOG_INFO, NULL);
    assert(ten_last_error() == TEN_ERROR_NULL_ARGUMENT);
    assert(g_capture_count == 2); /* 콜백이 호출되지 않았어야 한다 */

    /* 파일 출력 (콜백이 여전히 등록되어 있으므로 콜백도 함께 호출된다) */
    assert(ten_log_set_file("ten_test_log.txt") == TEN_OK);
    ten_log_warn("to the file");
    assert(g_capture_count == 3);
    assert(ten_log_set_file(NULL) == TEN_OK); /* flush 겸 닫기 */

    f = fopen("ten_test_log.txt", "r");
    assert(f != NULL);
    found_in_file = 0;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        if (strstr(line, "to the file") != NULL)
        {
            found_in_file = 1;
        }
    }
    fclose(f);
    assert(found_in_file);
    remove("ten_test_log.txt");

    /* 콜백 해제 */
    ten_log_set_callback(NULL, NULL);
    ten_log_info("no one is listening");
    assert(g_capture_count == 3); /* 변화 없음 */

    return 0;
}
