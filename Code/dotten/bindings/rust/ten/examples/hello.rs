fn main() {
    ten::init().expect("ten_init failed");

    println!(".TEN version: {}", ten::version());
    println!("initialized: {}", ten::is_initialized());

    // 중복 초기화 -> 에러 확인
    match ten::init() {
        Ok(()) => unreachable!(),
        Err(e) => println!("expected error on double init: {e}"),
    }
    println!("last_error(): {:?}", ten::last_error());
    ten::clear_error();

    // TenString
    let mut s = ten::TenString::new("Hello").expect("string create failed");
    s.append(", .TEN!").expect("append failed");
    println!("string: {} (len={})", s, s.len());
    println!("contains 'TEN': {}", s.contains("TEN"));

    // TenBuffer
    let mut buf = ten::TenBuffer::new().expect("buffer create failed");
    buf.write(b"hello").expect("write failed");
    buf.write(b" world").expect("write failed");
    println!("buffer: {:?} (len={})", buf.as_slice(), buf.len());

    // Path / Time
    let joined = ten::path::join("a", "b").expect("join failed");
    println!("path::join(a, b) = {joined}");
    println!("time::now_ms() = {}", ten::time::now_ms());

    // Log
    ten::log::set_level(ten::log::LogLevel::Info);
    ten::log::info("hello from .TEN via Rust");

    // File
    {
        let mut f = ten::TenFile::open("ten_rust_hello.tmp", "wb").expect("file open failed");
        f.write(b"hi").expect("file write failed");
    }
    println!("file exists: {}", ten::file::exists("ten_rust_hello.tmp"));
    ten::file::delete("ten_rust_hello.tmp").ok();

    // ThreadPool
    let pool = ten::TenThreadPool::new(4).expect("pool create failed");
    for i in 0..8 {
        pool.submit(move || {
            println!("task {i} ran");
        })
        .expect("submit failed");
    }
    drop(pool); // 큐가 빌 때까지 기다렸다가 워커를 종료한다

    // Process
    println!("pid = {}", ten::process::process_id());
    println!("PATH set: {}", ten::process::env_get("PATH").is_some());

    // Encoding
    let encoded = ten::encoding::base64_encode(b"Hello, .TEN!").unwrap();
    println!("base64: {encoded}");

    // JSON
    let mut obj = ten::TenJson::object().expect("json object failed");
    obj.object_set("name", ten::TenJson::string(".TEN").unwrap())
        .unwrap();
    obj.object_set("version", ten::TenJson::number(1.0).unwrap())
        .unwrap();
    println!("json: {}", obj.stringify().unwrap());

    // HTTP: 백그라운드 스레드에서 서버를 돌리고 클라이언트로 요청
    let server = std::sync::Arc::new(
        ten::TenHttpServer::new("127.0.0.1", 18080, |req, mut resp| {
            println!("server got {:?} {}", req.method(), req.path());
            resp.set_status(200);
            resp.set_body(b"hi from .TEN").ok();
        })
        .expect("http server create failed"),
    );
    let server_for_thread = std::sync::Arc::clone(&server);
    let server_thread = std::thread::spawn(move || {
        server_for_thread.run().ok();
    });
    std::thread::sleep(std::time::Duration::from_millis(50));

    let req = ten::TenHttpRequest::new(ten::HttpMethod::Get, "/hello").unwrap();
    let resp = ten::http::client_send("127.0.0.1", 18080, &req).expect("http request failed");
    println!("client got {} -> {:?}", resp.status(), std::str::from_utf8(resp.body()));

    server.stop();
    server_thread.join().ok();

    ten::shutdown();
    println!("initialized after shutdown: {}", ten::is_initialized());
}
