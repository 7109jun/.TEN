//! `ten/path.h`에 대한 safe wrapper. 순수 문자열 함수라 별도 타입 없이
//! 자유 함수로 제공한다.

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_void};

use crate::{last_error, Error};

fn take_owned_c_string(ptr: *mut c_char) -> Result<String, Error> {
    if ptr.is_null() {
        return Err(last_error().unwrap_or(Error::Unknown));
    }

    let owned = unsafe { CStr::from_ptr(ptr) }.to_string_lossy().into_owned();
    unsafe { ten_sys::ten_free(ptr as *mut c_void) };
    Ok(owned)
}

/// `ten_path_join()`.
pub fn join(a: &str, b: &str) -> Result<String, Error> {
    let ca = CString::new(a).map_err(|_| Error::InvalidArgument)?;
    let cb = CString::new(b).map_err(|_| Error::InvalidArgument)?;
    let ptr = unsafe { ten_sys::ten_path_join(ca.as_ptr(), cb.as_ptr()) };
    take_owned_c_string(ptr)
}

/// `ten_path_directory()`.
pub fn directory(path: &str) -> Result<String, Error> {
    let c = CString::new(path).map_err(|_| Error::InvalidArgument)?;
    let ptr = unsafe { ten_sys::ten_path_directory(c.as_ptr()) };
    take_owned_c_string(ptr)
}

/// `ten_path_filename()`.
pub fn filename(path: &str) -> Result<String, Error> {
    let c = CString::new(path).map_err(|_| Error::InvalidArgument)?;
    let ptr = unsafe { ten_sys::ten_path_filename(c.as_ptr()) };
    take_owned_c_string(ptr)
}

/// `ten_path_extension()`.
pub fn extension(path: &str) -> Result<String, Error> {
    let c = CString::new(path).map_err(|_| Error::InvalidArgument)?;
    let ptr = unsafe { ten_sys::ten_path_extension(c.as_ptr()) };
    take_owned_c_string(ptr)
}

/// `ten_path_is_absolute()`.
pub fn is_absolute(path: &str) -> Result<bool, Error> {
    let c = CString::new(path).map_err(|_| Error::InvalidArgument)?;
    let result = unsafe { ten_sys::ten_path_is_absolute(c.as_ptr()) };
    Ok(result != 0)
}
