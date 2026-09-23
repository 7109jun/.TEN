#ifndef TEN_INTERNAL_TYPE_INTERNAL_H
#define TEN_INTERNAL_TYPE_INTERNAL_H

#include "ten/type.h"
#include <stddef.h>

/*
 * List/Map의 element 저장 정책.
 *
 * List/Map은 값을 ten_type_t가 가리키는 크기만큼 "raw byte copy"로
 * 저장한다(고정폭 스칼라 타입은 값 자체, STRING/BUFFER/POINTER는
 * 포인터 값 자체 — 즉 참조이며 소유하지 않는다).
 *
 * 예: TEN_TYPE_STRING인 List에 ten_list_add(list, &some_char_ptr)를
 * 호출하면 sizeof(const char *) 바이트, 즉 포인터 값이 복사되어
 * 저장된다. 가리키는 문자열 자체의 수명은 호출자 책임이다.
 * (사양서 46절: "Collection의 문자열 복사/참조 정책을 명확히 하고
 *  전체 API에서 동일하게 적용해야 한다" — 이 정책을 List/Map 전체에
 *  일관되게 적용한다.)
 *
 * TEN_TYPE_VOID는 element 타입으로 허용하지 않는다(size 0 반환).
 */
static size_t ten__type_size(ten_type_t type)
{
    switch (type)
    {
        case TEN_TYPE_BOOL:
            return sizeof(unsigned char);

        case TEN_TYPE_I8:
        case TEN_TYPE_U8:
            return 1;

        case TEN_TYPE_I16:
        case TEN_TYPE_U16:
            return 2;

        case TEN_TYPE_I32:
        case TEN_TYPE_U32:
        case TEN_TYPE_F32:
            return 4;

        case TEN_TYPE_I64:
        case TEN_TYPE_U64:
        case TEN_TYPE_F64:
            return 8;

        case TEN_TYPE_STRING:
            return sizeof(const char *);

        case TEN_TYPE_BUFFER:
        case TEN_TYPE_POINTER:
            return sizeof(void *);

        case TEN_TYPE_VOID:
        default:
            return 0;
    }
}

#endif /* TEN_INTERNAL_TYPE_INTERNAL_H */
