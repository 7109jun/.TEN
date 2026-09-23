//! `ten/log.h`에 대한 safe wrapper.
//!
//! 콜백은 C 쪽이 전역 단일 콜백 하나만 지원하므로(사양서 28절), Rust
//! 쪽도 캡처하지 않는 `fn` 포인터 하나만 전역으로 등록하는 형태로
//! 제한한다 — 클로저 캡처가 필요하면 별도 전역 상태(예: `OnceLock`)를
//! 콜백 안에서 직접 참조하는 방식을 사용한다.

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_void};
use std::sync::Mutex;

use crate::Error;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
}

impl LogLevel {
    fn to_raw(self) -> c_int {
        match self {
            LogLevel::Trace => ten_sys::TEN_LOG_TRACE,
            LogLevel::Debug => ten_sys::TEN_LOG_DEBUG,
            LogLevel::Info => ten_sys::TEN_LOG_INFO,
            LogLevel::Warn => ten_sys::TEN_LOG_WARN,
            LogLevel::Error => ten_sys::TEN_LOG_ERROR,
            LogLevel::Fatal => ten_sys::TEN_LOG_FATAL,
        }
    }

    fn from_raw(raw: c_int) -> LogLevel {
        match raw {
            r if r == ten_sys::TEN_LOG_TRACE => LogLevel::Trace,
            r if r == ten_sys::TEN_LOG_DEBUG => LogLevel::Debug,
            r if r == ten_sys::TEN_LOG_INFO => LogLevel::Info,
            r if r == ten_sys::TEN_LOG_WARN => LogLevel::Warn,
            r if r == ten_sys::TEN_LOG_ERROR => LogLevel::Error,
            _ => LogLevel::Fatal,
        }
    }
}

pub type LogCallback = fn(LogLevel, &str);

static CALLBACK: Mutex<Option<LogCallback>> = Mutex::new(None);

extern "C" fn trampoline(level: c_int, message: *const c_char, _userdata: *mut c_void) {
    let level = LogLevel::from_raw(level);
    let msg = unsafe { CStr::from_ptr(message) }
        .to_str()
        .unwrap_or("<invalid utf-8>");

    if let Ok(guard) = CALLBACK.lock() {
        if let Some(cb) = *guard {
            cb(level, msg);
        }
    }
}

/// `ten_log()`. message는 그대로 출력되며 printf 포맷 문자열로 해석되지
/// 않는다(내부적으로 `"%s"` 포맷을 사용해 안전하게 전달한다).
pub fn log(level: LogLevel, message: &str) {
    let fmt = CString::new("%s").unwrap();
    let msg = match CString::new(message) {
        Ok(m) => m,
        Err(_) => CString::new("<message contained NUL byte>").unwrap(),
    };

    unsafe { ten_sys::ten_log(level.to_raw(), fmt.as_ptr(), msg.as_ptr()) };
}

pub fn trace(message: &str) {
    log(LogLevel::Trace, message)
}
pub fn debug(message: &str) {
    log(LogLevel::Debug, message)
}
pub fn info(message: &str) {
    log(LogLevel::Info, message)
}
pub fn warn(message: &str) {
    log(LogLevel::Warn, message)
}
pub fn error(message: &str) {
    log(LogLevel::Error, message)
}
pub fn fatal(message: &str) {
    log(LogLevel::Fatal, message)
}

/// `ten_log_set_level()`.
pub fn set_level(level: LogLevel) {
    unsafe { ten_sys::ten_log_set_level(level.to_raw()) }
}

/// `ten_log_set_console()`.
pub fn set_console(enabled: bool) {
    unsafe { ten_sys::ten_log_set_console(if enabled { 1 } else { 0 }) }
}

/// `ten_log_set_file()`. `None`이면 기존 로그 파일을 닫고 file 출력을 끈다.
pub fn set_file(path: Option<&str>) -> Result<(), Error> {
    let code = match path {
        Some(p) => {
            let c = CString::new(p).map_err(|_| Error::InvalidArgument)?;
            unsafe { ten_sys::ten_log_set_file(c.as_ptr()) }
        }
        None => unsafe { ten_sys::ten_log_set_file(std::ptr::null()) },
    };

    match Error::from_code(code) {
        None => Ok(()),
        Some(e) => Err(e),
    }
}

/// `ten_log_set_callback()`. `None`이면 콜백을 해제한다.
pub fn set_callback(callback: Option<LogCallback>) {
    if let Ok(mut guard) = CALLBACK.lock() {
        *guard = callback;
    }

    unsafe {
        if callback.is_some() {
            ten_sys::ten_log_set_callback(Some(trampoline), std::ptr::null_mut());
        } else {
            ten_sys::ten_log_set_callback(None, std::ptr::null_mut());
        }
    }
}
