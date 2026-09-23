//! `ten/thread_pool.h`에 대한 safe wrapper.
//!
//! `ten_thread`/`ten_mutex`/`ten_semaphore`/`ten_cond` 자체는 Rust의
//! `std::thread`/`std::sync`가 이미 더 안전하고 관용적인 대안을
//! 제공하므로 별도 safe wrapper를 만들지 않았다(raw 바인딩은
//! `ten_sys`에 있음 — Plugin ABI 등 C 쪽과 핸들을 주고받아야 하는
//! 경우에 쓴다). `ten_thread_pool`은 `.TEN` 고유의 구성 요소라 여기서
//! safe wrapper를 제공한다.

use std::os::raw::c_void;

use crate::{last_error, Error};

type BoxedTask = Box<dyn FnOnce() + Send>;

extern "C" fn trampoline(userdata: *mut c_void) -> *mut c_void {
    let task: Box<BoxedTask> = unsafe { Box::from_raw(userdata as *mut BoxedTask) };
    task();
    std::ptr::null_mut()
}

/// `.TEN`의 워커 스레드 풀(`ten_thread_pool_t`)을 소유하는 safe wrapper.
/// `Drop` 시 `ten_thread_pool_destroy()`를 호출한다(대기 중인 작업을
/// 모두 처리한 뒤 워커를 종료).
pub struct TenThreadPool {
    raw: *mut ten_sys::ten_thread_pool_t,
}

impl TenThreadPool {
    /// `ten_thread_pool_create()`.
    pub fn new(worker_count: usize) -> Result<Self, Error> {
        let raw = unsafe { ten_sys::ten_thread_pool_create(worker_count) };
        if raw.is_null() {
            Err(last_error().unwrap_or(Error::Unknown))
        } else {
            Ok(TenThreadPool { raw })
        }
    }

    /// `ten_thread_pool_submit()`. 클로저는 `'static + Send`여야 한다
    /// (다른 스레드에서, 이 호출 이후 임의의 시점에 실행되므로).
    pub fn submit<F>(&self, task: F) -> Result<(), Error>
    where
        F: FnOnce() + Send + 'static,
    {
        let boxed: Box<BoxedTask> = Box::new(Box::new(task));
        let ptr = Box::into_raw(boxed) as *mut c_void;

        let code = unsafe { ten_sys::ten_thread_pool_submit(self.raw, Some(trampoline), ptr) };
        match Error::from_code(code) {
            None => Ok(()),
            Some(e) => {
                // 제출이 거부됐으니 소유권을 되찾아 메모리 누수를 막는다.
                unsafe { drop(Box::from_raw(ptr as *mut BoxedTask)) };
                Err(e)
            }
        }
    }
}

impl Drop for TenThreadPool {
    fn drop(&mut self) {
        unsafe { ten_sys::ten_thread_pool_destroy(self.raw) }
    }
}

// C 쪽 ten_thread_pool_*는 내부적으로 mutex/cond로 동기화되므로 여러
// 스레드에서 동시에 &self로 submit()을 호출해도 안전하다.
unsafe impl Send for TenThreadPool {}
unsafe impl Sync for TenThreadPool {}
