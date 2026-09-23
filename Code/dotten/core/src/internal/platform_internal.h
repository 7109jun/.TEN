#ifndef TEN_INTERNAL_PLATFORM_INTERNAL_H
#define TEN_INTERNAL_PLATFORM_INTERNAL_H

/*
 * C99 표준에는 thread-local storage가 없다(_Thread_local은 C11부터).
 * 사양서 13절은 오류 상태를 스레드별로 분리할 것을 요구하므로,
 * 공식 지원 플랫폼(Windows/Linux/Android)의 주력 컴파일러가 제공하는
 * 확장 키워드를 통해 이를 구현한다.
 *
 * 매크로가 정의되지 않는 예외적인 컴파일러에서는 TEN_THREAD_LOCAL이
 * 비워지며, 그 경우 오류 상태는 스레드 간에 공유된다(NOT_THREAD_SAFE로
 * 저하됨). 이는 공식 지원 3플랫폼(MSVC/GCC/Clang)에서는 발생하지 않는다.
 */
#if defined(_MSC_VER)
#define TEN_THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
#define TEN_THREAD_LOCAL __thread
#else
#define TEN_THREAD_LOCAL
#endif

#endif /* TEN_INTERNAL_PLATFORM_INTERNAL_H */
