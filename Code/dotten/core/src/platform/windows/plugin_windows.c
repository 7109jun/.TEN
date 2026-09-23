/*
 * Windows 백엔드(미검증 — thread_windows.c 상단 주석 참고).
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ten/plugin.h"
#include "ten/memory.h"
#include "internal/error_internal.h"

#include <string.h>

/* plugin_posix.c와 동일한 이유로 memcpy 기반 타입 펀닝을 쓴다. */
static void ten__symbol_to_function(void *symbol, void *out_fn_ptr)
{
    memcpy(out_fn_ptr, &symbol, sizeof(symbol));
}

struct ten_plugin {
    HMODULE handle;
};

ten_plugin_t *ten_plugin_load(const char *path)
{
    HMODULE handle;
    ten_plugin_t *plugin;

    if (path == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    handle = LoadLibraryA(path);
    if (handle == NULL)
    {
        ten__set_error(TEN_ERROR_IO);
        return NULL;
    }

    plugin = (ten_plugin_t *)ten_malloc(sizeof(ten_plugin_t));
    if (plugin == NULL)
    {
        FreeLibrary(handle);
        return NULL;
    }

    plugin->handle = handle;
    ten__set_error(TEN_OK);
    return plugin;
}

void *ten_plugin_symbol(ten_plugin_t *plugin, const char *name)
{
    FARPROC sym;
    void *result;

    if (plugin == NULL || name == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    sym = GetProcAddress(plugin->handle, name);
    if (sym == NULL)
    {
        ten__set_error(TEN_ERROR_NOT_FOUND);
        return NULL;
    }

    memcpy(&result, &sym, sizeof(sym));
    ten__set_error(TEN_OK);
    return result;
}

void ten_plugin_unload(ten_plugin_t *plugin)
{
    if (plugin == NULL)
    {
        return;
    }
    FreeLibrary(plugin->handle);
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
