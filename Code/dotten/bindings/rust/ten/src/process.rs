//! `ten/process.h`에 대한 safe wrapper.

use std::ffi::{CStr, CString};
use std::os::raw::c_char;

use crate::Error;

/// `ten_process_id()`.
pub fn process_id() -> u64 {
    unsafe { ten_sys::ten_process_id() }
}

/// `ten_process_execute()`. `program`이 argv[0]으로 전달되고 `args`가
/// 그 뒤에 이어진다. 자식 프로세스가 끝날 때까지 대기한다(동기 실행).
/// 반환값은 실행 성공 여부만 나타낸다 — `program`을 찾지 못해도 자식이
/// exec 실패로 종료할 뿐이므로 `Ok(())`가 돌아올 수 있다(C 헤더 문서
/// 참고). 실행 전에 존재를 확인하려면 `ten::file::exists()`를 쓴다.
pub fn execute(program: &str, args: &[&str]) -> Result<(), Error> {
    let c_program = CString::new(program).map_err(|_| Error::InvalidArgument)?;

    let mut c_args: Vec<CString> = Vec::with_capacity(args.len() + 1);
    c_args.push(CString::new(program).map_err(|_| Error::InvalidArgument)?);
    for a in args {
        c_args.push(CString::new(*a).map_err(|_| Error::InvalidArgument)?);
    }

    let mut argv: Vec<*const c_char> = c_args.iter().map(|c| c.as_ptr()).collect();
    argv.push(std::ptr::null());

    let code = unsafe { ten_sys::ten_process_execute(c_program.as_ptr(), argv.as_ptr()) };
    match Error::from_code(code) {
        None => Ok(()),
        Some(e) => Err(e),
    }
}

/// `ten_env_get()`.
pub fn env_get(name: &str) -> Option<String> {
    let c_name = CString::new(name).ok()?;
    let ptr = unsafe { ten_sys::ten_env_get(c_name.as_ptr()) };
    if ptr.is_null() {
        None
    } else {
        Some(
            unsafe { CStr::from_ptr(ptr) }
                .to_string_lossy()
                .into_owned(),
        )
    }
}

/// `ten_env_set()`.
pub fn env_set(name: &str, value: &str) -> Result<(), Error> {
    let c_name = CString::new(name).map_err(|_| Error::InvalidArgument)?;
    let c_value = CString::new(value).map_err(|_| Error::InvalidArgument)?;
    let code = unsafe { ten_sys::ten_env_set(c_name.as_ptr(), c_value.as_ptr()) };
    match Error::from_code(code) {
        None => Ok(()),
        Some(e) => Err(e),
    }
}
