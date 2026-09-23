//! `ten/encoding.h`에 대한 safe wrapper.

use std::ffi::{CStr, CString};
use std::os::raw::c_void;

use crate::{last_error, Error};

fn take_owned_c_string(ptr: *mut std::os::raw::c_char) -> Result<String, Error> {
    if ptr.is_null() {
        return Err(last_error().unwrap_or(Error::Unknown));
    }
    let s = unsafe { CStr::from_ptr(ptr) }.to_string_lossy().into_owned();
    unsafe { ten_sys::ten_free(ptr as *mut c_void) };
    Ok(s)
}

fn take_owned_bytes(ptr: *mut c_void, len: usize) -> Vec<u8> {
    let slice = unsafe { std::slice::from_raw_parts(ptr as *const u8, len) };
    let owned = slice.to_vec();
    unsafe { ten_sys::ten_free(ptr) };
    owned
}

/// `ten_base64_encode()`.
pub fn base64_encode(data: &[u8]) -> Result<String, Error> {
    let ptr = unsafe { ten_sys::ten_base64_encode(data.as_ptr() as *const c_void, data.len()) };
    take_owned_c_string(ptr)
}

/// `ten_base64_decode()`.
pub fn base64_decode(text: &str) -> Result<Vec<u8>, Error> {
    let c_text = CString::new(text).map_err(|_| Error::Encoding)?;
    let mut size: usize = 0;
    let ptr = unsafe { ten_sys::ten_base64_decode(c_text.as_ptr(), &mut size) };
    if ptr.is_null() {
        Err(last_error().unwrap_or(Error::Unknown))
    } else {
        Ok(take_owned_bytes(ptr, size))
    }
}

/// `ten_hex_encode()`.
pub fn hex_encode(data: &[u8]) -> Result<String, Error> {
    let ptr = unsafe { ten_sys::ten_hex_encode(data.as_ptr() as *const c_void, data.len()) };
    take_owned_c_string(ptr)
}

/// `ten_hex_decode()`.
pub fn hex_decode(text: &str) -> Result<Vec<u8>, Error> {
    let c_text = CString::new(text).map_err(|_| Error::Encoding)?;
    let mut size: usize = 0;
    let ptr = unsafe { ten_sys::ten_hex_decode(c_text.as_ptr(), &mut size) };
    if ptr.is_null() {
        Err(last_error().unwrap_or(Error::Unknown))
    } else {
        Ok(take_owned_bytes(ptr, size))
    }
}

/// `ten_utf8_to_utf16()`.
pub fn utf8_to_utf16(text: &str) -> Result<Vec<u16>, Error> {
    let c_text = CString::new(text).map_err(|_| Error::Encoding)?;
    let mut out_length: usize = 0;
    let ptr = unsafe { ten_sys::ten_utf8_to_utf16(c_text.as_ptr(), &mut out_length) };
    if ptr.is_null() {
        return Err(last_error().unwrap_or(Error::Unknown));
    }
    let slice = unsafe { std::slice::from_raw_parts(ptr, out_length) };
    let owned = slice.to_vec();
    unsafe { ten_sys::ten_free(ptr as *mut c_void) };
    Ok(owned)
}

/// `ten_utf16_to_utf8()`.
pub fn utf16_to_utf8(units: &[u16]) -> Result<String, Error> {
    let ptr = unsafe { ten_sys::ten_utf16_to_utf8(units.as_ptr(), units.len()) };
    take_owned_c_string(ptr)
}

/// `ten_ascii_validate()`.
pub fn ascii_validate(data: &[u8]) -> bool {
    unsafe {
        ten_sys::ten_ascii_validate(data.as_ptr() as *const std::os::raw::c_char, data.len()) != 0
    }
}

/// `ten_utf8_validate()`.
pub fn utf8_validate(data: &[u8]) -> bool {
    unsafe {
        ten_sys::ten_utf8_validate(data.as_ptr() as *const std::os::raw::c_char, data.len()) != 0
    }
}
