//! `ten/json.h`에 대한 safe wrapper.
//!
//! `TenJson`은 최상위(parse/create로 얻은) 값을 소유하며 `Drop` 시
//! `ten_json_destroy()`를 호출한다. array/object의 자식 값은 부모가
//! 소유하고 있으므로 별도로 해제하면 안 된다 — 그래서 조회용
//! accessor(`array_get`/`object_get` 등)는 소유권이 없는 `TenJsonRef<'_>`
//! 를 돌려주며, 이 타입은 `Drop`을 구현하지 않고 부모의 수명에 묶인다.

use std::ffi::{CStr, CString};
use std::marker::PhantomData;
use std::os::raw::c_int;

use crate::{last_error, Error};

type RawJson = *mut ten_sys::ten_json_t;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum JsonType {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object,
}

impl JsonType {
    fn from_raw(raw: c_int) -> JsonType {
        match raw {
            r if r == ten_sys::TEN_JSON_BOOL => JsonType::Bool,
            r if r == ten_sys::TEN_JSON_NUMBER => JsonType::Number,
            r if r == ten_sys::TEN_JSON_STRING => JsonType::String,
            r if r == ten_sys::TEN_JSON_ARRAY => JsonType::Array,
            r if r == ten_sys::TEN_JSON_OBJECT => JsonType::Object,
            _ => JsonType::Null,
        }
    }
}

// ---- 읽기 전용 로직(raw 포인터 기준, TenJson/TenJsonRef가 공유) ----

fn raw_type(raw: RawJson) -> JsonType {
    JsonType::from_raw(unsafe { ten_sys::ten_json_type(raw) })
}

fn raw_get_bool(raw: RawJson) -> Result<bool, Error> {
    if raw_type(raw) != JsonType::Bool {
        return Err(Error::InvalidState);
    }
    Ok(unsafe { ten_sys::ten_json_get_bool(raw) } != 0)
}

fn raw_get_number(raw: RawJson) -> Result<f64, Error> {
    if raw_type(raw) != JsonType::Number {
        return Err(Error::InvalidState);
    }
    Ok(unsafe { ten_sys::ten_json_get_number(raw) })
}

fn raw_get_string(raw: RawJson) -> Result<String, Error> {
    if raw_type(raw) != JsonType::String {
        return Err(Error::InvalidState);
    }
    let ptr = unsafe { ten_sys::ten_json_get_string(raw) };
    Ok(unsafe { CStr::from_ptr(ptr) }.to_string_lossy().into_owned())
}

fn raw_array_count(raw: RawJson) -> usize {
    unsafe { ten_sys::ten_json_array_count(raw) }
}

fn raw_array_get(raw: RawJson, index: usize) -> Option<RawJson> {
    let child = unsafe { ten_sys::ten_json_array_get(raw, index) };
    if child.is_null() {
        None
    } else {
        Some(child)
    }
}

fn raw_object_count(raw: RawJson) -> usize {
    unsafe { ten_sys::ten_json_object_count(raw) }
}

fn raw_object_get(raw: RawJson, key: &str) -> Option<RawJson> {
    let c_key = match CString::new(key) {
        Ok(k) => k,
        Err(_) => return None,
    };
    let child = unsafe { ten_sys::ten_json_object_get(raw, c_key.as_ptr()) };
    if child.is_null() {
        None
    } else {
        Some(child)
    }
}

fn raw_object_key_at(raw: RawJson, index: usize) -> Option<String> {
    let ptr = unsafe { ten_sys::ten_json_object_key_at(raw, index) };
    if ptr.is_null() {
        None
    } else {
        Some(unsafe { CStr::from_ptr(ptr) }.to_string_lossy().into_owned())
    }
}

/// array/object 자식 값에 대한 **소유권 없는** 참조. 부모(`TenJson`)보다
/// 오래 살 수 없다(수명 `'a`로 강제).
#[derive(Clone, Copy)]
pub struct TenJsonRef<'a> {
    raw: RawJson,
    _marker: PhantomData<&'a ten_sys::ten_json_t>,
}

impl<'a> TenJsonRef<'a> {
    pub fn json_type(&self) -> JsonType {
        raw_type(self.raw)
    }
    pub fn as_bool(&self) -> Result<bool, Error> {
        raw_get_bool(self.raw)
    }
    pub fn as_number(&self) -> Result<f64, Error> {
        raw_get_number(self.raw)
    }
    pub fn as_str(&self) -> Result<String, Error> {
        raw_get_string(self.raw)
    }
    pub fn array_count(&self) -> usize {
        raw_array_count(self.raw)
    }
    pub fn array_get(&self, index: usize) -> Option<TenJsonRef<'a>> {
        raw_array_get(self.raw, index).map(|raw| TenJsonRef {
            raw,
            _marker: PhantomData,
        })
    }
    pub fn object_count(&self) -> usize {
        raw_object_count(self.raw)
    }
    pub fn object_get(&self, key: &str) -> Option<TenJsonRef<'a>> {
        raw_object_get(self.raw, key).map(|raw| TenJsonRef {
            raw,
            _marker: PhantomData,
        })
    }
    pub fn object_key_at(&self, index: usize) -> Option<String> {
        raw_object_key_at(self.raw, index)
    }
}

/// `.TEN`의 JSON 값(`ten_json_t`)을 소유하는 safe wrapper. `Drop` 시
/// `ten_json_destroy()`를 호출한다(자식 값까지 재귀적으로 함께 해제).
pub struct TenJson {
    raw: RawJson,
}

impl TenJson {
    /// `ten_json_parse()`.
    pub fn parse(text: &str) -> Result<Self, Error> {
        let c_text = CString::new(text).map_err(|_| Error::Parse)?;
        let raw = unsafe { ten_sys::ten_json_parse(c_text.as_ptr()) };
        if raw.is_null() {
            Err(last_error().unwrap_or(Error::Unknown))
        } else {
            Ok(TenJson { raw })
        }
    }

    /// `ten_json_stringify()`.
    pub fn stringify(&self) -> Result<String, Error> {
        let ptr = unsafe { ten_sys::ten_json_stringify(self.raw) };
        if ptr.is_null() {
            return Err(last_error().unwrap_or(Error::Unknown));
        }
        let s = unsafe { CStr::from_ptr(ptr) }.to_string_lossy().into_owned();
        unsafe { ten_sys::ten_free(ptr as *mut std::os::raw::c_void) };
        Ok(s)
    }

    pub fn null() -> Result<Self, Error> {
        Self::from_raw(unsafe { ten_sys::ten_json_create_null() })
    }
    pub fn bool(value: bool) -> Result<Self, Error> {
        Self::from_raw(unsafe { ten_sys::ten_json_create_bool(if value { 1 } else { 0 }) })
    }
    pub fn number(value: f64) -> Result<Self, Error> {
        Self::from_raw(unsafe { ten_sys::ten_json_create_number(value) })
    }
    pub fn string(value: &str) -> Result<Self, Error> {
        let c_value = CString::new(value).map_err(|_| Error::InvalidArgument)?;
        Self::from_raw(unsafe { ten_sys::ten_json_create_string(c_value.as_ptr()) })
    }
    pub fn array() -> Result<Self, Error> {
        Self::from_raw(unsafe { ten_sys::ten_json_create_array() })
    }
    pub fn object() -> Result<Self, Error> {
        Self::from_raw(unsafe { ten_sys::ten_json_create_object() })
    }

    fn from_raw(raw: RawJson) -> Result<Self, Error> {
        if raw.is_null() {
            Err(last_error().unwrap_or(Error::Unknown))
        } else {
            Ok(TenJson { raw })
        }
    }

    pub fn json_type(&self) -> JsonType {
        raw_type(self.raw)
    }
    pub fn as_bool(&self) -> Result<bool, Error> {
        raw_get_bool(self.raw)
    }
    pub fn as_number(&self) -> Result<f64, Error> {
        raw_get_number(self.raw)
    }
    pub fn as_str(&self) -> Result<String, Error> {
        raw_get_string(self.raw)
    }
    pub fn array_count(&self) -> usize {
        raw_array_count(self.raw)
    }
    pub fn array_get(&self, index: usize) -> Option<TenJsonRef<'_>> {
        raw_array_get(self.raw, index).map(|raw| TenJsonRef {
            raw,
            _marker: PhantomData,
        })
    }
    pub fn object_count(&self) -> usize {
        raw_object_count(self.raw)
    }
    pub fn object_get(&self, key: &str) -> Option<TenJsonRef<'_>> {
        raw_object_get(self.raw, key).map(|raw| TenJsonRef {
            raw,
            _marker: PhantomData,
        })
    }
    pub fn object_key_at(&self, index: usize) -> Option<String> {
        raw_object_key_at(self.raw, index)
    }

    /// `ten_json_array_add()`. 성공하면 `value`의 소유권이 `self`로
    /// 넘어간다(더 이상 `value`를 쓰면 안 되므로 이 함수가 값을 그대로
    /// 소비한다). 실패하면 `value`는 정상적으로 drop된다.
    pub fn array_add(&mut self, value: TenJson) -> Result<(), Error> {
        let code = unsafe { ten_sys::ten_json_array_add(self.raw, value.raw) };
        match Error::from_code(code) {
            None => {
                std::mem::forget(value); // 소유권이 C 쪽(self)으로 넘어갔다.
                Ok(())
            }
            Some(e) => Err(e), // value는 여기서 정상적으로 drop된다.
        }
    }

    /// `ten_json_object_set()`. 소유권 규칙은 `array_add`와 같다.
    pub fn object_set(&mut self, key: &str, value: TenJson) -> Result<(), Error> {
        let c_key = match CString::new(key) {
            Ok(k) => k,
            Err(_) => return Err(Error::InvalidArgument),
        };
        let code = unsafe { ten_sys::ten_json_object_set(self.raw, c_key.as_ptr(), value.raw) };
        match Error::from_code(code) {
            None => {
                std::mem::forget(value);
                Ok(())
            }
            Some(e) => Err(e),
        }
    }
}

impl Drop for TenJson {
    fn drop(&mut self) {
        unsafe { ten_sys::ten_json_destroy(self.raw) }
    }
}
