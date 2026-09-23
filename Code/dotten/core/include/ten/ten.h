#ifndef TEN_TEN_H
#define TEN_TEN_H

/*
 * .TEN 통합 헤더.
 *
 * 개별 헤더(<ten/core.h>, <ten/error.h> 등)도 직접 include 할 수 있다.
 * 아래 목록은 로드맵에 따라 모듈이 구현되는 대로 채워진다(M1: core/error).
 */

#include "error.h"
#include "core.h"
#include "memory.h"
#include "type.h"
#include "list.h"
#include "map.h"
#include "string.h"
#include "buffer.h"
#include "path.h"
#include "time.h"
#include "log.h"
#include "file.h"
#include "thread.h"
#include "sync.h"
#include "thread_pool.h"
#include "process.h"
#include "encoding.h"
#include "json.h"
#include "socket.h"
#include "http.h"
#include "plugin.h"

/* 사양서 53절 Phase 1~5 전 모듈 구현 완료. 남은 건 39~51절 횡단
   관심사(Feature Configuration 빌드 옵션, 문서화 등) 정리뿐이다. */

#endif /* TEN_TEN_H */
