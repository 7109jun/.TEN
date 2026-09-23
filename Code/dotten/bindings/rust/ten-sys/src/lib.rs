//! `.TEN` C99 라이브러리에 대한 raw FFI 바인딩.
//!
//! 이 크레이트는 안전성 보장이 없는 1:1 바인딩만 제공한다.
//! 애플리케이션 코드는 대신 `ten` 크레이트(safe wrapper)를 사용해야 한다.
//!
//! `.TEN` 사양서의 Phase 1~5(Core/System/Network/Data/Extension) 전체
//! 헤더에 대한 바인딩이다: core,error,memory,type,list,map,string,buffer,
//! path,time,log,file,thread,sync,thread_pool,process,encoding,json,
//! socket,http,plugin.

#![allow(non_camel_case_types)]

use std::os::raw::{c_char, c_int, c_void};

// core/include/ten/error.h 의 ten_error_code_t 와 값이 1:1로 일치해야 한다.
pub const TEN_OK: c_int = 0;
pub const TEN_ERROR_UNKNOWN: c_int = 1;
pub const TEN_ERROR_INVALID_ARGUMENT: c_int = 2;
pub const TEN_ERROR_NULL_ARGUMENT: c_int = 3;
pub const TEN_ERROR_OUT_OF_MEMORY: c_int = 4;
pub const TEN_ERROR_OUT_OF_RANGE: c_int = 5;
pub const TEN_ERROR_NOT_FOUND: c_int = 6;
pub const TEN_ERROR_ALREADY_EXISTS: c_int = 7;
pub const TEN_ERROR_ACCESS_DENIED: c_int = 8;
pub const TEN_ERROR_NOT_SUPPORTED: c_int = 9;
pub const TEN_ERROR_INVALID_STATE: c_int = 10;
pub const TEN_ERROR_TIMEOUT: c_int = 11;
pub const TEN_ERROR_CANCELLED: c_int = 12;
pub const TEN_ERROR_IO: c_int = 13;
pub const TEN_ERROR_NETWORK: c_int = 14;
pub const TEN_ERROR_PARSE: c_int = 15;
pub const TEN_ERROR_ENCODING: c_int = 16;
pub const TEN_ERROR_SYSTEM: c_int = 17;
pub const TEN_ERROR_ALREADY_INITIALIZED: c_int = 18;
pub const TEN_ERROR_NOT_INITIALIZED: c_int = 19;

// core/include/ten/type.h 의 ten_type_t 와 값이 1:1로 일치해야 한다.
pub const TEN_TYPE_VOID: c_int = 0;
pub const TEN_TYPE_BOOL: c_int = 1;
pub const TEN_TYPE_I8: c_int = 2;
pub const TEN_TYPE_I16: c_int = 3;
pub const TEN_TYPE_I32: c_int = 4;
pub const TEN_TYPE_I64: c_int = 5;
pub const TEN_TYPE_U8: c_int = 6;
pub const TEN_TYPE_U16: c_int = 7;
pub const TEN_TYPE_U32: c_int = 8;
pub const TEN_TYPE_U64: c_int = 9;
pub const TEN_TYPE_F32: c_int = 10;
pub const TEN_TYPE_F64: c_int = 11;
pub const TEN_TYPE_STRING: c_int = 12;
pub const TEN_TYPE_BUFFER: c_int = 13;
pub const TEN_TYPE_POINTER: c_int = 14;

// core/include/ten/log.h 의 ten_log_level_t 와 값이 1:1로 일치해야 한다.
pub const TEN_LOG_TRACE: c_int = 0;
pub const TEN_LOG_DEBUG: c_int = 1;
pub const TEN_LOG_INFO: c_int = 2;
pub const TEN_LOG_WARN: c_int = 3;
pub const TEN_LOG_ERROR: c_int = 4;
pub const TEN_LOG_FATAL: c_int = 5;

/// core/include/ten/log.h 의 `ten_log_callback_t`.
pub type ten_log_callback_t =
    Option<extern "C" fn(level: c_int, message: *const c_char, userdata: *mut c_void)>;

/// core/include/ten/file.h 의 `ten_file_t` — opaque 타입.
#[repr(C)]
pub struct ten_file_t {
    _private: [u8; 0],
}

/// core/include/ten/list.h 의 `ten_list_t` — opaque 타입.
#[repr(C)]
pub struct ten_list_t {
    _private: [u8; 0],
}

/// core/include/ten/map.h 의 `ten_map_t` — opaque 타입.
#[repr(C)]
pub struct ten_map_t {
    _private: [u8; 0],
}

/// core/include/ten/string.h 의 `ten_string_t` — opaque 타입.
#[repr(C)]
pub struct ten_string_t {
    _private: [u8; 0],
}

/// core/include/ten/buffer.h 의 `ten_buffer_t` — opaque 타입.
#[repr(C)]
pub struct ten_buffer_t {
    _private: [u8; 0],
}

/// core/include/ten/thread.h 의 `ten_thread_t` — opaque 타입.
#[repr(C)]
pub struct ten_thread_t {
    _private: [u8; 0],
}

/// core/include/ten/thread.h 의 `ten_thread_fn`.
pub type ten_thread_fn = Option<extern "C" fn(userdata: *mut c_void) -> *mut c_void>;

/// core/include/ten/sync.h 의 `ten_mutex_t` — opaque 타입.
#[repr(C)]
pub struct ten_mutex_t {
    _private: [u8; 0],
}

/// core/include/ten/sync.h 의 `ten_semaphore_t` — opaque 타입.
#[repr(C)]
pub struct ten_semaphore_t {
    _private: [u8; 0],
}

/// core/include/ten/sync.h 의 `ten_cond_t` — opaque 타입.
#[repr(C)]
pub struct ten_cond_t {
    _private: [u8; 0],
}

/// core/include/ten/thread_pool.h 의 `ten_thread_pool_t` — opaque 타입.
#[repr(C)]
pub struct ten_thread_pool_t {
    _private: [u8; 0],
}

// core/include/ten/json.h 의 ten_json_type_t 와 값이 1:1로 일치해야 한다.
pub const TEN_JSON_NULL: c_int = 0;
pub const TEN_JSON_BOOL: c_int = 1;
pub const TEN_JSON_NUMBER: c_int = 2;
pub const TEN_JSON_STRING: c_int = 3;
pub const TEN_JSON_ARRAY: c_int = 4;
pub const TEN_JSON_OBJECT: c_int = 5;

/// core/include/ten/json.h 의 `ten_json_t` — opaque 타입.
#[repr(C)]
pub struct ten_json_t {
    _private: [u8; 0],
}

/// core/include/ten/socket.h 의 `ten_socket_t` — opaque 타입.
#[repr(C)]
pub struct ten_socket_t {
    _private: [u8; 0],
}

// core/include/ten/http.h 의 ten_http_method_t 와 값이 1:1로 일치해야 한다.
pub const TEN_HTTP_GET: c_int = 0;
pub const TEN_HTTP_POST: c_int = 1;
pub const TEN_HTTP_PUT: c_int = 2;
pub const TEN_HTTP_DELETE: c_int = 3;
pub const TEN_HTTP_HEAD: c_int = 4;
pub const TEN_HTTP_OPTIONS: c_int = 5;

/// core/include/ten/http.h 의 `ten_http_request_t` — opaque 타입.
#[repr(C)]
pub struct ten_http_request_t {
    _private: [u8; 0],
}

/// core/include/ten/http.h 의 `ten_http_response_t` — opaque 타입.
#[repr(C)]
pub struct ten_http_response_t {
    _private: [u8; 0],
}

/// core/include/ten/http.h 의 `ten_http_server_t` — opaque 타입.
#[repr(C)]
pub struct ten_http_server_t {
    _private: [u8; 0],
}

/// core/include/ten/plugin.h 의 `ten_plugin_t` — opaque 타입.
#[repr(C)]
pub struct ten_plugin_t {
    _private: [u8; 0],
}

/// core/include/ten/plugin.h 의 `TEN_PLUGIN_ABI_VERSION`.
pub const TEN_PLUGIN_ABI_VERSION: c_int = 1;

/// core/include/ten/http.h 의 `ten_http_handler_t`.
pub type ten_http_handler_t = Option<
    extern "C" fn(
        request: *const ten_http_request_t,
        response: *mut ten_http_response_t,
        userdata: *mut c_void,
    ),
>;

extern "C" {
    // core/include/ten/core.h
    pub fn ten_init() -> c_int;
    pub fn ten_is_initialized() -> c_int;
    pub fn ten_shutdown();

    pub fn ten_version() -> *const c_char;
    pub fn ten_version_major() -> c_int;
    pub fn ten_version_minor() -> c_int;
    pub fn ten_version_patch() -> c_int;

    // core/include/ten/error.h
    pub fn ten_last_error() -> c_int;
    pub fn ten_error_message(code: c_int) -> *const c_char;
    pub fn ten_clear_error();

    // core/include/ten/memory.h
    pub fn ten_malloc(size: usize) -> *mut c_void;
    pub fn ten_calloc(count: usize, size: usize) -> *mut c_void;
    pub fn ten_realloc(ptr: *mut c_void, size: usize) -> *mut c_void;
    pub fn ten_free(ptr: *mut c_void);
    pub fn ten_memdup(data: *const c_void, size: usize) -> *mut c_void;
    pub fn ten_set_allocator(allocator: *const ten_allocator_t) -> c_int;

    // core/include/ten/list.h
    pub fn ten_list_create(element_type: c_int) -> *mut ten_list_t;
    pub fn ten_list_destroy(list: *mut ten_list_t);
    pub fn ten_list_add(list: *mut ten_list_t, value: *const c_void) -> c_int;
    pub fn ten_list_insert(list: *mut ten_list_t, index: usize, value: *const c_void) -> c_int;
    pub fn ten_list_remove_at(list: *mut ten_list_t, index: usize) -> c_int;
    pub fn ten_list_get(list: *mut ten_list_t, index: usize) -> *mut c_void;
    pub fn ten_list_count(list: *const ten_list_t) -> usize;
    pub fn ten_list_capacity(list: *const ten_list_t) -> usize;
    pub fn ten_list_reserve(list: *mut ten_list_t, capacity: usize) -> c_int;

    // core/include/ten/map.h
    pub fn ten_map_create(key_type: c_int, value_type: c_int) -> *mut ten_map_t;
    pub fn ten_map_destroy(map: *mut ten_map_t);
    pub fn ten_map_set(map: *mut ten_map_t, key: *const c_void, value: *const c_void) -> c_int;
    pub fn ten_map_get(map: *mut ten_map_t, key: *const c_void) -> *mut c_void;
    pub fn ten_map_remove(map: *mut ten_map_t, key: *const c_void) -> c_int;
    pub fn ten_map_contains(map: *const ten_map_t, key: *const c_void) -> c_int;
    pub fn ten_map_count(map: *const ten_map_t) -> usize;

    // core/include/ten/string.h
    pub fn ten_string_create(text: *const c_char) -> *mut ten_string_t;
    pub fn ten_string_empty() -> *mut ten_string_t;
    pub fn ten_string_destroy(string: *mut ten_string_t);
    pub fn ten_string_cstr(string: *const ten_string_t) -> *const c_char;
    pub fn ten_string_length(string: *const ten_string_t) -> usize;
    pub fn ten_string_append(string: *mut ten_string_t, text: *const c_char) -> c_int;
    pub fn ten_string_equals(a: *const ten_string_t, b: *const ten_string_t) -> c_int;
    pub fn ten_string_contains(string: *const ten_string_t, text: *const c_char) -> c_int;
    pub fn ten_string_substring(
        string: *const ten_string_t,
        start: usize,
        length: usize,
    ) -> *mut ten_string_t;

    // core/include/ten/buffer.h
    pub fn ten_buffer_create() -> *mut ten_buffer_t;
    pub fn ten_buffer_destroy(buffer: *mut ten_buffer_t);
    pub fn ten_buffer_write(buffer: *mut ten_buffer_t, data: *const c_void, size: usize) -> c_int;
    pub fn ten_buffer_data(buffer: *mut ten_buffer_t) -> *mut c_void;
    pub fn ten_buffer_size(buffer: *const ten_buffer_t) -> usize;
    pub fn ten_buffer_clear(buffer: *mut ten_buffer_t);

    // core/include/ten/path.h — 반환된 char*는 ten_free()로 해제해야 한다.
    pub fn ten_path_join(a: *const c_char, b: *const c_char) -> *mut c_char;
    pub fn ten_path_directory(path: *const c_char) -> *mut c_char;
    pub fn ten_path_filename(path: *const c_char) -> *mut c_char;
    pub fn ten_path_extension(path: *const c_char) -> *mut c_char;
    pub fn ten_path_is_absolute(path: *const c_char) -> c_int;

    // core/include/ten/time.h
    pub fn ten_time_now_ms() -> u64;
    pub fn ten_time_now_us() -> u64;
    pub fn ten_time_monotonic_ms() -> u64;
    pub fn ten_time_unix() -> i64;

    // core/include/ten/log.h
    pub fn ten_log(level: c_int, format: *const c_char, ...);
    pub fn ten_log_set_level(level: c_int);
    pub fn ten_log_set_console(enabled: c_int);
    pub fn ten_log_set_file(path: *const c_char) -> c_int;
    pub fn ten_log_set_callback(callback: ten_log_callback_t, userdata: *mut c_void);

    // core/include/ten/file.h
    pub fn ten_file_open(path: *const c_char, mode: *const c_char) -> *mut ten_file_t;
    pub fn ten_file_close(file: *mut ten_file_t);
    pub fn ten_file_read(file: *mut ten_file_t, buffer: *mut c_void, size: usize) -> usize;
    pub fn ten_file_write(file: *mut ten_file_t, buffer: *const c_void, size: usize) -> usize;
    pub fn ten_file_seek(file: *mut ten_file_t, offset: i64, origin: c_int) -> c_int;
    pub fn ten_file_size(file: *mut ten_file_t) -> i64;
    pub fn ten_file_exists(path: *const c_char) -> c_int;
    pub fn ten_file_delete(path: *const c_char) -> c_int;
    pub fn ten_file_copy(source: *const c_char, destination: *const c_char) -> c_int;
    pub fn ten_file_move(source: *const c_char, destination: *const c_char) -> c_int;

    // core/include/ten/thread.h
    pub fn ten_thread_create(function: ten_thread_fn, userdata: *mut c_void) -> *mut ten_thread_t;
    pub fn ten_thread_join(thread: *mut ten_thread_t, result: *mut *mut c_void) -> c_int;
    pub fn ten_thread_sleep(milliseconds: u64);
    pub fn ten_thread_current_id() -> u64;

    // core/include/ten/sync.h
    pub fn ten_mutex_create() -> *mut ten_mutex_t;
    pub fn ten_mutex_create_recursive() -> *mut ten_mutex_t;
    pub fn ten_mutex_destroy(mutex: *mut ten_mutex_t);
    pub fn ten_mutex_lock(mutex: *mut ten_mutex_t) -> c_int;
    pub fn ten_mutex_try_lock(mutex: *mut ten_mutex_t) -> c_int;
    pub fn ten_mutex_unlock(mutex: *mut ten_mutex_t) -> c_int;

    pub fn ten_semaphore_create(initial: std::os::raw::c_uint) -> *mut ten_semaphore_t;
    pub fn ten_semaphore_wait(semaphore: *mut ten_semaphore_t) -> c_int;
    pub fn ten_semaphore_post(semaphore: *mut ten_semaphore_t) -> c_int;
    pub fn ten_semaphore_destroy(semaphore: *mut ten_semaphore_t);

    pub fn ten_cond_create() -> *mut ten_cond_t;
    pub fn ten_cond_destroy(cond: *mut ten_cond_t);
    pub fn ten_cond_wait(cond: *mut ten_cond_t, mutex: *mut ten_mutex_t) -> c_int;
    pub fn ten_cond_wait_timeout(
        cond: *mut ten_cond_t,
        mutex: *mut ten_mutex_t,
        milliseconds: u64,
    ) -> c_int;
    pub fn ten_cond_signal(cond: *mut ten_cond_t) -> c_int;
    pub fn ten_cond_broadcast(cond: *mut ten_cond_t) -> c_int;

    // core/include/ten/thread_pool.h
    pub fn ten_thread_pool_create(worker_count: usize) -> *mut ten_thread_pool_t;
    pub fn ten_thread_pool_submit(
        pool: *mut ten_thread_pool_t,
        function: ten_thread_fn,
        userdata: *mut c_void,
    ) -> c_int;
    pub fn ten_thread_pool_destroy(pool: *mut ten_thread_pool_t);

    // core/include/ten/process.h
    pub fn ten_process_id() -> u64;
    pub fn ten_process_execute(program: *const c_char, argv: *const *const c_char) -> c_int;
    pub fn ten_env_get(name: *const c_char) -> *const c_char;
    pub fn ten_env_set(name: *const c_char, value: *const c_char) -> c_int;

    // core/include/ten/encoding.h
    pub fn ten_base64_encode(data: *const c_void, size: usize) -> *mut c_char;
    pub fn ten_base64_decode(text: *const c_char, size: *mut usize) -> *mut c_void;
    pub fn ten_hex_encode(data: *const c_void, size: usize) -> *mut c_char;
    pub fn ten_hex_decode(text: *const c_char, size: *mut usize) -> *mut c_void;
    pub fn ten_utf8_to_utf16(utf8: *const c_char, out_length: *mut usize) -> *mut u16;
    pub fn ten_utf16_to_utf8(utf16: *const u16, length: usize) -> *mut c_char;
    pub fn ten_ascii_validate(text: *const c_char, size: usize) -> c_int;
    pub fn ten_utf8_validate(text: *const c_char, size: usize) -> c_int;

    // core/include/ten/json.h
    pub fn ten_json_parse(text: *const c_char) -> *mut ten_json_t;
    pub fn ten_json_stringify(json: *const ten_json_t) -> *mut c_char;
    pub fn ten_json_destroy(json: *mut ten_json_t);

    pub fn ten_json_create_null() -> *mut ten_json_t;
    pub fn ten_json_create_bool(value: c_int) -> *mut ten_json_t;
    pub fn ten_json_create_number(value: f64) -> *mut ten_json_t;
    pub fn ten_json_create_string(value: *const c_char) -> *mut ten_json_t;
    pub fn ten_json_create_array() -> *mut ten_json_t;
    pub fn ten_json_create_object() -> *mut ten_json_t;

    pub fn ten_json_type(json: *const ten_json_t) -> c_int;
    pub fn ten_json_get_bool(json: *const ten_json_t) -> c_int;
    pub fn ten_json_get_number(json: *const ten_json_t) -> f64;
    pub fn ten_json_get_string(json: *const ten_json_t) -> *const c_char;

    pub fn ten_json_array_count(json: *const ten_json_t) -> usize;
    pub fn ten_json_array_get(json: *const ten_json_t, index: usize) -> *mut ten_json_t;
    pub fn ten_json_array_add(json: *mut ten_json_t, value: *mut ten_json_t) -> c_int;

    pub fn ten_json_object_count(json: *const ten_json_t) -> usize;
    pub fn ten_json_object_get(json: *const ten_json_t, key: *const c_char) -> *mut ten_json_t;
    pub fn ten_json_object_set(
        json: *mut ten_json_t,
        key: *const c_char,
        value: *mut ten_json_t,
    ) -> c_int;
    pub fn ten_json_object_key_at(json: *const ten_json_t, index: usize) -> *const c_char;

    // core/include/ten/socket.h
    pub fn ten_tcp_connect(host: *const c_char, port: u16) -> *mut ten_socket_t;
    pub fn ten_tcp_listen(host: *const c_char, port: u16, backlog: c_int) -> *mut ten_socket_t;
    pub fn ten_tcp_accept(server: *mut ten_socket_t) -> *mut ten_socket_t;

    pub fn ten_udp_bind(host: *const c_char, port: u16) -> *mut ten_socket_t;
    pub fn ten_udp_socket() -> *mut ten_socket_t;
    pub fn ten_udp_send_to(
        socket: *mut ten_socket_t,
        host: *const c_char,
        port: u16,
        data: *const c_void,
        size: usize,
    ) -> i64;
    pub fn ten_udp_receive_from(
        socket: *mut ten_socket_t,
        buffer: *mut c_void,
        size: usize,
        out_host: *mut c_char,
        out_host_size: usize,
        out_port: *mut u16,
    ) -> i64;

    pub fn ten_socket_send(socket: *mut ten_socket_t, data: *const c_void, size: usize) -> i64;
    pub fn ten_socket_receive(socket: *mut ten_socket_t, buffer: *mut c_void, size: usize) -> i64;
    pub fn ten_socket_close(socket: *mut ten_socket_t);
    pub fn ten_socket_native_handle(socket: *const ten_socket_t) -> isize;
    pub fn ten_socket_poll_readable(socket: *const ten_socket_t, timeout_ms: c_int) -> c_int;

    // core/include/ten/http.h
    pub fn ten_http_request_create(method: c_int, path: *const c_char) -> *mut ten_http_request_t;
    pub fn ten_http_request_destroy(request: *mut ten_http_request_t);
    pub fn ten_http_request_set_header(
        request: *mut ten_http_request_t,
        name: *const c_char,
        value: *const c_char,
    ) -> c_int;
    pub fn ten_http_request_set_body(
        request: *mut ten_http_request_t,
        data: *const c_void,
        size: usize,
    ) -> c_int;
    pub fn ten_http_request_method(request: *const ten_http_request_t) -> c_int;
    pub fn ten_http_request_path(request: *const ten_http_request_t) -> *const c_char;
    pub fn ten_http_request_header(
        request: *const ten_http_request_t,
        name: *const c_char,
    ) -> *const c_char;
    pub fn ten_http_request_body(
        request: *const ten_http_request_t,
        out_size: *mut usize,
    ) -> *const c_void;

    pub fn ten_http_response_set_status(response: *mut ten_http_response_t, status_code: c_int) -> c_int;
    pub fn ten_http_response_set_header(
        response: *mut ten_http_response_t,
        name: *const c_char,
        value: *const c_char,
    ) -> c_int;
    pub fn ten_http_response_set_body(
        response: *mut ten_http_response_t,
        data: *const c_void,
        size: usize,
    ) -> c_int;
    pub fn ten_http_response_status(response: *const ten_http_response_t) -> c_int;
    pub fn ten_http_response_header(
        response: *const ten_http_response_t,
        name: *const c_char,
    ) -> *const c_char;
    pub fn ten_http_response_body(
        response: *const ten_http_response_t,
        out_size: *mut usize,
    ) -> *const c_void;
    pub fn ten_http_response_destroy(response: *mut ten_http_response_t);

    pub fn ten_http_client_send(
        host: *const c_char,
        port: u16,
        request: *const ten_http_request_t,
    ) -> *mut ten_http_response_t;

    pub fn ten_http_server_create(
        host: *const c_char,
        port: u16,
        handler: ten_http_handler_t,
        userdata: *mut c_void,
    ) -> *mut ten_http_server_t;
    pub fn ten_http_server_run(server: *mut ten_http_server_t) -> c_int;
    pub fn ten_http_server_stop(server: *mut ten_http_server_t);
    pub fn ten_http_server_destroy(server: *mut ten_http_server_t);

    // core/include/ten/plugin.h
    pub fn ten_plugin_load(path: *const c_char) -> *mut ten_plugin_t;
    pub fn ten_plugin_symbol(plugin: *mut ten_plugin_t, name: *const c_char) -> *mut c_void;
    pub fn ten_plugin_unload(plugin: *mut ten_plugin_t);
    pub fn ten_plugin_call_init(plugin: *mut ten_plugin_t) -> c_int;
    pub fn ten_plugin_call_shutdown(plugin: *mut ten_plugin_t);
}

/// core/include/ten/memory.h 의 `ten_allocator_t`.
/// 현재 `ten`(safe wrapper) 크레이트에는 이를 사용하는 safe API가 아직
/// 없다 — Rust 쪽에서 `.TEN`의 allocator를 직접 교체할 필요가 생기면
/// (예: Plugin ABI 연동) 추가한다.
#[repr(C)]
pub struct ten_allocator_t {
    pub malloc_fn: Option<extern "C" fn(usize) -> *mut c_void>,
    pub calloc_fn: Option<extern "C" fn(usize, usize) -> *mut c_void>,
    pub realloc_fn: Option<extern "C" fn(*mut c_void, usize) -> *mut c_void>,
    pub free_fn: Option<extern "C" fn(*mut c_void)>,
}
