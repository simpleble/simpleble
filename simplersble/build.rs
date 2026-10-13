use cmake;
use cxx_build;
use std::env;
use std::path::Path;

// SIMPLERSBLE_PLAIN=1 builds SimpleBLE with the simulated backend instead of the native one,
// so that tests can run without Bluetooth hardware. This is only meant for development and CI.
fn use_plain_backend() -> bool {
    match env::var("SIMPLERSBLE_PLAIN") {
        Ok(value) => matches!(value.to_ascii_lowercase().as_str(), "1" | "on" | "true" | "yes"),
        Err(_) => false,
    }
}

fn compile_simpleble(plain: bool) {
    let build_debug = env::var("DEBUG").unwrap() == "true";
    if build_debug { println!("cargo:warning=Building in DEBUG mode"); }
    if plain { println!("cargo:warning=Building with the simulated backend"); }

    let cargo_manifest_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let simplersble_source_path = Path::new(&cargo_manifest_dir).join("simplersble");

    // println!("cargo:warning=All Environment Variables:");
    // println!("cargo:warning=CWD: {}", env::current_dir().unwrap().display());
    // for (key, value) in env::vars() {
    //     println!("cargo:warning=ENV: {} - {}", key, value);
    // }

    // The simpleble library name depends if we're building in debug more or not.
    let simpleble_library_name = if build_debug {"simpleble-debug"} else {"simpleble"};

    let mut simpleble_config = cmake::Config::new("simpleble");
    simpleble_config.define("SIMPLEBLE_PLAIN", if plain {"ON"} else {"OFF"});
    if plain {
        // Use a separate build directory, so that switching flavors doesn't reconfigure the native build.
        simpleble_config.out_dir(Path::new(&env::var("OUT_DIR").unwrap()).join("plain"));
    }
    if env::var("CARGO_CFG_TARGET_ENV").unwrap_or_default() == "msvc" {
        // NOTE: Rust always links the release C runtime, so debug builds of SimpleBLE must use it too.
        let static_crt = env::var("CARGO_CFG_TARGET_FEATURE").unwrap_or_default().split(',').any(|f| f == "crt-static");
        simpleble_config.define("CMAKE_MSVC_RUNTIME_LIBRARY", if static_crt {"MultiThreaded"} else {"MultiThreadedDLL"});
        // NOTE: The cmake crate replaces CMake's default flags, which include exception unwinding.
        // Without it, a thrown exception skips destructors and can leave mutexes locked.
        simpleble_config.cxxflag("/EHsc");
    }
    let simpleble_build_dest = simpleble_config.build();
    let simpleble_include_path = Path::new(&simpleble_build_dest).join("include");

    cxx_build::CFG.exported_header_dirs.push(&simpleble_include_path);
    cxx_build::CFG.exported_header_dirs.push(&simplersble_source_path);

    println!("cargo:rustc-link-search=native={}/lib", simpleble_build_dest.display());
    println!("cargo:rustc-link-search=native={}/lib64", simpleble_build_dest.display());
    println!("cargo:rustc-link-lib=static={}", simpleble_library_name);

    let target_os = env::var("CARGO_CFG_TARGET_OS").unwrap();
    match target_os.as_str() {
        // The simulated backend doesn't need any platform Bluetooth libraries.
        "macos" | "windows" | "linux" if plain => {},
        "macos" => {
            println!("cargo:rustc-link-lib=framework=Foundation");
            println!("cargo:rustc-link-lib=framework=CoreBluetooth");
            println!("cargo:rustc-link-lib=framework=IOBluetooth");
            println!("cargo:rustc-link-lib=framework=IOKit");
        },
        "windows" => {
            println!("cargo:rustc-link-lib=setupapi");
        },
        "linux" => {
            println!("cargo:rustc-link-lib=dbus-1");
        },
        &_ => panic!("Unexpected target OS")
    }
}

fn main() {
    // TODO: Add all files that would trigger a rerun
    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/bindings/Bindings.hpp");
    println!("cargo:rerun-if-changed=src/bindings/Bindings.cpp");
    println!("cargo:rerun-if-env-changed=SIMPLERSBLE_PLAIN");

    let plain = use_plain_backend();
    println!("cargo:rustc-check-cfg=cfg(simplersble_plain)");
    if plain {
        println!("cargo:rustc-cfg=simplersble_plain");
    }

    compile_simpleble(plain);

    if std::env::var("DOCS_RS").is_ok() {
        println!("cargo:warning=Building DOCS");
    }

    // The bindings use the same backend macros as SimpleBLE to pick the advanced platform functions.
    let target_os = env::var("CARGO_CFG_TARGET_OS").unwrap();
    let backend = |os: &str| if !plain && target_os == os {"1"} else {"0"};

    // Build the bindings
    cxx_build::bridge("simplersble/src/lib.rs")
        .file("simplersble/src/bindings/Bindings.cpp")
        .std("c++17")
        .define("SIMPLEBLE_BACKEND_PLAIN", if plain {"1"} else {"0"})
        .define("SIMPLEBLE_BACKEND_LINUX", backend("linux"))
        .define("SIMPLEBLE_BACKEND_WINDOWS", backend("windows"))
        .define("SIMPLEBLE_BACKEND_MACOS", backend("macos"))
        .compile("simpleble_bindings");
}
