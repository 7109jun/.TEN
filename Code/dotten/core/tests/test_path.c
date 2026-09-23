#include <assert.h>
#include <string.h>
#include <ten/ten.h>

static void check(char *actual, const char *expected)
{
    assert(actual != NULL);
    assert(strcmp(actual, expected) == 0);
    ten_free(actual);
}

int main(void)
{
    /* join */
    check(ten_path_join("a", "b"), "a" "/" "b");
    check(ten_path_join("a/", "b"), "a/b");
    check(ten_path_join("a", "/b"), "a/b");
    check(ten_path_join("a/", "/b"), "a/b");
    check(ten_path_join("", "b"), "b");
    check(ten_path_join("a", ""), "a");

    /* directory / filename */
    check(ten_path_directory("/usr/local/bin"), "/usr/local");
    check(ten_path_directory("noslash"), ".");
    check(ten_path_directory("/root"), "/");

    check(ten_path_filename("/usr/local/bin"), "bin");
    check(ten_path_filename("noslash"), "noslash");
    check(ten_path_filename("/trailing/"), "");

    /* extension */
    check(ten_path_extension("archive.tar.gz"), ".gz");
    check(ten_path_extension("noext"), "");
    check(ten_path_extension(".gitignore"), "");        /* 숨김 파일은 확장자 없음 */
    check(ten_path_extension("/path/to/file.txt"), ".txt");

    /* is_absolute (POSIX 빌드 기준) */
    assert(ten_path_is_absolute("/usr/bin") == 1);
    assert(ten_path_is_absolute("relative/path") == 0);

    /* NULL 인자 */
    assert(ten_path_join(NULL, "b") == NULL);
    assert(ten_last_error() == TEN_ERROR_NULL_ARGUMENT);
    assert(ten_path_directory(NULL) == NULL);
    assert(ten_path_filename(NULL) == NULL);
    assert(ten_path_extension(NULL) == NULL);
    assert(ten_path_is_absolute(NULL) == 0);
    assert(ten_last_error() == TEN_ERROR_NULL_ARGUMENT);

    return 0;
}
