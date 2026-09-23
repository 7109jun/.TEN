//! `ten_string_t`에 대한 safe wrapper.

use std::ffi::{CStr, CString};
use std::fmt;

use crate::{last_error, Error};

/// `.TEN`의 동적 문자열(`ten_string_t`)을 소유하는 safe wrapper.
///
/// `Drop` 시 `ten_string_destroy()`를 호출한다. C 쪽 문서대로
/// NOT_THREAD_SAFE이므로 여러 스레드에서 같은 인스턴스를 공유하려면
/// 호출자가 동기화해야 한다.
pub struct TenString {
    raw: *mut ten_sys::ten_string_t,
}

impl TenString {
    /// `ten_string_create()`.
    pub fn new(text: &str) -> Result<Self, Error> {
        let c_text = CString::new(text).map_err(|_| Error::InvalidArgument)?;
        let raw = unsafe { ten_sys::ten_string_create(c_text.as_ptr()) };
        if raw.is_null() {
            Err(last_error().unwrap_or(Error::Unknown))
        } else {
            Ok(TenString { raw })
        }
    }

    /// `ten_string_empty()`.
    pub fn empty() -> Result<Self, Error> {
        let raw = unsafe { ten_sys::ten_string_empty() };
        if raw.is_null() {
            Err(last_error().unwrap_or(Error::Unknown))
        } else {
            Ok(TenString { raw })
        }
    }

    /// `ten_string_cstr()` + UTF-8 디코딩.
    pub fn as_str(&self) -> &str {
        unsafe {
            let ptr = ten_sys::ten_string_cstr(self.raw);
            CStr::from_ptr(ptr).to_str().unwrap_or("invalid-utf8")
        }
    }

    /// `ten_string_length()`.
    pub fn len(&self) -> usize {
        unsafe { ten_sys::ten_string_length(self.raw) }
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    /// `ten_string_append()`.
    pub fn append(&mut self, text: &str) -> Result<(), Error> {
        let c_text = CString::new(text).map_err(|_| Error::InvalidArgument)?;
        let code = unsafe { ten_sys::ten_string_append(self.raw, c_text.as_ptr()) };
        match Error::from_code(code) {
            None => Ok(()),
            Some(e) => Err(e),
        }
    }

    /// `ten_string_contains()`. text에 내부 NUL 바이트가 있으면 항상
    /// `false`를 반환한다(C 문자열로 표현할 수 없으므로).
    pub fn contains(&self, text: &str) -> bool {
        let c_text = match CString::new(text) {
            Ok(c) => c,
            Err(_) => return false,
        };
        unsafe { ten_sys::ten_string_contains(self.raw, c_text.as_ptr()) != 0 }
    }

    /// `ten_string_substring()`.
    pub fn substring(&self, start: usize, length: usize) -> Result<Self, Error> {
        let raw = unsafe { ten_sys::ten_string_substring(self.raw, start, length) };
        if raw.is_null() {
            Err(last_error().unwrap_or(Error::Unknown))
        } else {
            Ok(TenString { raw })
        }
    }
}

impl PartialEq for TenString {
    /// `ten_string_equals()`.
    fn eq(&self, other: &Self) -> bool {
        unsafe { ten_sys::ten_string_equals(self.raw, other.raw) != 0 }
    }
}

impl Eq for TenString {}

impl fmt::Display for TenString {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.as_str())
    }
}

impl fmt::Debug for TenString {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_tuple("TenString").field(&self.as_str()).finish()
    }
}

impl Drop for TenString {
    fn drop(&mut self) {
        unsafe { ten_sys::ten_string_destroy(self.raw) }
    }
}
