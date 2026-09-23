#include "ten/core.h"

#define TEN_STR2(x) #x
#define TEN_STR(x) TEN_STR2(x)

static const char TEN_VERSION_STRING[] =
    TEN_STR(TEN_VERSION_MAJOR) "." TEN_STR(TEN_VERSION_MINOR) "." TEN_STR(TEN_VERSION_PATCH);

const char *ten_version(void)
{
    return TEN_VERSION_STRING;
}

int ten_version_major(void)
{
    return TEN_VERSION_MAJOR;
}

int ten_version_minor(void)
{
    return TEN_VERSION_MINOR;
}

int ten_version_patch(void)
{
    return TEN_VERSION_PATCH;
}
