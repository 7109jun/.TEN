#ifndef TEN_TYPE_H
#define TEN_TYPE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * .TEN 사양서 16절 — C의 기본 타입을 추상화하는 runtime type descriptor.
 * 객체 타입은 제공하지 않는다.
 */
typedef enum {
    TEN_TYPE_VOID,

    TEN_TYPE_BOOL,

    TEN_TYPE_I8,
    TEN_TYPE_I16,
    TEN_TYPE_I32,
    TEN_TYPE_I64,

    TEN_TYPE_U8,
    TEN_TYPE_U16,
    TEN_TYPE_U32,
    TEN_TYPE_U64,

    TEN_TYPE_F32,
    TEN_TYPE_F64,

    TEN_TYPE_STRING,
    TEN_TYPE_BUFFER,
    TEN_TYPE_POINTER
} ten_type_t;

#ifdef __cplusplus
}
#endif

#endif /* TEN_TYPE_H */
