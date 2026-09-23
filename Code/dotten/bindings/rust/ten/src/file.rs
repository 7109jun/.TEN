//! `ten/file.h`에 대한 safe wrapper.

use std::ffi::CString;
use std::os::raw::{c_int, c_void};

use crate::{last_error, Error};

/// `SEEK_SET`/`SEEK_CUR`/`SEEK_END`에 대응.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SeekFrom {
    Start,
    Current,
    End,
}

impl SeekFrom {
    fn to_raw(self) -> c_int {
        match self {
            SeekFrom::Start => 0,
            SeekFrom::Current => 1,
            SeekFrom::End => 2,
        }
    }
}

/// `.TEN`의 파일 핸들(`ten_file_t`)을 소유하는 safe wrapper.
/// `Drop` 시 `ten_file_close()`를 호출한다. NOT_THREAD_SAFE.
pub struct TenFile {
    raw: *mut ten_sys::ten_file_t,
}

impl TenFile {
    /// `ten_file_open()`. mode는 C 쪽과 동일하게 "r"/"r+"/"w"/"w+"/"a"/"a+"
    /// (+선택적 "b")를 지원한다.
    pub fn open(path: &str, mode: &str) -> Result<Self, Error> {
        let c_path = CString::new(path).map_err(|_| Error::InvalidArgument)?;
        let c_mode = CString::new(mode).map_err(|_| Error::InvalidArgument)?;

        let raw = unsafe { ten_sys::ten_file_open(c_path.as_ptr(), c_mode.as_ptr()) };
        if raw.is_null() {
            Err(last_error().unwrap_or(Error::Unknown))
        } else {
            Ok(TenFile { raw })
        }
    }

    /// `ten_file_read()`. 반환값은 실제로 읽은 바이트 수(0이면 EOF).
    pub fn read(&mut self, buf: &mut [u8]) -> usize {
        unsafe { ten_sys::ten_file_read(self.raw, buf.as_mut_ptr() as *mut c_void, buf.len()) }
    }

    /// `ten_file_write()`. data 전체가 기록되지 않으면 오류를 반환한다.
    pub fn write(&mut self, data: &[u8]) -> Result<usize, Error> {
        let n = unsafe {
            ten_sys::ten_file_write(self.raw, data.as_ptr() as *const c_void, data.len())
        };

        if n < data.len() {
            Err(last_error().unwrap_or(Error::Unknown))
        } else {
            Ok(n)
        }
    }

    /// `ten_file_seek()`.
    pub fn seek(&mut self, offset: i64, from: SeekFrom) -> Result<(), Error> {
        let code = unsafe { ten_sys::ten_file_seek(self.raw, offset, from.to_raw()) };
        match Error::from_code(code) {
            None => Ok(()),
            Some(e) => Err(e),
        }
    }

    /// `ten_file_size()`.
    pub fn size(&mut self) -> Result<i64, Error> {
        let size = unsafe { ten_sys::ten_file_size(self.raw) };
        if size < 0 {
            Err(last_error().unwrap_or(Error::Unknown))
        } else {
            Ok(size)
        }
    }
}

impl Drop for TenFile {
    fn drop(&mut self) {
        unsafe { ten_sys::ten_file_close(self.raw) }
    }
}

/// `ten_file_exists()`.
pub fn exists(path: &str) -> bool {
    match CString::new(path) {
        Ok(c) => unsafe { ten_sys::ten_file_exists(c.as_ptr()) != 0 },
        Err(_) => false,
    }
}

/// `ten_file_delete()`.
pub fn delete(path: &str) -> Result<(), Error> {
    let c = CString::new(path).map_err(|_| Error::InvalidArgument)?;
    let code = unsafe { ten_sys::ten_file_delete(c.as_ptr()) };
    match Error::from_code(code) {
        None => Ok(()),
        Some(e) => Err(e),
    }
}

/// `ten_file_copy()`.
pub fn copy(source: &str, destination: &str) -> Result<(), Error> {
    let cs = CString::new(source).map_err(|_| Error::InvalidArgument)?;
    let cd = CString::new(destination).map_err(|_| Error::InvalidArgument)?;
    let code = unsafe { ten_sys::ten_file_copy(cs.as_ptr(), cd.as_ptr()) };
    match Error::from_code(code) {
        None => Ok(()),
        Some(e) => Err(e),
    }
}

/// `ten_file_move()`. (Rust의 `move`는 예약어라 `move_file`로 이름 붙였다.)
pub fn move_file(source: &str, destination: &str) -> Result<(), Error> {
    let cs = CString::new(source).map_err(|_| Error::InvalidArgument)?;
    let cd = CString::new(destination).map_err(|_| Error::InvalidArgument)?;
    let code = unsafe { ten_sys::ten_file_move(cs.as_ptr(), cd.as_ptr()) };
    match Error::from_code(code) {
        None => Ok(()),
        Some(e) => Err(e),
    }
}
