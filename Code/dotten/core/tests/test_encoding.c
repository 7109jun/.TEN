#include <assert.h>
#include <string.h>
#include <ten/ten.h>

static void test_base64(void)
{
    char *encoded;
    void *decoded;
    size_t decoded_size;

    /* RFC 4648 예시: "Man" -> "TWFu" */
    encoded = ten_base64_encode("Man", 3);
    assert(encoded != NULL);
    assert(strcmp(encoded, "TWFu") == 0);
    ten_free(encoded);

    /* 패딩 케이스: "Ma" -> "TWE=" */
    encoded = ten_base64_encode("Ma", 2);
    assert(strcmp(encoded, "TWE=") == 0);
    ten_free(encoded);

    /* 왕복 */
    encoded = ten_base64_encode("Hello, .TEN!", 12);
    decoded = ten_base64_decode(encoded, &decoded_size);
    assert(decoded != NULL);
    assert(decoded_size == 12);
    assert(memcmp(decoded, "Hello, .TEN!", 12) == 0);
    ten_free(encoded);
    ten_free(decoded);

    /* 빈 입력 */
    encoded = ten_base64_encode(NULL, 0);
    assert(encoded != NULL);
    assert(strcmp(encoded, "") == 0);
    ten_free(encoded);

    /* 잘못된 입력 */
    assert(ten_base64_decode("abc", &decoded_size) == NULL); /* 4의 배수 아님 */
    assert(ten_last_error() == TEN_ERROR_ENCODING);
    assert(ten_base64_decode("ab!=", &decoded_size) == NULL); /* 잘못된 문자 */
    assert(ten_last_error() == TEN_ERROR_ENCODING);

    /* NULL 인자 */
    assert(ten_base64_encode(NULL, 4) == NULL);
    assert(ten_last_error() == TEN_ERROR_NULL_ARGUMENT);
    assert(ten_base64_decode(NULL, &decoded_size) == NULL);
    assert(ten_last_error() == TEN_ERROR_NULL_ARGUMENT);
}

static void test_hex(void)
{
    char *encoded;
    void *decoded;
    size_t decoded_size;

    encoded = ten_hex_encode("\x01\xAB\xFF", 3);
    assert(encoded != NULL);
    assert(strcmp(encoded, "01abff") == 0);
    ten_free(encoded);

    decoded = ten_hex_decode("01abff", &decoded_size);
    assert(decoded != NULL);
    assert(decoded_size == 3);
    assert(memcmp(decoded, "\x01\xAB\xFF", 3) == 0);
    ten_free(decoded);

    /* 대문자도 허용 */
    decoded = ten_hex_decode("01ABFF", &decoded_size);
    assert(decoded != NULL);
    assert(memcmp(decoded, "\x01\xAB\xFF", 3) == 0);
    ten_free(decoded);

    /* 잘못된 입력 */
    assert(ten_hex_decode("abc", &decoded_size) == NULL); /* 홀수 길이 */
    assert(ten_last_error() == TEN_ERROR_ENCODING);
    assert(ten_hex_decode("zz", &decoded_size) == NULL); /* 16진수 아님 */
    assert(ten_last_error() == TEN_ERROR_ENCODING);
}

static void test_utf_conversion(void)
{
    const char *utf8_input = "A\xea\xb0\x80\xf0\x9f\x98\x80"; /* 'A', 가(U+AC00), 😀(U+1F600) */
    uint16_t *utf16;
    size_t utf16_len;
    char *back;

    utf16 = ten_utf8_to_utf16(utf8_input, &utf16_len);
    assert(utf16 != NULL);
    /* 'A'(1) + 가(1) + 😀(surrogate pair, 2) = 4 코드 유닛 */
    assert(utf16_len == 4);
    assert(utf16[0] == 0x0041);
    assert(utf16[1] == 0xAC00);
    assert(utf16[2] == 0xD83D); /* high surrogate */
    assert(utf16[3] == 0xDE00); /* low surrogate */

    back = ten_utf16_to_utf8(utf16, utf16_len);
    assert(back != NULL);
    assert(strcmp(back, utf8_input) == 0);

    ten_free(utf16);
    ten_free(back);

    /* 잘못된 UTF-8 */
    assert(ten_utf8_to_utf16("\xff\xfe", &utf16_len) == NULL);
    assert(ten_last_error() == TEN_ERROR_ENCODING);

    /* 짝 없는 surrogate */
    {
        uint16_t bad[1];
        bad[0] = 0xD800;
        assert(ten_utf16_to_utf8(bad, 1) == NULL);
        assert(ten_last_error() == TEN_ERROR_ENCODING);
    }
}

static void test_validation(void)
{
    assert(ten_ascii_validate("Hello", 5) == 1);
    assert(ten_ascii_validate("Hell\xC3\xB6", 6) == 0); /* 'ö' 포함 */

    assert(ten_utf8_validate("Hello \xea\xb0\x80", 9) == 1);
    assert(ten_utf8_validate("\xff\xfe", 2) == 0);
}

int main(void)
{
    test_base64();
    test_hex();
    test_utf_conversion();
    test_validation();
    return 0;
}
