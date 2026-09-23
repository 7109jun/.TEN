//! `ten/time.h`에 대한 safe wrapper. 전부 순수/무실패 함수라 자유
//! 함수로 그대로 노출한다.

/// `ten_time_now_ms()`.
pub fn now_ms() -> u64 {
    unsafe { ten_sys::ten_time_now_ms() }
}

/// `ten_time_now_us()`.
pub fn now_us() -> u64 {
    unsafe { ten_sys::ten_time_now_us() }
}

/// `ten_time_monotonic_ms()`.
pub fn monotonic_ms() -> u64 {
    unsafe { ten_sys::ten_time_monotonic_ms() }
}

/// `ten_time_unix()`.
pub fn unix_seconds() -> i64 {
    unsafe { ten_sys::ten_time_unix() }
}
