#ifndef TEN_INTERNAL_FILE_MODE_INTERNAL_H
#define TEN_INTERNAL_FILE_MODE_INTERNAL_H

#include <string.h>

typedef struct {
    int read;
    int write;
    int append;
    int truncate;
    int create;
} ten__file_mode_t;

/*
 * fopen 스타일 mode 문자열을 해석한다. "r", "r+", "w", "w+", "a", "a+"
 * 및 그 사이에 낀 'b'를 지원한다(예: "rb", "r+b", "rb+").
 * 인식할 수 없으면 모든 필드가 0인 구조체를 반환하며, 호출자는
 * (!read && !write)로 이를 감지해 TEN_ERROR_INVALID_ARGUMENT를 설정해야
 * 한다.
 */
static ten__file_mode_t ten__parse_file_mode(const char *mode)
{
    ten__file_mode_t m;
    size_t len = strlen(mode);
    int has_plus = (len > 0 && (mode[len - 1] == '+' || (len > 1 && mode[1] == '+')));

    m.read = 0;
    m.write = 0;
    m.append = 0;
    m.truncate = 0;
    m.create = 0;

    if (len == 0)
    {
        return m;
    }

    switch (mode[0])
    {
        case 'r':
            m.read = 1;
            if (has_plus)
            {
                m.write = 1;
            }
            break;

        case 'w':
            m.write = 1;
            m.create = 1;
            m.truncate = 1;
            if (has_plus)
            {
                m.read = 1;
            }
            break;

        case 'a':
            m.write = 1;
            m.create = 1;
            m.append = 1;
            if (has_plus)
            {
                m.read = 1;
            }
            break;

        default:
            break;
    }

    return m;
}

#endif /* TEN_INTERNAL_FILE_MODE_INTERNAL_H */
