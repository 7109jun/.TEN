# .TEN Guide

> One API. Native C. Everywhere.

This document is a guide for people encountering the `.TEN` library for the first time. It provides an overview of the design philosophy, module APIs, build process, Rust bindings, and current implementation status in one place.

If the official specification (`.TEN Specification`) defines **"what should be built,"** this guide explains **"what has actually been built and how to use it."**

---

## Table of Contents

1. [.TEN Overview](#1-ten-overview)
2. [Design Philosophy](#2-design-philosophy)
3. [Platform Support](#3-platform-support)
4. [Repository Structure](#4-repository-structure)
5. [Building](#5-building)
6. [Common Rules: Naming · Ownership · Error Handling](#6-common-rules-naming--ownership--error-handling)
7. [Module Reference](#7-module-reference)

   * [7.1 Core](#71-core)
   * [7.2 Memory](#72-memory)
   * [7.3 Error](#73-error)
   * [7.4 Type](#74-type)
   * [7.5 List](#75-list)
   * [7.6 Map](#76-map)
   * [7.7 String](#77-string)
   * [7.8 Buffer](#78-buffer)
   * [7.9 Path](#79-path)
   * [7.10 Time](#710-time)
   * [7.11 Logging](#711-logging)
   * [7.12 File](#712-file)
   * [7.13 Thread](#713-thread)
   * [7.14 Synchronization](#714-synchronization)
   * [7.15 Thread Pool](#715-thread-pool)
   * [7.16 Process](#716-process)
   * [7.17 Encoding](#717-encoding)
   * [7.18 JSON](#718-json)
   * [7.19 Socket](#719-socket)
   * [7.20 HTTP](#720-http)
   * [7.21 Plugin](#721-plugin)
8. [Rust Bindings](#8-rust-bindings)
9. [Testing & Verification](#9-testing--verification)
10. [Known Limitations and Remaining Work](#10-known-limitations-and-remaining-work)

---

## 1. .TEN Overview

`.TEN` is a **cross-platform unified C library with C99 as its primary language**.

It officially supports Windows, Linux, and Android. Its goal is to hide differences between operating system APIs and provide the following through a single consistent C API:

* Memory management / data structures (List, Map) / strings / binary buffers
* File and path handling
* Threads and synchronization / thread pools
* Processes, time, and logging
* Networking (Socket, HTTP)
* Data processing (JSON, Encoding)
* Dynamic library loading (Plugin)

It is inspired by the unified library design philosophy of .NET, but `.TEN` itself does not implement the .NET runtime or an object-oriented framework. It operates as a pure native C library and does not require a heavy runtime such as a GC or VM.

## 2. Design Philosophy

| Principle               | Description                                                                                                                                                                       |
| ----------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **C99 First**           | All core functionality compiles using C99 alone. It does not use C11/C17/C23-only features, C++, STL, exceptions, RTTI, or templates.                                             |
| **C++ Compatibility**   | C++ programs can use `.TEN`'s C API directly, but C++-specific wrappers are not included in the core library.                                                                     |
| **Non-Object-Oriented** | Instead of Classes/Inheritance/Polymorphism/Virtual Functions, `.TEN` uses `struct` + `typedef` + `enum` + function pointers + opaque structures.                                 |
| **Lightweight Runtime** | No VM, JIT, GC, CLR, Managed Heap, or Exceptions.                                                                                                                                 |
| **ABI Stability**       | All public composite data structures are opaque types (`typedef struct ten_x ten_x_t;`). Internal implementations can therefore change while maintaining a stable public API/ABI. |
| **Actually Functional** | Mocks and placeholders are not treated as real functionality. There are no public APIs that are merely declared without implementation.                                           |

## 3. Platform Support

| Platform | Architecture            | Status                                                                                  |
| -------- | ----------------------- | --------------------------------------------------------------------------------------- |
| Windows  | x86, x86_64             | Code complete, **not actually build-verified** (MSVC/MinGW local verification required) |
| Linux    | x86, x86_64, ARM, ARM64 | **Actually verified** with GCC + AddressSanitizer/UBSan/ThreadSanitizer                 |
| Android  | ARM, ARM64, x86_64      | Designed to share the POSIX backend, **NDK build not actually verified**                |

macOS/iOS are not officially supported.

Internally, platform-specific implementations are separated under `core/src/platform/{posix,windows}/`.

The POSIX backend is shared by Linux and Android because both provide POSIX-compatible APIs.

## 4. Repository Structure

```text
.
├── core/                       # Official .TEN implementation (C99, CMake)
│   ├── include/ten/            # Public headers (unified header: ten.h)
│   ├── src/
│   │   ├── core/  error/  memory/  list/  map/  string/  buffer/
│   │   ├── path/  time/  log/  thread_pool/  encoding/  json/  http/
│   │   ├── platform/
│   │   │   ├── posix/          # file/thread/sync/process/socket/plugin
│   │   │   └── windows/        # same modules, based on Win32 APIs
│   │   └── internal/           # Private headers (error state, type sizes, etc.)
│   ├── examples/hello.c
│   └── tests/                   # 20 module tests + plugin fixture
├── bindings/
│   └── rust/
│       ├── ten-sys/             # Raw FFI bindings (unsafe, 1:1)
│       └── ten/                 # Safe wrapper crate
├── CMakeLists.txt               # Top-level entry point
└── Doxyfile                    # API documentation generation configuration
```

## 5. Building

### C Library

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
./build/core/examples/ten_hello
```

Options:

```bash
# Build a shared library instead of a static library (libten.so / ten.dll)
cmake -S . -B build -DTEN_BUILD_SHARED=ON

# Build only the library, without examples/tests
cmake -S . -B build -DTEN_BUILD_EXAMPLES=OFF -DTEN_BUILD_TESTS=OFF
```

### Using the Library

```c
#include <ten/ten.h>   // Unified header — includes all modules

// Or include only what is needed
#include <ten/list.h>
#include <ten/json.h>
```

Link with CMake:

```cmake
add_subdirectory(path/to/dotten)
target_link_libraries(my_app PRIVATE ten)
```

### API Documentation

```bash
doxygen Doxyfile     # Generates docs/html/index.html
```

### Using .TEN from Rust

```bash
cd bindings/rust
cargo run --example hello -p ten
```

`ten-sys` automatically configures and builds `core/CMakeLists.txt` through `build.rs` and statically links the library, so the C library does not need to be built separately beforehand.

CMake and a C compiler must still be available on the system.

## 6. Common Rules: Naming · Ownership · Error Handling

### Naming

Functions use `ten_<module>_<operation>`, types use `ten_<name>_t`, and constants use `TEN_<NAME>`.

```c
ten_list_add()      ten_map_get()      ten_log_info()
ten_list_t          ten_map_t          ten_string_t
TEN_OK              TEN_TYPE_STRING    TEN_LOG_ERROR
```

### Ownership

There are only two basic rules:

```text
create/open/load/parse  → the caller owns the result
destroy/close/unload    → releases the resource
```

Raw memory directly allocated by `ten_malloc`—for example, the return value of `ten_path_join` or `ten_json_stringify`—must be released with `ten_free()`.

Each header explicitly documents ownership under an `"Ownership:"` section in its function comments.

### Error Handling

Most functions return an `int`. On success, they return `TEN_OK` (`0`).

The reason for a failure is stored in thread-local state and can be retrieved with `ten_last_error()`.

Because C99 does not provide standard TLS, the implementation uses `__thread` / `__declspec(thread)`.

```c
if (ten_list_add(list, &value) != TEN_OK) {
    fprintf(stderr, "Error: %s\n", ten_error_message(ten_last_error()));
}
```

Pointer-returning creation functions return `NULL` on failure and store the reason in `ten_last_error()`.

The library does not call `abort()` because of invalid arguments.

### Thread Safety

Collections (`List`/`Map`) do not perform internal locking by default in order to avoid unnecessary synchronization overhead.

When the same instance is accessed from multiple threads, the caller must synchronize access using something such as `ten_mutex_t`.

Each header documents functions/modules as `THREAD_SAFE`, `THREAD_COMPATIBLE`, or `NOT_THREAD_SAFE`.

---

## 7. Module Reference

### 7.1 Core

Initialization and version management.

Every program starts with `ten_init()` and ends with `ten_shutdown()`.

```c
#include <ten/ten.h>

int main(void) {
    if (ten_init() != TEN_OK) return 1;
    printf(".TEN %s\n", ten_version());   // "1.0.0"
    ten_shutdown();
    return 0;
}
```

`ten_init()` is safe to call repeatedly. If the library is already initialized, it returns `TEN_ERROR_ALREADY_INITIALIZED` rather than crashing.

### 7.2 Memory

Basic APIs wrapping `malloc`/`calloc`/`realloc`/`free`, plus support for replacing the allocator with a custom allocator.

```c
void *p = ten_malloc(128);
ten_free(p);

char *copy = ten_memdup(src, len);   // Create a copy

ten_allocator_t my_alloc = { my_malloc, my_calloc, my_realloc, my_free };
ten_set_allocator(&my_alloc);        // Subsequent .TEN allocations use this
```

Dynamically allocated memory created by `.TEN` can always be released with `ten_free()`.

### 7.3 Error

```c
typedef enum {
    TEN_OK = 0,
    TEN_ERROR_UNKNOWN, TEN_ERROR_INVALID_ARGUMENT, TEN_ERROR_NULL_ARGUMENT,
    TEN_ERROR_OUT_OF_MEMORY, TEN_ERROR_OUT_OF_RANGE, TEN_ERROR_NOT_FOUND,
    TEN_ERROR_ALREADY_EXISTS, TEN_ERROR_ACCESS_DENIED, TEN_ERROR_NOT_SUPPORTED,
    TEN_ERROR_INVALID_STATE, TEN_ERROR_TIMEOUT, TEN_ERROR_CANCELLED,
    TEN_ERROR_IO, TEN_ERROR_NETWORK, TEN_ERROR_PARSE, TEN_ERROR_ENCODING,
    TEN_ERROR_SYSTEM, TEN_ERROR_ALREADY_INITIALIZED, TEN_ERROR_NOT_INITIALIZED
} ten_error_code_t;

ten_error_code_t ten_last_error(void);
const char      *ten_error_message(ten_error_code_t code);
void              ten_clear_error(void);
```

The error state is independent for each thread.

Almost every library function updates this state regardless of whether the call succeeds or fails.

### 7.4 Type

Runtime type descriptors representing C's basic types.

They are used by List/Map to specify element types.

```c
typedef enum {
    TEN_TYPE_VOID, TEN_TYPE_BOOL,
    TEN_TYPE_I8, TEN_TYPE_I16, TEN_TYPE_I32, TEN_TYPE_I64,
    TEN_TYPE_U8, TEN_TYPE_U16, TEN_TYPE_U32, TEN_TYPE_U64,
    TEN_TYPE_F32, TEN_TYPE_F64,
    TEN_TYPE_STRING, TEN_TYPE_BUFFER, TEN_TYPE_POINTER
} ten_type_t;
```

There are no object types.

This module provides type tags only.

### 7.5 List

Dynamic array.

Fixed-width scalar types such as `TEN_TYPE_I32` store values directly. `TEN_TYPE_STRING`, `TEN_TYPE_BUFFER`, and `TEN_TYPE_POINTER` store pointer values (references). The lifetime of referenced objects remains the caller's responsibility.

```c
ten_list_t *list = ten_list_create(TEN_TYPE_STRING);

const char *hello = "Hello";
ten_list_add(list, &hello);                 // Pass the address of the pointer

const char *v = *(const char **)ten_list_get(list, 0);

ten_list_insert(list, 0, &hello);
ten_list_remove_at(list, 0);
size_t n = ten_list_count(list);

ten_list_destroy(list);
```

Performance:

* Indexed access: `O(1)`
* Amortized append: `O(1)`
* Insert/remove: `O(n)`

### 7.6 Map

Chaining-based hash table.

**`TEN_TYPE_STRING` keys are hashed and compared by the contents of the referenced string, not by the pointer itself.**

This makes normal string-key usage possible. Unlike List, this case receives special treatment. Other types use raw-byte comparison.

```c
ten_map_t *map = ten_map_create(TEN_TYPE_STRING, TEN_TYPE_I32);

const char *key = "answer";
int value = 42;
ten_map_set(map, &key, &value);

int *found = (int *)ten_map_get(map, &key);   // 42
ten_map_contains(map, &key);                  // 1
ten_map_remove(map, &key);

ten_map_destroy(map);
```

Average performance:

* Lookup: `O(1)`
* Insert: `O(1)`
* Remove: `O(1)`

When the load factor exceeds `0.75`, the bucket count is doubled and the table is rehashed.

### 7.7 String

Dynamically allocated, NUL-terminated UTF-8 string.

```c
ten_string_t *s = ten_string_create("Hello");
ten_string_append(s, ", .TEN!");
printf("%s (%zu)\n", ten_string_cstr(s), ten_string_length(s));

ten_string_contains(s, "TEN");                 // 1
ten_string_t *sub = ten_string_substring(s, 0, 5);   // "Hello"

ten_string_destroy(sub);
ten_string_destroy(s);
```

### 7.8 Buffer

Dynamic buffer for binary data with capacity doubling.

```c
ten_buffer_t *buf = ten_buffer_create();
ten_buffer_write(buf, "hello", 5);
ten_buffer_write(buf, " world", 6);

void *data = ten_buffer_data(buf);
size_t size = ten_buffer_size(buf);

ten_buffer_clear(buf);      // Sets size to 0 while preserving capacity
ten_buffer_destroy(buf);
```

### 7.9 Path

String path composition/decomposition utilities.

Composition uses the platform separator (`\` on Windows / `/` elsewhere), while decomposition recognizes both styles.

```c
char *joined = ten_path_join("a", "b");        // "a/b"
char *dir    = ten_path_directory("/usr/local/bin");  // "/usr/local"
char *name   = ten_path_filename("/usr/local/bin");   // "bin"
char *ext    = ten_path_extension("archive.tar.gz");  // ".gz"
int abs      = ten_path_is_absolute("/usr/bin");      // 1

ten_free(joined);
ten_free(dir);
ten_free(name);
ten_free(ext);
```

### 7.10 Time

```c
uint64_t now_ms       = ten_time_now_ms();          // Epoch-based ms
uint64_t now_us       = ten_time_now_us();          // Epoch-based us
uint64_t mono_ms      = ten_time_monotonic_ms();    // For elapsed-time measurement
int64_t  unix_seconds = ten_time_unix();            // Epoch-based seconds
```

POSIX uses `clock_gettime`; Windows uses `FILETIME` / `GetTickCount64`.

### 7.11 Logging

Six levels from TRACE to FATAL, with simultaneous Console/File/Callback output.

```c
ten_log_set_level(TEN_LOG_INFO);          // Filter out levels below INFO
ten_log_set_file("app.log");              // Add file output in append mode
ten_log_set_callback(my_callback, ctx);   // Invoke callback for each log

ten_log_info("Server started, port=%d", 8080);
ten_log_error("Connection failed: %s", reason);
```

### 7.12 File

Platform-independent file I/O.

To safely handle 64-bit offsets and sizes, `.TEN` directly uses OS-level APIs instead of stdio (`raw fd` on POSIX and `HANDLE` on Windows).

```c
ten_file_t *f = ten_file_open("data.bin", "wb");
ten_file_write(f, buf, len);
ten_file_close(f);

f = ten_file_open("data.bin", "rb");
int64_t size = ten_file_size(f);
ten_file_seek(f, 0, 0);                 // SEEK_SET
size_t n = ten_file_read(f, buf, sizeof(buf));
ten_file_close(f);

ten_file_exists("data.bin");
ten_file_copy("data.bin", "backup.bin");
ten_file_move("backup.bin", "archive/backup.bin");
ten_file_delete("data.bin");
```

If `ten_file_read()` returns `0`, it may indicate either EOF or an error.

Check whether `ten_last_error() == TEN_OK` to distinguish the two.

### 7.13 Thread

```c
void *worker(void *arg) {
    printf("worker running\n");
    return NULL;
}

ten_thread_t *t = ten_thread_create(worker, NULL);
ten_thread_join(t, NULL);     // t becomes invalid after join; internal resources are released

ten_thread_sleep(100);         // ms
uint64_t id = ten_thread_current_id();
```

### 7.14 Synchronization

Mutex (including recursive mutex), Semaphore, and Condition Variable.

```c
ten_mutex_t *m = ten_mutex_create();               // Normal
ten_mutex_t *rm = ten_mutex_create_recursive();   // Same thread may lock repeatedly

ten_mutex_lock(m);
ten_mutex_try_lock(m);     // TEN_ERROR_TIMEOUT if already locked
ten_mutex_unlock(m);
ten_mutex_destroy(m);

ten_semaphore_t *sem = ten_semaphore_create(0);
ten_semaphore_post(sem);
ten_semaphore_wait(sem);
ten_semaphore_destroy(sem);

ten_cond_t *cond = ten_cond_create();
ten_mutex_lock(m);
while (!ready) ten_cond_wait(cond, m);          // Or ten_cond_wait_timeout
ten_mutex_unlock(m);
ten_cond_signal(cond);                          // Or ten_cond_broadcast
ten_cond_destroy(cond);
```

> **Note:** Recursive Mutex and Condition Variable are mentioned as supported in the specification, but their concrete function signatures were not defined. These APIs were added during implementation.

### 7.15 Thread Pool

A fixed-size worker pool built on top of `ten_thread` / `ten_mutex` / `ten_cond`.

It contains no platform-specific code.

```c
ten_thread_pool_t *pool = ten_thread_pool_create(4);

for (int i = 0; i < 100; i++) {
    ten_thread_pool_submit(pool, my_task, task_data(i));
}

ten_thread_pool_destroy(pool);   // Waits until the queue is empty, then terminates workers
```

### 7.16 Process

```c
uint64_t pid = ten_process_id();

const char *argv[] = { "ls", "-la", NULL };
ten_process_execute("ls", argv);        // Synchronous execution

const char *path = ten_env_get("PATH");
ten_env_set("MY_VAR", "value");
```

`ten_process_execute()` reports only whether execution itself succeeded.

Because its signature is fixed to a single `int`, there is no place to return the actual child process exit code. This is a documented design trade-off.

### 7.17 Encoding

Base64 is specified by the specification. Hex conversion, UTF conversion, and validation were added during implementation.

```c
char *b64 = ten_base64_encode(data, len);
void *raw = ten_base64_decode(b64, &raw_len);

char *hex = ten_hex_encode(data, len);
void *bytes = ten_hex_decode(hex, &bytes_len);

uint16_t *utf16;
size_t units;
utf16 = ten_utf8_to_utf16("가나다", &units);
char *utf8_back = ten_utf16_to_utf8(utf16, units);

ten_utf8_validate(text, len);    // 1/0
ten_ascii_validate(text, len);   // 1/0
```

### 7.18 JSON

`parse` / `stringify` / `destroy` are specified by the specification. Value creation/access APIs were added during implementation; otherwise integration with List/Map/String would not be possible.

**Objects actually use `ten_map` (for `O(1)` lookup) together with `ten_list` (to preserve insertion order), arrays use `ten_list`, and string values directly use `ten_string`.**

The three modules therefore work together in the actual implementation.

```c
ten_json_t *json = ten_json_parse(
    "{\"name\":\".TEN\",\"tags\":[\"c99\",\"cross-platform\"]}"
);

const char *name =
    ten_json_get_string(ten_json_object_get(json, "name"));

ten_json_t *tags = ten_json_object_get(json, "tags");

for (size_t i = 0; i < ten_json_array_count(tags); i++) {
    printf("%s\n",
           ten_json_get_string(ten_json_array_get(tags, i)));
}

ten_json_destroy(json);

// Build directly
ten_json_t *obj = ten_json_create_object();
ten_json_object_set(obj, "version", ten_json_create_number(1.0));

char *text = ten_json_stringify(obj);   // Free with ten_free()
ten_free(text);

ten_json_destroy(obj);
```

`array_add` / `object_set` take ownership of the value on success.

On failure, ownership remains with the caller.

### 7.19 Socket

TCP is explicitly specified by the specification.

UDP is only described as being provided using the same conventions as TCP, so it was implemented under the `ten_udp_*` naming scheme.

```c
// TCP server
ten_socket_t *server = ten_tcp_listen("0.0.0.0", 8080, 16);
ten_socket_t *client = ten_tcp_accept(server);
ten_socket_send(client, "hi", 2);
ten_socket_close(client);

// TCP client
ten_socket_t *conn = ten_tcp_connect("example.com", 80);
ten_socket_receive(conn, buf, sizeof(buf));
ten_socket_close(conn);

// UDP
ten_socket_t *udp = ten_udp_bind("0.0.0.0", 9090);
ten_udp_send_to(udp, "127.0.0.1", 9090, "ping", 4);
ten_udp_receive_from(
    udp,
    buf,
    sizeof(buf),
    host,
    sizeof(host),
    &port
);
```

`ten_socket_poll_readable()` was also implemented using `select()`.

It is used by the HTTP server to implement a safe `stop()` operation.

### 7.20 HTTP

Only the method enum and callback types were explicitly specified.

The complete request/response/client/server API was designed and implemented during development.

Known v1.0 simplifications:

* No keep-alive
* No pipelining
* No chunked encoding
* Bodies always use `Content-Length`

```c
// Server
void handler(
    const ten_http_request_t *req,
    ten_http_response_t *resp,
    void *ctx
) {
    if (strcmp(ten_http_request_path(req), "/hello") == 0) {
        ten_http_response_set_status(resp, 200);
        ten_http_response_set_body(resp, "world", 5);
    } else {
        ten_http_response_set_status(resp, 404);
    }
}

ten_http_server_t *server =
    ten_http_server_create("0.0.0.0", 8080, handler, NULL);

ten_http_server_run(server);     // Blocking
// ...
ten_http_server_stop(server);
ten_http_server_destroy(server);

// Client
ten_http_request_t *req =
    ten_http_request_create(TEN_HTTP_GET, "/hello");

ten_http_response_t *resp =
    ten_http_client_send("127.0.0.1", 8080, req);

printf(
    "%d: %.*s\n",
    ten_http_response_status(resp),
    (int)body_size,
    body
);

ten_http_request_destroy(req);
ten_http_response_destroy(resp);
```

> **Implementation Note:** Initially, `stop()` closed the listening socket from another thread in order to wake `accept()`. Under AddressSanitizer's slower timing, an actual file-descriptor reuse race condition was reproduced, causing the operation to hang.
>
> The implementation was redesigned around `ten_socket_poll_readable()` (`select()` with a 200 ms timeout) and a mutex-protected `stopping` flag. This resolved the issue.

### 7.21 Plugin

Dynamic library loading.

`ten_plugin_init` / `ten_plugin_shutdown` are **not implemented by the `.TEN` core**. They are entry-point contracts that plugin authors implement and export.

```c
// Plugin side (separate .so/.dll)
int ten_plugin_init(void) {
    /* ... */
    return 0;
}

void ten_plugin_shutdown(void) {
    /* ... */
}

// Host/application side
ten_plugin_t *plugin =
    ten_plugin_load("./myplugin.so");

ten_plugin_call_init(plugin);
// Finds and calls the "ten_plugin_init" symbol

void *sym =
    ten_plugin_symbol(plugin, "my_custom_function");

// Casting void* to a function pointer uses the memcpy type-punning
// approach to avoid -Wpedantic issues, following POSIX conventions.

ten_plugin_call_shutdown(plugin);
ten_plugin_unload(plugin);
```

---

## 8. Rust Bindings

There are two crates under `bindings/rust/`:

* **`ten-sys`** — Raw FFI bindings (`unsafe`, 1:1 with the C headers). `build.rs` automatically builds the C library through `core/CMakeLists.txt` and statically links it.
* **`ten`** — Safe wrapper crate providing a Rust API corresponding to the modules.

```rust
fn main() -> Result<(), ten::Error> {
    ten::init()?;

    let mut s = ten::TenString::new("Hello")?;
    s.append(", .TEN!")?;
    println!("{s}");

    let mut buf = ten::TenBuffer::new()?;
    buf.write(b"hello")?;

    let mut json = ten::TenJson::object()?;
    json.object_set("name", ten::TenJson::string(".TEN")?)?;
    println!("{}", json.stringify()?);

    let pool = ten::TenThreadPool::new(4)?;
    pool.submit(|| println!("task ran"))?;

    ten::shutdown();
    Ok(())
}
```

### Intentionally Unwrapped APIs

There are intentionally no safe wrappers for the following:

* **List/Map** — The `ten_type_t`-based generic design requires additional consideration, so these are currently deferred. They can still be used through the raw `ten_sys` bindings.
* **Thread/Sync primitive types** (`ten_mutex_t`, etc.) — Rust's `std::thread` / `std::sync` already provide safer and more idiomatic alternatives, so wrapping them provides little benefit. The raw `ten_sys` bindings remain useful when C handles need to be exchanged, such as for Plugin ABI integration.

### Closure-Based Callbacks

Closure-based callbacks are supported by `TenThreadPool::submit` and `TenHttpServer::new`.

Rust closures are boxed twice to obtain a thin pointer that can be passed through C's `void *userdata`. An `extern "C"` trampoline function then reconstructs and invokes the closure. This follows the standard callback pattern.

---

## 9. Testing & Verification

* **20 module tests:** `test_core`, `test_error`, `test_memory`, `test_list`, `test_map`, `test_string`, `test_buffer`, `test_path`, `test_time`, `test_log`, `test_file`, `test_thread`, `test_sync`, `test_thread_pool`, `test_process`, `test_encoding`, `test_json`, `test_socket`, `test_http`, `test_plugin`.
* Cases cover NULL arguments, boundary values, out-of-range access, duplicate keys, missing files/symbols, network errors, invalid encodings, malformed JSON, and other edge cases.
* `test_plugin` is an integration test that loads an independent dummy shared library with `dlopen()` and verifies the plugin interface without linking the test itself against `.TEN`.
* **Verification result:** GCC with `-std=c99 -Wall -Wextra -Wpedantic` produced **zero warnings**. AddressSanitizer + UndefinedBehaviorSanitizer were **fully clean**. Concurrent modules (Thread/Sync/ThreadPool/Socket/HTTP) were also clean under ThreadSanitizer.
* All of these results are based on POSIX/Linux. The Windows backend could not be compiled because no Windows build environment was available.
* An actual race condition was reproduced and fixed under the ASan environment (see the HTTP section in 7.20).

---

## 10. Known Limitations and Remaining Work

### Documented Design Simplifications

* **HTTP:** No keep-alive, pipelining, or chunked encoding.
* **`ten_process_execute()`:** Does not return the actual child process exit code because its signature is fixed to a single `int`.
* **Map lookup result pointers / internal List and Buffer references:** May become invalid when the collection is modified, following normal C conventions.

### Unverified Components

* **Windows backend (`src/platform/windows/*`)** — The code has been written, but it has not even been compiled because MSVC/MinGW was unavailable in the development environment.
* **Android NDK build** — Designed to share the POSIX backend, but has not been actually built.
* **Complete Rust bindings** — The environment did not have `cargo`, so they have never been actually compiled.

### Not Yet Completed

* Some cases from the 45-section test matrix, including permission-failure tests. These were intentionally omitted because the test environment was running as root, making such tests ineffective.
* Generating the actual Doxygen output. `Doxyfile` exists, but Doxygen was not installed in the environment, so it could not be executed.

Alongside this guide, the function-level comments in each header file (`core/include/ten/*.h`) are the most accurate reference. Ownership, thread-safety requirements, and error codes are documented for each function.
