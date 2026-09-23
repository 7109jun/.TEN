#include "ten/plugin.h"
#include "ten/memory.h"
#include "internal/error_internal.h"

#include <dlfcn.h>
#include <string.h>

/*
 * dlsym()은 void*를 반환하지만 우리가 원하는 건 함수 포인터다. ISO C는
 * object pointer <-> function pointer 캐스트를 금지하지만(-Wpedantic
 * 경고), POSIX는 dlsym에 한해 이 변환이 동작할 것을 요구하며 memcpy를
 * 통한 타입 펀닝을 권장한다(dlsym(3) man page 참고). sizeof(void*) ==
 * sizeof(함수 포인터)라고 가정한다 — 사양서 공식 지원 플랫폼
 * (Windows/Linux/Android)에서는 항상 성립한다.
 */
static void ten__symbol_to_function(void *symbol, void *out_fn_ptr)
{
    memcpy(out_fn_ptr, &symbol, sizeof(symbol));
}

struct ten_plugin {
    void *handle;
};

ten_plugin_t *ten_plugin_load(const char *path)
{
    void *handle;
    ten_plugin_t *plugin;

    if (path == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (handle == NULL)
    {
        ten__set_error(TEN_ERROR_IO);
        return NULL;
    }

    plugin = (ten_plugin_t *)ten_malloc(sizeof(ten_plugin_t));
    if (plugin == NULL)
    {
        dlclose(handle);
        return NULL;
    }

    plugin->handle = handle;
    ten__set_error(TEN_OK);
    return plugin;
}

void *ten_plugin_symbol(ten_plugin_t *plugin, const char *name)
{
    void *sym;

    if (plugin == NULL || name == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    dlerror(); /* 이전 오류 기록 비우기 */
    sym = dlsym(plugin->handle, name);
    if (sym == NULL)
    {
        ten__set_error(TEN_ERROR_NOT_FOUND);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return sym;
}

void ten_plugin_unload(ten_plugin_t *plugin)
{
    if (plugin == NULL)
    {
        return;
    }
    dlclose(plugin->handle);
    ten_free(plugin);
}

int ten_plugin_call_init(ten_plugin_t *plugin)
{
    void *symbol;
    ten_plugin_init_fn fn = NULL;

    if (plugin == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    symbol = ten_plugin_symbol(plugin, "ten_plugin_init");
    if (symbol == NULL)
    {
        ten__set_error(TEN_ERROR_NOT_SUPPORTED);
        return (int)TEN_ERROR_NOT_SUPPORTED;
    }

    ten__symbol_to_function(symbol, &fn);
    return fn();
}

void ten_plugin_call_shutdown(ten_plugin_t *plugin)
{
    void *symbol;
    ten_plugin_shutdown_fn fn = NULL;

    if (plugin == NULL)
    {
        return;
    }

    symbol = ten_plugin_symbol(plugin, "ten_plugin_shutdown");
    if (symbol != NULL)
    {
        ten__symbol_to_function(symbol, &fn);
        fn();
    }
}
