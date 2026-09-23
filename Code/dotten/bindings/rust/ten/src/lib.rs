//! `.TEN` C99 라이브러리에 대한 safe Rust wrapper — 사양서 Phase 1~5
//! (Core/System/Network/Data/Extension) 전체를 다룬다. List/Map은
//! ten_type_t 기반 제네릭 설계가 더 필요해 safe wrapper를 보류했다(raw
//! 바인딩은 `ten_sys`에 있음). Thread/Sync 원시 타입도 마찬가지로 safe
//! wrapper가 없다 — `std::thread`/`std::sync`가 더 나은 대안이라서다
//! (thread_pool 모듈 문서 참고). `plugin::TenPlugin::symbol()`이 돌려주는
//! 심볼을 실제 함수로 캐스팅해 호출하는 것은 원리적으로 unsafe로 남는다.

use std::ffi::CStr;
use std::fmt;

mod buffer;
mod string;
mod thread_pool;
pub mod encoding;
pub mod file;
pub mod http;
pub mod json;
pub mod log;
pub mod path;
pub mod plugin;
pub mod process;
pub mod socket;
pub mod time;

pub use buffer::TenBuffer;
pub use file::TenFile;
pub use http::{HttpMethod, TenHttpRequest, TenHttpRequestRef, TenHttpResponse, TenHttpResponseRef, TenHttpServer};
pub use json::{JsonType, TenJson, TenJsonRef};
pub use plugin::TenPlugin;
pub use socket::TenSocket;
pub use string::TenString;
pub use thread_pool::TenThreadPool;

/// `.TEN`의 `ten_error_code_t`에 대응하는 Rust 오류 타입.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Error {
    Unknown,
    InvalidArgument,
    NullArgument,
    OutOfMemory,
    OutOfRange,
    NotFound,
    AlreadyExists,
    AccessDenied,
    NotSupported,
    InvalidState,
    Timeout,
    Cancelled,
    Io,
    Network,
    Parse,
    Encoding,
    System,
    AlreadyInitialized,
    NotInitialized,
    /// C 쪽에서 알 수 없는 코드가 반환된 경우(바인딩 불일치 등).
    Other(i32),
}

impl Error {
    pub(crate) fn from_code(code: i32) -> Option<Self> {
        use ten_sys::*;
        match code {
            c if c == TEN_OK => None,
            c if c == TEN_ERROR_UNKNOWN => Some(Error::Unknown),
            c if c == TEN_ERROR_INVALID_ARGUMENT => Some(Error::InvalidArgument),
            c if c == TEN_ERROR_NULL_ARGUMENT => Some(Error::NullArgument),
            c if c == TEN_ERROR_OUT_OF_MEMORY => Some(Error::OutOfMemory),
            c if c == TEN_ERROR_OUT_OF_RANGE => Some(Error::OutOfRange),
            c if c == TEN_ERROR_NOT_FOUND => Some(Error::NotFound),
            c if c == TEN_ERROR_ALREADY_EXISTS => Some(Error::AlreadyExists),
            c if c == TEN_ERROR_ACCESS_DENIED => Some(Error::AccessDenied),
            c if c == TEN_ERROR_NOT_SUPPORTED => Some(Error::NotSupported),
            c if c == TEN_ERROR_INVALID_STATE => Some(Error::InvalidState),
            c if c == TEN_ERROR_TIMEOUT => Some(Error::Timeout),
            c if c == TEN_ERROR_CANCELLED => Some(Error::Cancelled),
            c if c == TEN_ERROR_IO => Some(Error::Io),
            c if c == TEN_ERROR_NETWORK => Some(Error::Network),
            c if c == TEN_ERROR_PARSE => Some(Error::Parse),
            c if c == TEN_ERROR_ENCODING => Some(Error::Encoding),
            c if c == TEN_ERROR_SYSTEM => Some(Error::System),
            c if c == TEN_ERROR_ALREADY_INITIALIZED => Some(Error::AlreadyInitialized),
            c if c == TEN_ERROR_NOT_INITIALIZED => Some(Error::NotInitialized),
            other => Some(Error::Other(other)),
        }
    }
}

impl Error {
    /// 이 오류에 대응하는 `.TEN`의 원시 오류 코드.
    fn code(&self) -> i32 {
        use ten_sys::*;
        match *self {
            Error::Unknown => TEN_ERROR_UNKNOWN,
            Error::InvalidArgument => TEN_ERROR_INVALID_ARGUMENT,
            Error::NullArgument => TEN_ERROR_NULL_ARGUMENT,
            Error::OutOfMemory => TEN_ERROR_OUT_OF_MEMORY,
            Error::OutOfRange => TEN_ERROR_OUT_OF_RANGE,
            Error::NotFound => TEN_ERROR_NOT_FOUND,
            Error::AlreadyExists => TEN_ERROR_ALREADY_EXISTS,
            Error::AccessDenied => TEN_ERROR_ACCESS_DENIED,
            Error::NotSupported => TEN_ERROR_NOT_SUPPORTED,
            Error::InvalidState => TEN_ERROR_INVALID_STATE,
            Error::Timeout => TEN_ERROR_TIMEOUT,
            Error::Cancelled => TEN_ERROR_CANCELLED,
            Error::Io => TEN_ERROR_IO,
            Error::Network => TEN_ERROR_NETWORK,
            Error::Parse => TEN_ERROR_PARSE,
            Error::Encoding => TEN_ERROR_ENCODING,
            Error::System => TEN_ERROR_SYSTEM,
            Error::AlreadyInitialized => TEN_ERROR_ALREADY_INITIALIZED,
            Error::NotInitialized => TEN_ERROR_NOT_INITIALIZED,
            Error::Other(code) => code,
        }
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // 메시지 문자열의 출처는 C 라이브러리(ten_error_message)로 단일화한다.
        let msg = unsafe {
            let ptr = ten_sys::ten_error_message(self.code());
            CStr::from_ptr(ptr).to_str().unwrap_or("invalid-utf8")
        };
        write!(f, "{}", msg)
    }
}

impl std::error::Error for Error {}

/// `ten_init()`. 이미 초기화된 경우 `Error::AlreadyInitialized`를 반환한다.
pub fn init() -> Result<(), Error> {
    let code = unsafe { ten_sys::ten_init() };
    match Error::from_code(code) {
        None => Ok(()),
        Some(e) => Err(e),
    }
}

/// 현재 스레드의 마지막 오류 상태를 조회한다(`ten_last_error()`).
/// `TEN_OK`이면 `None`을 반환한다.
pub fn last_error() -> Option<Error> {
    let code = unsafe { ten_sys::ten_last_error() };
    Error::from_code(code)
}

/// 현재 스레드의 마지막 오류 상태를 초기화한다(`ten_clear_error()`).
pub fn clear_error() {
    unsafe { ten_sys::ten_clear_error() }
}

/// `ten_is_initialized()`.
pub fn is_initialized() -> bool {
    unsafe { ten_sys::ten_is_initialized() != 0 }
}

/// `ten_shutdown()`. 초기화되지 않은 상태에서 호출해도 안전하다.
pub fn shutdown() {
    unsafe { ten_sys::ten_shutdown() }
}

/// `ten_version()` — "MAJOR.MINOR.PATCH" 형식의 정적 문자열.
pub fn version() -> &'static str {
    unsafe {
        let ptr = ten_sys::ten_version();
        CStr::from_ptr(ptr).to_str().unwrap_or("invalid-utf8")
    }
}

pub fn version_major() -> i32 {
    unsafe { ten_sys::ten_version_major() }
}

pub fn version_minor() -> i32 {
    unsafe { ten_sys::ten_version_minor() }
}

pub fn version_patch() -> i32 {
    unsafe { ten_sys::ten_version_patch() }
}
