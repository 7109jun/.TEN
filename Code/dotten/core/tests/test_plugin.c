#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ten/ten.h>

typedef int (*get_value_fn)(void);
typedef int (*add_fn)(int, int);

/* dlsym 계열이 반환하는 void*를 함수 포인터로 바꾸는 표준(POSIX 권장)
   패턴 — plugin_posix.c의 ten__symbol_to_function과 같은 이유. */
static void symbol_to_function(void *symbol, void *out_fn_ptr)
{
    memcpy(out_fn_ptr, &symbol, sizeof(symbol));
}

int main(void)
{
    const char *plugin_path = getenv("TEN_TEST_PLUGIN_PATH");
    ten_plugin_t *plugin;
    get_value_fn get_value = NULL;
    add_fn add = NULL;
    void *symbol;

    assert(plugin_path != NULL); /* 테스트 러너(CMake/스크립트)가 설정해야 한다 */

    plugin = ten_plugin_load(plugin_path);
    assert(plugin != NULL);

    /* ABI 계약: ten_plugin_init/shutdown 심볼을 찾아 호출 */
    assert(ten_plugin_call_init(plugin) == 0);

    symbol = ten_plugin_symbol(plugin, "ten_test_plugin_get_value");
    assert(symbol != NULL);
    symbol_to_function(symbol, &get_value);
    assert(get_value() == 42); /* ten_plugin_init()이 g_value = 42로 설정했어야 한다 */

    symbol = ten_plugin_symbol(plugin, "ten_test_plugin_add");
    assert(symbol != NULL);
    symbol_to_function(symbol, &add);
    assert(add(3, 4) == 7);

    ten_plugin_call_shutdown(plugin);
    assert(get_value() == 0); /* ten_plugin_shutdown()이 초기화했어야 한다 */

    /* 존재하지 않는 심볼 */
    assert(ten_plugin_symbol(plugin, "does_not_exist_xyz") == NULL);
    assert(ten_last_error() == TEN_ERROR_NOT_FOUND);

    ten_plugin_unload(plugin);

    /* 존재하지 않는 라이브러리 */
    assert(ten_plugin_load("/no/such/plugin.so") == NULL);
    assert(ten_last_error() == TEN_ERROR_IO);

    /* NULL 인자 */
    assert(ten_plugin_load(NULL) == NULL);
    assert(ten_last_error() == TEN_ERROR_NULL_ARGUMENT);
    assert(ten_plugin_symbol(NULL, "x") == NULL);
    assert(ten_plugin_call_init(NULL) == (int)TEN_ERROR_NULL_ARGUMENT);

    /* unload(NULL)/call_shutdown(NULL)은 안전해야 한다 */
    ten_plugin_unload(NULL);
    ten_plugin_call_shutdown(NULL);

    return 0;
}
