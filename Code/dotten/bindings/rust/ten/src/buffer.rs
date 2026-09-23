//! `ten_buffer_t`에 대한 safe wrapper.

use std::fmt;
use std::os::raw::c_void;

use crate::{last_error, Error};

/// `.TEN`의 바이너리 버퍼(`ten_buffer_t`)를 소유하는 safe wrapper.
///
/// `Drop` 시 `ten_buffer_destroy()`를 호출한다. NOT_THREAD_SAFE.
pub struct TenBuffer {
    raw: *mut ten_sys::ten_buffer_t,
}

impl TenBuffer {
    /// `ten_buffer_create()`.
    pub fn new() -> Result<Self, Error> {
        let raw = unsafe { ten_sys::ten_buffer_create() };
        if raw.is_null() {
            Err(last_error().unwrap_or(Error::Unknown))
        } else {
            Ok(TenBuffer { raw })
        }
    }

    /// `ten_buffer_write()`.
    pub fn write(&mut self, data: &[u8]) -> Result<(), Error> {
        let code = unsafe {
            ten_sys::ten_buffer_write(self.raw, data.as_ptr() as *const c_void, data.len())
        };
        match Error::from_code(code) {
            None => Ok(()),
            Some(e) => Err(e),
        }
    }

    /// `ten_buffer_data()` + `ten_buffer_size()`.
    pub fn as_slice(&self) -> &[u8] {
        unsafe {
            let len = ten_sys::ten_buffer_size(self.raw);
            if len == 0 {
                return &[];
            }
            let ptr = ten_sys::ten_buffer_data(self.raw);
            std::slice::from_raw_parts(ptr as *const u8, len)
        }
    }

    /// `ten_buffer_size()`.
    pub fn len(&self) -> usize {
        unsafe { ten_sys::ten_buffer_size(self.raw) }
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    /// `ten_buffer_clear()`.
    pub fn clear(&mut self) {
        unsafe { ten_sys::ten_buffer_clear(self.raw) }
    }
}

impl fmt::Debug for TenBuffer {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("TenBuffer").field("len", &self.len()).finish()
    }
}

impl Drop for TenBuffer {
    fn drop(&mut self) {
        unsafe { ten_sys::ten_buffer_destroy(self.raw) }
    }
}
