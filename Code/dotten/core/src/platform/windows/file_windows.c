/*
 * Windows 백엔드.
 *
 * 이 샌드박스는 Linux이므로 이 파일은 컴파일 검증을 하지 못했다.
 * CMake가 WIN32 빌드에서만 이 파일을 소스 목록에 포함시키므로
 * Linux/Android 빌드에는 전혀 관여하지 않는다. Windows에서 빌드/테스트
 * 후 문제가 있으면 알려주면 수정한다.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ten/file.h"
#include "ten/memory.h"
#include "internal/error_internal.h"
#include "internal/file_mode_internal.h"

struct ten_file {
    HANDLE handle;
};

ten_file_t *ten_file_open(const char *path, const char *mode)
{
    ten__file_mode_t m;
    DWORD access = 0;
    DWORD disposition;
    HANDLE handle;
    ten_file_t *file;

    if (path == NULL || mode == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    m = ten__parse_file_mode(mode);
    if (!m.read && !m.write)
    {
        ten__set_error(TEN_ERROR_INVALID_ARGUMENT);
        return NULL;
    }

    if (m.read)
    {
        access |= GENERIC_READ;
    }
    if (m.write)
    {
        access |= GENERIC_WRITE;
    }

    if (m.create && m.truncate)
    {
        disposition = CREATE_ALWAYS;
    }
    else if (m.create)
    {
        disposition = OPEN_ALWAYS;
    }
    else
    {
        disposition = OPEN_EXISTING;
    }

    handle = CreateFileA(
        path,
        access,
        FILE_SHARE_READ,
        NULL,
        disposition,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (handle == INVALID_HANDLE_VALUE)
    {
        ten__set_error(GetLastError() == ERROR_FILE_NOT_FOUND ? TEN_ERROR_NOT_FOUND : TEN_ERROR_IO);
        return NULL;
    }

    if (m.append)
    {
        LARGE_INTEGER zero;
        zero.QuadPart = 0;
        SetFilePointerEx(handle, zero, NULL, FILE_END);
    }

    file = (ten_file_t *)ten_malloc(sizeof(ten_file_t));
    if (file == NULL)
    {
        CloseHandle(handle);
        return NULL;
    }

    file->handle = handle;
    ten__set_error(TEN_OK);
    return file;
}

void ten_file_close(ten_file_t *file)
{
    if (file == NULL)
    {
        return;
    }

    CloseHandle(file->handle);
    ten_free(file);
}

size_t ten_file_read(ten_file_t *file, void *buffer, size_t size)
{
    DWORD read_bytes = 0;

    if (file == NULL || (buffer == NULL && size > 0))
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return 0;
    }

    if (size == 0)
    {
        ten__set_error(TEN_OK);
        return 0;
    }

    if (!ReadFile(file->handle, buffer, (DWORD)size, &read_bytes, NULL))
    {
        ten__set_error(TEN_ERROR_IO);
        return 0;
    }

    ten__set_error(TEN_OK);
    return (size_t)read_bytes;
}

size_t ten_file_write(ten_file_t *file, const void *buffer, size_t size)
{
    DWORD written = 0;

    if (file == NULL || (buffer == NULL && size > 0))
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return 0;
    }

    if (size == 0)
    {
        ten__set_error(TEN_OK);
        return 0;
    }

    if (!WriteFile(file->handle, buffer, (DWORD)size, &written, NULL))
    {
        ten__set_error(TEN_ERROR_IO);
        return 0;
    }

    ten__set_error(TEN_OK);
    return (size_t)written;
}

int ten_file_seek(ten_file_t *file, int64_t offset, int origin)
{
    LARGE_INTEGER distance;
    DWORD method;

    if (file == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    switch (origin)
    {
        case 0: method = FILE_BEGIN; break;
        case 1: method = FILE_CURRENT; break;
        case 2: method = FILE_END; break;
        default:
            ten__set_error(TEN_ERROR_INVALID_ARGUMENT);
            return (int)TEN_ERROR_INVALID_ARGUMENT;
    }

    distance.QuadPart = offset;
    if (!SetFilePointerEx(file->handle, distance, NULL, method))
    {
        ten__set_error(TEN_ERROR_IO);
        return (int)TEN_ERROR_IO;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

int64_t ten_file_size(ten_file_t *file)
{
    LARGE_INTEGER size;

    if (file == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return -1;
    }

    if (!GetFileSizeEx(file->handle, &size))
    {
        ten__set_error(TEN_ERROR_IO);
        return -1;
    }

    ten__set_error(TEN_OK);
    return (int64_t)size.QuadPart;
}

int ten_file_exists(const char *path)
{
    DWORD attrs;

    if (path == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return 0;
    }

    attrs = GetFileAttributesA(path);
    ten__set_error(TEN_OK);
    return attrs != INVALID_FILE_ATTRIBUTES;
}

int ten_file_delete(const char *path)
{
    if (path == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (!DeleteFileA(path))
    {
        DWORD err = GetLastError();
        ten_error_code_t code = (err == ERROR_FILE_NOT_FOUND) ? TEN_ERROR_NOT_FOUND : TEN_ERROR_IO;
        ten__set_error(code);
        return (int)code;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

int ten_file_copy(const char *source, const char *destination)
{
    if (source == NULL || destination == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (!CopyFileA(source, destination, FALSE))
    {
        ten__set_error(TEN_ERROR_IO);
        return (int)TEN_ERROR_IO;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

int ten_file_move(const char *source, const char *destination)
{
    if (source == NULL || destination == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (!MoveFileExA(source, destination, MOVEFILE_REPLACE_EXISTING))
    {
        ten__set_error(TEN_ERROR_IO);
        return (int)TEN_ERROR_IO;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}
