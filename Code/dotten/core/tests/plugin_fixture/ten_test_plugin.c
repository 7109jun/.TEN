/*
 * test_plugin.c가 ten_plugin_load()로 실제 로드해서 검증하는 더미
 * 플러그인. .TEN 코어 라이브러리에 링크하지 않는 완전히 독립된 작은
 * 공유 라이브러리다.
 */

#if defined(_WIN32)
#define TEN_TEST_PLUGIN_EXPORT __declspec(dllexport)
#else
#define TEN_TEST_PLUGIN_EXPORT
#endif

static int g_value = 0;

TEN_TEST_PLUGIN_EXPORT int ten_plugin_init(void)
{
    g_value = 42;
    return 0;
}

TEN_TEST_PLUGIN_EXPORT void ten_plugin_shutdown(void)
{
    g_value = 0;
}

TEN_TEST_PLUGIN_EXPORT int ten_test_plugin_get_value(void)
{
    return g_value;
}

TEN_TEST_PLUGIN_EXPORT int ten_test_plugin_add(int a, int b)
{
    return a + b;
}
