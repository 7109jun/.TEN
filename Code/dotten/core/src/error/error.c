#include "ten/error.h"
#include "internal/error_internal.h"
#include "internal/platform_internal.h"

static TEN_THREAD_LOCAL ten_error_code_t g_ten_last_error = TEN_OK;

void ten__set_error(ten_error_code_t code)
{
    g_ten_last_error = code;
}

ten_error_code_t ten_last_error(void)
{
    return g_ten_last_error;
}

void ten_clear_error(void)
{
    g_ten_last_error = TEN_OK;
}

const char *ten_error_message(ten_error_code_t code)
{
    switch (code)
    {
        case TEN_OK:                        return "No error";
        case TEN_ERROR_UNKNOWN:             return "Unknown error";
        case TEN_ERROR_INVALID_ARGUMENT:    return "Invalid argument";
        case TEN_ERROR_NULL_ARGUMENT:       return "Null argument";
        case TEN_ERROR_OUT_OF_MEMORY:       return "Out of memory";
        case TEN_ERROR_OUT_OF_RANGE:        return "Out of range";
        case TEN_ERROR_NOT_FOUND:           return "Not found";
        case TEN_ERROR_ALREADY_EXISTS:      return "Already exists";
        case TEN_ERROR_ACCESS_DENIED:       return "Access denied";
        case TEN_ERROR_NOT_SUPPORTED:       return "Not supported";
        case TEN_ERROR_INVALID_STATE:       return "Invalid state";
        case TEN_ERROR_TIMEOUT:             return "Timeout";
        case TEN_ERROR_CANCELLED:           return "Cancelled";
        case TEN_ERROR_IO:                  return "I/O error";
        case TEN_ERROR_NETWORK:             return "Network error";
        case TEN_ERROR_PARSE:               return "Parse error";
        case TEN_ERROR_ENCODING:            return "Encoding error";
        case TEN_ERROR_SYSTEM:              return "System error";
        case TEN_ERROR_ALREADY_INITIALIZED: return "Already initialized";
        case TEN_ERROR_NOT_INITIALIZED:     return "Not initialized";
        default:                            return "Unrecognized error code";
    }
}
