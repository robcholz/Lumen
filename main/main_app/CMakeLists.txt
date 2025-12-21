set(RUST_CRATE_DIR ${CMAKE_CURRENT_LIST_DIR})
set(RUST_LIB_NAME main_app)
set(RUST_LIB ${RUST_CRATE_DIR}/target/riscv32imc-unknown-none-elf/release/lib${RUST_LIB_NAME}.a)

add_custom_command(
        OUTPUT ${RUST_LIB}
        COMMAND cargo build --release --manifest-path ${RUST_CRATE_DIR}/Cargo.toml
        WORKING_DIRECTORY ${RUST_CRATE_DIR}
        COMMENT "Building Rust static library (main_app)"
        VERBATIM
)

add_custom_target(main_app_rust ALL
        DEPENDS ${RUST_LIB}
)

add_library(main_app_lib STATIC IMPORTED GLOBAL)
set_target_properties(main_app_lib PROPERTIES
        IMPORTED_LOCATION ${RUST_LIB}
)

add_dependencies(main_app_lib main_app_rust)
