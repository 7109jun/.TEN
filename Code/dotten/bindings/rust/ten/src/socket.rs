//! `ten/socket.h`에 대한 safe wrapper.

use std::ffi::{CStr, CString};
use std::os::raw::c_void;

use crate::{last_error, Error};

/// `.TEN`의 소켓(`ten_socket_t`)을 소유하는 safe wrapper — TCP/UDP
/// 공용. `Drop` 시 `ten_socket_close()`를 호출한다. C 쪽 문서대로
/// NOT_THREAD_SAFE(같은 소켓을 여러 스레드에서 동시에 send/receive하지
/// 않는다)이지만, 소켓 자체를 다른 스레드로 옮겨 쓰는 것은 안전하다.
pub struct TenSocket {
    raw: *mut ten_sys::ten_socket_t,
}

impl TenSocket {
    fn from_raw(raw: *mut ten_sys::ten_socket_t) -> Result<Self, Error> {
        if raw.is_null() {
            Err(last_error().unwrap_or(Error::Unknown))
        } else {
            Ok(TenSocket { raw })
        }
    }

    /// `ten_tcp_connect()`.
    pub fn tcp_connect(host: &str, port: u16) -> Result<Self, Error> {
        let c_host = CString::new(host).map_err(|_| Error::InvalidArgument)?;
        let raw = unsafe { ten_sys::ten_tcp_connect(c_host.as_ptr(), port) };
        Self::from_raw(raw)
    }

    /// `ten_tcp_listen()`.
    pub fn tcp_listen(host: &str, port: u16, backlog: i32) -> Result<Self, Error> {
        let c_host = CString::new(host).map_err(|_| Error::InvalidArgument)?;
        let raw = unsafe { ten_sys::ten_tcp_listen(c_host.as_ptr(), port, backlog) };
        Self::from_raw(raw)
    }

    /// `ten_tcp_accept()`.
    pub fn tcp_accept(&self) -> Result<Self, Error> {
        let raw = unsafe { ten_sys::ten_tcp_accept(self.raw) };
        Self::from_raw(raw)
    }

    /// `ten_udp_bind()`.
    pub fn udp_bind(host: &str, port: u16) -> Result<Self, Error> {
        let c_host = CString::new(host).map_err(|_| Error::InvalidArgument)?;
        let raw = unsafe { ten_sys::ten_udp_bind(c_host.as_ptr(), port) };
        Self::from_raw(raw)
    }

    /// `ten_udp_socket()`.
    pub fn udp_socket() -> Result<Self, Error> {
        let raw = unsafe { ten_sys::ten_udp_socket() };
        Self::from_raw(raw)
    }

    /// `ten_udp_send_to()`.
    pub fn udp_send_to(&self, host: &str, port: u16, data: &[u8]) -> Result<i64, Error> {
        let c_host = CString::new(host).map_err(|_| Error::InvalidArgument)?;
        let n = unsafe {
            ten_sys::ten_udp_send_to(
                self.raw,
                c_host.as_ptr(),
                port,
                data.as_ptr() as *const c_void,
                data.len(),
            )
        };
        if n < 0 {
            Err(last_error().unwrap_or(Error::Network))
        } else {
            Ok(n)
        }
    }

    /// `ten_udp_receive_from()`. `(받은 바이트 수, 발신자 host, 발신자 port)`.
    pub fn udp_receive_from(&self, buf: &mut [u8]) -> Result<(i64, String, u16), Error> {
        let mut host_buf = vec![0u8; 256];
        let mut port: u16 = 0;

        let n = unsafe {
            ten_sys::ten_udp_receive_from(
                self.raw,
                buf.as_mut_ptr() as *mut c_void,
                buf.len(),
                host_buf.as_mut_ptr() as *mut std::os::raw::c_char,
                host_buf.len(),
                &mut port,
            )
        };

        if n < 0 {
            return Err(last_error().unwrap_or(Error::Network));
        }

        let host = unsafe { CStr::from_ptr(host_buf.as_ptr() as *const std::os::raw::c_char) }
            .to_string_lossy()
            .into_owned();

        Ok((n, host, port))
    }

    /// `ten_socket_send()`.
    pub fn send(&self, data: &[u8]) -> Result<i64, Error> {
        let n = unsafe { ten_sys::ten_socket_send(self.raw, data.as_ptr() as *const c_void, data.len()) };
        if n < 0 {
            Err(last_error().unwrap_or(Error::Network))
        } else {
            Ok(n)
        }
    }

    /// `ten_socket_receive()`. `0`은 상대가 연결을 닫았음을 뜻한다.
    pub fn receive(&self, buf: &mut [u8]) -> Result<i64, Error> {
        let n = unsafe { ten_sys::ten_socket_receive(self.raw, buf.as_mut_ptr() as *mut c_void, buf.len()) };
        if n < 0 {
            Err(last_error().unwrap_or(Error::Network))
        } else {
            Ok(n)
        }
    }
}

impl Drop for TenSocket {
    fn drop(&mut self) {
        unsafe { ten_sys::ten_socket_close(self.raw) }
    }
}

// 소켓 자체를 다른 스레드로 옮겨 쓰는 것(예: accept한 연결을 워커
// 스레드로 넘기기)은 안전하다 — 동시 사용만 호출자가 피해야 한다.
unsafe impl Send for TenSocket {}
