#if !defined(_POSIX_C_SOURCE)
/* -std=c99(strict)에서 POSIX API를 노출시키기 위함. */
#define _POSIX_C_SOURCE 200809L
#endif

#include "ten/file.h"
#include "ten/memory.h"
#include "internal/error_internal.h"
#include "internal/file_mode_internal.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

struct ten_file {
    int fd;
};

ten_file_t *ten_file_open(const char *path, const char *mode)
{
    ten__file_mode_t m;
    int flags;
    int fd;
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

    if (m.read && m.write)
    {
        flags = O_RDWR;
    }
    else if (m.write)
    {
        flags = O_WRONLY;
    }
    else
    {
        flags = O_RDONLY;
    }

    if (m.create)
    {
        flags |= O_CREAT;
    }
    if (m.truncate)
    {
        flags |= O_TRUNC;
    }
    if (m.append)
    {
        flags |= O_APPEND;
    }

    fd = open(path, flags, 0644);
    if (fd < 0)
    {
        ten__set_error(errno == ENOENT ? TEN_ERROR_NOT_FOUND : TEN_ERROR_IO);
        return NULL;
    }

    file = (ten_file_t *)ten_malloc(sizeof(ten_file_t));
    if (file == NULL)
    {
        close(fd);
        return NULL;
    }

    file->fd = fd;
    ten__set_error(TEN_OK);
    return file;
}

void ten_file_close(ten_file_t *file)
{
    if (file == NULL)
    {
        return;
    }

    close(file->fd);
    ten_free(file);
}

size_t ten_file_read(ten_file_t *file, void *buffer, size_t size)
{
    ssize_t n;

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

    n = read(file->fd, buffer, size);
    if (n < 0)
    {
        ten__set_error(TEN_ERROR_IO);
        return 0;
    }

    ten__set_error(TEN_OK);
    return (size_t)n;
}

size_t ten_file_write(ten_file_t *file, const void *buffer, size_t size)
{
    ssize_t n;

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

    n = write(file->fd, buffer, size);
    if (n < 0)
    {
        ten__set_error(TEN_ERROR_IO);
        return 0;
    }

    ten__set_error(TEN_OK);
    return (size_t)n;
}

int ten_file_seek(ten_file_t *file, int64_t offset, int origin)
{
    int whence;
    off_t result;

    if (file == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    switch (origin)
    {
        case 0: whence = SEEK_SET; break;
        case 1: whence = SEEK_CUR; break;
        case 2: whence = SEEK_END; break;
        default:
            ten__set_error(TEN_ERROR_INVALID_ARGUMENT);
            return (int)TEN_ERROR_INVALID_ARGUMENT;
    }

    result = lseek(file->fd, (off_t)offset, whence);
    if (result == (off_t)-1)
    {
        ten__set_error(TEN_ERROR_IO);
        return (int)TEN_ERROR_IO;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

int64_t ten_file_size(ten_file_t *file)
{
    struct stat st;

    if (file == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return -1;
    }

    if (fstat(file->fd, &st) != 0)
    {
        ten__set_error(TEN_ERROR_IO);
        return -1;
    }

    ten__set_error(TEN_OK);
    return (int64_t)st.st_size;
}

int ten_file_exists(const char *path)
{
    struct stat st;

    if (path == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return 0;
    }

    ten__set_error(TEN_OK);
    return stat(path, &st) == 0;
}

int ten_file_delete(const char *path)
{
    if (path == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (unlink(path) != 0)
    {
        ten_error_code_t code = (errno == ENOENT) ? TEN_ERROR_NOT_FOUND : TEN_ERROR_IO;
        ten__set_error(code);
        return (int)code;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

int ten_file_copy(const char *source, const char *destination)
{
    ten_file_t *src;
    ten_file_t *dst;
    unsigned char buf[8192];
    int rc = (int)TEN_OK;

    if (source == NULL || destination == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    src = ten_file_open(source, "rb");
    if (src == NULL)
    {
        return (int)ten_last_error();
    }

    dst = ten_file_open(destination, "wb");
    if (dst == NULL)
    {
        rc = (int)ten_last_error();
        ten_file_close(src);
        return rc;
    }

    for (;;)
    {
        size_t n = ten_file_read(src, buf, sizeof(buf));
        if (n == 0)
        {
            if (ten_last_error() != TEN_OK)
            {
                rc = (int)TEN_ERROR_IO;
            }
            break;
        }

        if (ten_file_write(dst, buf, n) != n)
        {
            rc = (int)TEN_ERROR_IO;
            break;
        }
    }

    ten_file_close(src);
    ten_file_close(dst);

    ten__set_error((ten_error_code_t)rc);
    return rc;
}

int ten_file_move(const char *source, const char *destination)
{
    if (source == NULL || destination == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    if (rename(source, destination) != 0)
    {
        ten__set_error(TEN_ERROR_IO);
        return (int)TEN_ERROR_IO;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}
