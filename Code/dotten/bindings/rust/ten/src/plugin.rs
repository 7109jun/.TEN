//! `ten/plugin.h`에 대한 safe wrapper.
//!
//! 로드/언로드/ABI 진입점 호출은 safe하지만, 찾은 심볼을 실제 함수로
//! 캐스팅해서 호출하는 것은 원리적으로 안전할 수 없다(호출자가 그
//! 심볼의 실제 시그니처를 책임져야 한다) — Rust의 `libloading` 크레이트
//! 등과 같은 이유로, `symbol()`은 raw 포인터를 돌려주고 캐스팅/호출은
//! `unsafe`로 남긴다.

use std::ffi::CString;
use std::os::raw::c_void;

use crate::{last_error, Error};

/// `.TEN`이 로드한 동적 라이브러리(`ten_plugin_t`)를 소유하는 safe
/// wrapper. `Drop` 시 `ten_plugin_unload()`를 호출한다.
pub struct TenPlugin {
    raw: *mut ten_sys::ten_plugin_t,
}

impl TenPlugin {
    /// `ten_plugin_load()`.
    pub fn load(path: &str) -> Result<Self, Error> {
        let c_path = CString::new(path).map_err(|_| Error::InvalidArgument)?;
        let raw = unsafe { ten_sys::ten_plugin_load(c_path.as_ptr()) };
        if raw.is_null() {
            Err(last_error().unwrap_or(Error::Unknown))
        } else {
            Ok(TenPlugin { raw })
        }
    }

    /// `ten_plugin_symbol()`. 반환된 포인터를 실제로 호출하는 것은
    /// `unsafe`다 — 그 심볼의 시그니처가 호출자가 기대하는 것과 같은지는
    /// Rust가 검증해 줄 수 없다.
    pub fn symbol(&self, name: &str) -> Result<*mut c_void, Error> {
        let c_name = CString::new(name).map_err(|_| Error::InvalidArgument)?;
        let sym = unsafe { ten_sys::ten_plugin_symbol(self.raw, c_name.as_ptr()) };
        if sym.is_null() {
            Err(last_error().unwrap_or(Error::NotFound))
        } else {
            Ok(sym)
        }
    }

    /// `ten_plugin_call_init()` — `"ten_plugin_init"` 심볼을 찾아 호출한다.
    /// 반환값은 host가 그 심볼을 찾지 못했을 때만 우리 오류 코드(주로
    /// `Error::NotSupported`)이고, 심볼을 찾아 호출했다면 **플러그인
    /// 자신의 `ten_plugin_init()`이 반환한 임의의 int 그대로**다 — C
    /// API 자체가 이 둘을 값만으로 구분해 주지 않으므로(같은 정수
    /// 공간을 공유), 심볼을 못 찾은 경우인지 구분하려면 `symbol()`을
    /// 먼저 호출해 보는 것을 권장한다.
    pub fn call_init(&self) -> i32 {
        unsafe { ten_sys::ten_plugin_call_init(self.raw) }
    }

    /// `ten_plugin_call_shutdown()` — `"ten_plugin_shutdown"` 심볼이
    /// 있으면 호출한다(없어도 오류 아님).
    pub fn call_shutdown(&self) {
        unsafe { ten_sys::ten_plugin_call_shutdown(self.raw) }
    }
}

impl Drop for TenPlugin {
    fn drop(&mut self) {
        unsafe { ten_sys::ten_plugin_unload(self.raw) }
    }
}
