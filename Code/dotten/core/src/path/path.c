#include "ten/path.h"
#include "ten/memory.h"
#include "internal/error_internal.h"

#include <string.h>

#ifdef _WIN32
#define TEN_PATH_SEPARATOR '\\'
#else
#define TEN_PATH_SEPARATOR '/'
#endif

static int ten__is_sep(char c)
{
    return c == '/' || c == '\\';
}

/* path 안에서 마지막 separator를 찾는다. 없으면 NULL. */
static const char *ten__find_last_sep(const char *path)
{
    const char *last = NULL;
    const char *p;

    for (p = path; *p != '\0'; p++)
    {
        if (ten__is_sep(*p))
        {
            last = p;
        }
    }

    return last;
}

static char *ten__dup_range(const char *start, size_t len)
{
    char *result = (char *)ten_malloc(len + 1);
    if (result == NULL)
    {
        return NULL;
    }

    memcpy(result, start, len);
    result[len] = '\0';
    return result;
}

char *ten_path_join(const char *a, const char *b)
{
    size_t len_a;
    size_t len_b;
    int need_sep;
    char *result;

    if (a == NULL || b == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    len_a = strlen(a);
    len_b = strlen(b);

    /* a의 트레일링 separator, b의 leading separator는 중복되지 않도록
       하나만 남긴다. */
    while (len_a > 0 && ten__is_sep(a[len_a - 1]))
    {
        len_a--;
    }
    while (len_b > 0 && ten__is_sep(b[0]))
    {
        b++;
        len_b--;
    }

    need_sep = (len_a > 0 && len_b > 0) ? 1 : 0;

    result = (char *)ten_malloc(len_a + (size_t)need_sep + len_b + 1);
    if (result == NULL)
    {
        return NULL;
    }

    memcpy(result, a, len_a);
    if (need_sep)
    {
        result[len_a] = TEN_PATH_SEPARATOR;
    }
    memcpy(result + len_a + (size_t)need_sep, b, len_b);
    result[len_a + (size_t)need_sep + len_b] = '\0';

    ten__set_error(TEN_OK);
    return result;
}

char *ten_path_directory(const char *path)
{
    const char *sep;

    if (path == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    sep = ten__find_last_sep(path);
    ten__set_error(TEN_OK);

    if (sep == NULL)
    {
        return ten__dup_range(".", 1);
    }

    if (sep == path)
    {
        /* "/foo" -> "/" */
        return ten__dup_range(path, 1);
    }

    return ten__dup_range(path, (size_t)(sep - path));
}

char *ten_path_filename(const char *path)
{
    const char *sep;

    if (path == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    sep = ten__find_last_sep(path);
    ten__set_error(TEN_OK);

    if (sep == NULL)
    {
        return ten__dup_range(path, strlen(path));
    }

    return ten__dup_range(sep + 1, strlen(sep + 1));
}

char *ten_path_extension(const char *path)
{
    char *filename;
    char *dot;
    char *result;

    if (path == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    filename = ten_path_filename(path);
    if (filename == NULL)
    {
        return NULL;
    }

    /* 숨김 파일(".gitignore")처럼 이름이 '.'로 시작하는 경우는
       확장자가 없는 것으로 취급한다. */
    dot = strrchr(filename + (filename[0] == '.' ? 1 : 0), '.');

    if (dot == NULL)
    {
        ten_free(filename);
        ten__set_error(TEN_OK);
        return ten__dup_range("", 0);
    }

    result = ten__dup_range(dot, strlen(dot));
    ten_free(filename);
    ten__set_error(TEN_OK);
    return result;
}

int ten_path_is_absolute(const char *path)
{
    if (path == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return 0;
    }

    ten__set_error(TEN_OK);

#ifdef _WIN32
    if (ten__is_sep(path[0]))
    {
        return 1; /* "\\foo" 또는 UNC "\\\\server\\share" */
    }
    if (((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) &&
        path[1] == ':' && ten__is_sep(path[2]))
    {
        return 1; /* "C:\\foo" */
    }
    return 0;
#else
    return path[0] == '/';
#endif
}
