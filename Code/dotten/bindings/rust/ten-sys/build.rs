use std::path::PathBuf;

fn main() {
    // .TEN의 공식 빌드 시스템은 CMake이므로(사양서 41절),
    // Rust 바인딩도 core/CMakeLists.txt를 그대로 빌드해서 사용한다.
    let core_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../../core");

    let dst = cmake::Config::new(&core_dir)
        .define("TEN_BUILD_EXAMPLES", "OFF")
        .define("TEN_BUILD_TESTS", "OFF")
        .define("CMAKE_BUILD_TYPE", "Release")
        .build();

    // cmake crate는 install(...) 규칙에 따라 <dst>/lib, <dst>/include 등으로 설치한다.
    println!("cargo:rustc-link-search=native={}/lib", dst.display());
    println!("cargo:rustc-link-search=native={}/lib64", dst.display());
    println!("cargo:rustc-link-lib=static=ten");
    println!("cargo:rerun-if-changed={}", core_dir.display());
}
