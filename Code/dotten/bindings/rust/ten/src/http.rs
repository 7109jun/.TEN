//! `ten/http.h`에 대한 safe wrapper.
//!
//! `TenHttpRequest`/`TenHttpResponse`는 클라이언트 쪽에서 만들고 읽는
//! 소유 타입(`Drop`으로 자동 해제)이다. 서버 핸들러는 대신 부모(서버
//! 내부)가 소유한 값을 가리키는 `TenHttpRequestRef<'_>`(읽기 전용)와
//! `TenHttpResponseRef<'_>`(쓰기 가능)를 받는다 — 핸들러가 실수로
//! 해제하지 못하도록 `Drop`이 없다.

use std::ffi::{CStr, CString};
use std::marker::PhantomData;
use std::os::raw::{c_int, c_void};

use crate::{last_error, Error};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum HttpMethod {
    Get,
    Post,
    Put,
    Delete,
    Head,
    Options,
}

impl HttpMethod {
    fn to_raw(self) -> c_int {
        match self {
            HttpMethod::Get => ten_sys::TEN_HTTP_GET,
            HttpMethod::Post => ten_sys::TEN_HTTP_POST,
            HttpMethod::Put => ten_sys::TEN_HTTP_PUT,
            HttpMethod::Delete => ten_sys::TEN_HTTP_DELETE,
            HttpMethod::Head => ten_sys::TEN_HTTP_HEAD,
            HttpMethod::Options => ten_sys::TEN_HTTP_OPTIONS,
        }
    }

    fn from_raw(raw: c_int) -> HttpMethod {
        match raw {
            r if r == ten_sys::TEN_HTTP_POST => HttpMethod::Post,
            r if r == ten_sys::TEN_HTTP_PUT => HttpMethod::Put,
            r if r == ten_sys::TEN_HTTP_DELETE => HttpMethod::Delete,
            r if r == ten_sys::TEN_HTTP_HEAD => HttpMethod::Head,
            r if r == ten_sys::TEN_HTTP_OPTIONS => HttpMethod::Options,
            _ => HttpMethod::Get,
        }
    }
}

fn opt_string(ptr: *const std::os::raw::c_char) -> Option<String> {
    if ptr.is_null() {
        None
    } else {
        Some(unsafe { CStr::from_ptr(ptr) }.to_string_lossy().into_owned())
    }
}

// ---- 서버 핸들러가 받는, 소유권 없는 뷰 ----

/// 서버 핸들러에 전달되는, 읽기 전용 요청 뷰(소유권 없음).
#[derive(Clone, Copy)]
pub struct TenHttpRequestRef<'a> {
    raw: *const ten_sys::ten_http_request_t,
    _marker: PhantomData<&'a ten_sys::ten_http_request_t>,
}

impl<'a> TenHttpRequestRef<'a> {
    pub fn method(&self) -> HttpMethod {
        HttpMethod::from_raw(unsafe { ten_sys::ten_http_request_method(self.raw) })
    }

    pub fn path(&self) -> String {
        let ptr = unsafe { ten_sys::ten_http_request_path(self.raw) };
        opt_string(ptr).unwrap_or_default()
    }

    pub fn header(&self, name: &str) -> Option<String> {
        let c_name = CString::new(name).ok()?;
        opt_string(unsafe { ten_sys::ten_http_request_header(self.raw, c_name.as_ptr()) })
    }

    pub fn body(&self) -> &'a [u8] {
        let mut size: usize = 0;
        let ptr = unsafe { ten_sys::ten_http_request_body(self.raw, &mut size) };
        if ptr.is_null() || size == 0 {
            &[]
        } else {
            unsafe { std::slice::from_raw_parts(ptr as *const u8, size) }
        }
    }
}

/// 서버 핸들러에 전달되는, 쓰기 가능한 응답 뷰(소유권 없음 — 서버가
/// 직렬화해서 보낸 뒤 자신이 해제한다).
pub struct TenHttpResponseRef<'a> {
    raw: *mut ten_sys::ten_http_response_t,
    _marker: PhantomData<&'a mut ten_sys::ten_http_response_t>,
}

impl<'a> TenHttpResponseRef<'a> {
    pub fn set_status(&mut self, status: i32) {
        unsafe { ten_sys::ten_http_response_set_status(self.raw, status) };
    }

    pub fn set_header(&mut self, name: &str, value: &str) -> Result<(), Error> {
        let c_name = CString::new(name).map_err(|_| Error::InvalidArgument)?;
        let c_value = CString::new(value).map_err(|_| Error::InvalidArgument)?;
        let code = unsafe { ten_sys::ten_http_response_set_header(self.raw, c_name.as_ptr(), c_value.as_ptr()) };
        match Error::from_code(code) {
            None => Ok(()),
            Some(e) => Err(e),
        }
    }

    pub fn set_body(&mut self, data: &[u8]) -> Result<(), Error> {
        let code =
            unsafe { ten_sys::ten_http_response_set_body(self.raw, data.as_ptr() as *const c_void, data.len()) };
        match Error::from_code(code) {
            None => Ok(()),
            Some(e) => Err(e),
        }
    }
}

// ---- 클라이언트가 쓰는 소유 타입 ----

/// 클라이언트가 보낼 요청. `Drop` 시 `ten_http_request_destroy()`.
pub struct TenHttpRequest {
    raw: *mut ten_sys::ten_http_request_t,
}

impl TenHttpRequest {
    pub fn new(method: HttpMethod, path: &str) -> Result<Self, Error> {
        let c_path = CString::new(path).map_err(|_| Error::InvalidArgument)?;
        let raw = unsafe { ten_sys::ten_http_request_create(method.to_raw(), c_path.as_ptr()) };
        if raw.is_null() {
            Err(last_error().unwrap_or(Error::Unknown))
        } else {
            Ok(TenHttpRequest { raw })
        }
    }

    pub fn set_header(&mut self, name: &str, value: &str) -> Result<(), Error> {
        let c_name = CString::new(name).map_err(|_| Error::InvalidArgument)?;
        let c_value = CString::new(value).map_err(|_| Error::InvalidArgument)?;
        let code = unsafe { ten_sys::ten_http_request_set_header(self.raw, c_name.as_ptr(), c_value.as_ptr()) };
        match Error::from_code(code) {
            None => Ok(()),
            Some(e) => Err(e),
        }
    }

    pub fn set_body(&mut self, data: &[u8]) -> Result<(), Error> {
        let code =
            unsafe { ten_sys::ten_http_request_set_body(self.raw, data.as_ptr() as *const c_void, data.len()) };
        match Error::from_code(code) {
            None => Ok(()),
            Some(e) => Err(e),
        }
    }
}

impl Drop for TenHttpRequest {
    fn drop(&mut self) {
        unsafe { ten_sys::ten_http_request_destroy(self.raw) }
    }
}

/// `ten_http_client_send()`가 돌려주는 응답. `Drop` 시
/// `ten_http_response_destroy()`.
pub struct TenHttpResponse {
    raw: *mut ten_sys::ten_http_response_t,
}

impl TenHttpResponse {
    pub fn status(&self) -> i32 {
        unsafe { ten_sys::ten_http_response_status(self.raw) }
    }

    pub fn header(&self, name: &str) -> Option<String> {
        let c_name = CString::new(name).ok()?;
        opt_string(unsafe { ten_sys::ten_http_response_header(self.raw, c_name.as_ptr()) })
    }

    pub fn body(&self) -> &[u8] {
        let mut size: usize = 0;
        let ptr = unsafe { ten_sys::ten_http_response_body(self.raw, &mut size) };
        if ptr.is_null() || size == 0 {
            &[]
        } else {
            unsafe { std::slice::from_raw_parts(ptr as *const u8, size) }
        }
    }
}

impl Drop for TenHttpResponse {
    fn drop(&mut self) {
        unsafe { ten_sys::ten_http_response_destroy(self.raw) }
    }
}

/// `ten_http_client_send()`.
pub fn client_send(host: &str, port: u16, request: &TenHttpRequest) -> Result<TenHttpResponse, Error> {
    let c_host = CString::new(host).map_err(|_| Error::InvalidArgument)?;
    let raw = unsafe { ten_sys::ten_http_client_send(c_host.as_ptr(), port, request.raw) };
    if raw.is_null() {
        Err(last_error().unwrap_or(Error::Unknown))
    } else {
        Ok(TenHttpResponse { raw })
    }
}

// ---- 서버 ----

type HandlerBox = Box<dyn Fn(TenHttpRequestRef<'_>, TenHttpResponseRef<'_>) + Send + Sync>;

extern "C" fn trampoline(
    request: *const ten_sys::ten_http_request_t,
    response: *mut ten_sys::ten_http_response_t,
    userdata: *mut c_void,
) {
    let handler = unsafe { &*(userdata as *const HandlerBox) };
    let req_ref = TenHttpRequestRef {
        raw: request,
        _marker: PhantomData,
    };
    let resp_ref = TenHttpResponseRef {
        raw: response,
        _marker: PhantomData,
    };
    handler(req_ref, resp_ref);
}

/// `.TEN`의 콜백 기반 HTTP 서버(`ten_http_server_t`)를 소유하는 safe
/// wrapper. `Drop` 시 `ten_http_server_destroy()`. `run()`이 반환(즉
/// `stop()` 이후)한 뒤에 drop하는 것을 전제로 한다(C 헤더 문서와 동일).
pub struct TenHttpServer {
    raw: *mut ten_sys::ten_http_server_t,
    // 트램폴린이 참조하는 핸들러를 서버 수명 동안 살려 둔다.
    _handler: Box<HandlerBox>,
}

impl TenHttpServer {
    pub fn new<F>(host: &str, port: u16, handler: F) -> Result<Self, Error>
    where
        F: Fn(TenHttpRequestRef<'_>, TenHttpResponseRef<'_>) + Send + Sync + 'static,
    {
        let c_host = CString::new(host).map_err(|_| Error::InvalidArgument)?;
        let boxed: Box<HandlerBox> = Box::new(Box::new(handler));
        let userdata = boxed.as_ref() as *const HandlerBox as *mut c_void;

        let raw = unsafe { ten_sys::ten_http_server_create(c_host.as_ptr(), port, Some(trampoline), userdata) };

        if raw.is_null() {
            Err(last_error().unwrap_or(Error::Unknown))
        } else {
            Ok(TenHttpServer {
                raw,
                _handler: boxed,
            })
        }
    }

    /// `ten_http_server_run()`. 다른 스레드에서 `stop()`을 호출할 때까지
    /// 블로킹한다.
    pub fn run(&self) -> Result<(), Error> {
        let code = unsafe { ten_sys::ten_http_server_run(self.raw) };
        match Error::from_code(code) {
            None => Ok(()),
            Some(e) => Err(e),
        }
    }

    /// `ten_http_server_stop()`.
    pub fn stop(&self) {
        unsafe { ten_sys::ten_http_server_stop(self.raw) }
    }
}

impl Drop for TenHttpServer {
    fn drop(&mut self) {
        unsafe { ten_sys::ten_http_server_destroy(self.raw) }
    }
}

// run()/stop()은 C 쪽에서 mutex로 동기화되므로 여러 스레드에서 같은
// 서버를 참조해도(예: 한 스레드는 run(), 다른 스레드는 stop()) 안전하다.
unsafe impl Send for TenHttpServer {}
unsafe impl Sync for TenHttpServer {}
