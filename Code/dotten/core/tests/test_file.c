#include <assert.h>
#include <string.h>
#include <ten/ten.h>

#define TEST_FILE_A "ten_test_file_a.bin"
#define TEST_FILE_B "ten_test_file_b.bin"
#define MISSING_FILE "ten_test_missing_file.bin"

int main(void)
{
    ten_file_t *f;
    char buf[64];
    size_t n;

    /* 정리(이전 실행 잔재 제거) — 존재하지 않아도 상관없다 */
    ten_file_delete(TEST_FILE_A);
    ten_file_delete(TEST_FILE_B);
    ten_file_delete(MISSING_FILE);

    /* 없는 파일을 읽기 모드로 열면 실패해야 한다 */
    assert(ten_file_open(MISSING_FILE, "r") == NULL);
    assert(ten_last_error() == TEN_ERROR_NOT_FOUND);
    assert(ten_file_exists(MISSING_FILE) == 0);

    /* 쓰기 -> 닫기 -> 다시 읽기 */
    f = ten_file_open(TEST_FILE_A, "wb");
    assert(f != NULL);
    assert(ten_file_write(f, "Hello, .TEN!", 12) == 12);
    ten_file_close(f);

    assert(ten_file_exists(TEST_FILE_A) == 1);

    f = ten_file_open(TEST_FILE_A, "rb");
    assert(f != NULL);
    assert(ten_file_size(f) == 12);

    n = ten_file_read(f, buf, sizeof(buf));
    assert(n == 12);
    assert(memcmp(buf, "Hello, .TEN!", 12) == 0);

    /* EOF: 더 읽으면 0바이트, 오류는 아니어야 한다 */
    n = ten_file_read(f, buf, sizeof(buf));
    assert(n == 0);
    assert(ten_last_error() == TEN_OK);

    /* seek 후 재조회 */
    assert(ten_file_seek(f, 7, 0) == TEN_OK); /* SEEK_SET */
    n = ten_file_read(f, buf, 4);
    assert(n == 4);
    assert(memcmp(buf, ".TEN", 4) == 0);

    ten_file_close(f);

    /* 잘못된 origin */
    f = ten_file_open(TEST_FILE_A, "rb");
    assert(ten_file_seek(f, 0, 99) == (int)TEN_ERROR_INVALID_ARGUMENT);
    ten_file_close(f);

    /* copy */
    assert(ten_file_copy(TEST_FILE_A, TEST_FILE_B) == TEN_OK);
    f = ten_file_open(TEST_FILE_B, "rb");
    assert(f != NULL);
    assert(ten_file_size(f) == 12);
    n = ten_file_read(f, buf, sizeof(buf));
    assert(n == 12);
    assert(memcmp(buf, "Hello, .TEN!", 12) == 0);
    ten_file_close(f);

    /* move */
    assert(ten_file_move(TEST_FILE_B, MISSING_FILE) == TEN_OK); /* 이름 재사용 */
    assert(ten_file_exists(TEST_FILE_B) == 0);
    assert(ten_file_exists(MISSING_FILE) == 1);

    /* delete */
    assert(ten_file_delete(TEST_FILE_A) == TEN_OK);
    assert(ten_file_delete(MISSING_FILE) == TEN_OK);
    assert(ten_file_delete(TEST_FILE_A) == (int)TEN_ERROR_NOT_FOUND);

    /* NULL 인자 */
    assert(ten_file_open(NULL, "r") == NULL);
    assert(ten_last_error() == TEN_ERROR_NULL_ARGUMENT);
    assert(ten_file_open(TEST_FILE_A, NULL) == NULL);
    assert(ten_file_write(NULL, "x", 1) == 0);
    assert(ten_last_error() == TEN_ERROR_NULL_ARGUMENT);
    assert(ten_file_delete(NULL) == (int)TEN_ERROR_NULL_ARGUMENT);
    assert(ten_file_exists(NULL) == 0);

    /* 잘못된 mode 문자열 */
    assert(ten_file_open(TEST_FILE_A, "x") == NULL);
    assert(ten_last_error() == TEN_ERROR_INVALID_ARGUMENT);

    /* close(NULL)은 안전해야 한다 */
    ten_file_close(NULL);

    return 0;
}
