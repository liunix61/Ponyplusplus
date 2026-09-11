cmake_minimum_required(VERSION 3.16)
project(ponyplusplus VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Options ----
option(BUILD_TESTING "Build tests" ON)
option(USE_ASAN "Address sanitizer" OFF)
option(USE_UBSAN "Undefined behavior sanitizer" OFF)
option(USE_TSAN "Thread sanitizer" OFF)
# TLS 支持 (可选, 需要 OpenSSL)
option(USE_TLS "Enable TLS support (requires OpenSSL)" OFF)
if(USE_TLS)
    find_package(OpenSSL REQUIRED)
    add_definitions(-DPONYPP_USE_TLS)
    list(APPEND PONYPP_LINK_LIBS OpenSSL::SSL OpenSSL::Crypto)
    message(STATUS "TLS support: ENABLED")
else()
    message(STATUS "TLS support: DISABLED (use -DUSE_TLS=ON)")
endif()

option(COVERAGE "Coverage flags" OFF)

# ---- Sanitizer / Coverage flags ----
if(USE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()
if(USE_UBSAN)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=undefined)
endif()
if(USE_TSAN)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=thread)
endif()
if(COVERAGE)
    add_compile_options(--coverage -fprofile-arcs -ftest-coverage -O0 -g)
    add_link_options(--coverage)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Wno-unused-function -march=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Wasm GC backend (requires wasmtime C API)
option(PONYPP_USE_WASM_GC "Enable Wasm GC backend via wasmtime" OFF)
if(PONYPP_USE_WASM_GC)
    set(WASMTIME_DIR ${CMAKE_SOURCE_DIR}/external/wasmtime-v27.0.0-aarch64-linux-c-api)
    add_compile_definitions(PONYPP_USE_WASM_GC)
    include_directories(${WASMTIME_DIR}/include)
    link_directories(${WASMTIME_DIR}/lib)
endif()

# ---- Sources ----
set(PONYPP_SRCS
    src/ast.c
    src/capabilities.c
    src/codegen.c
    src/lexer.c
    src/parser.c
    src/typecheck.c
    src/types.c
    src/util.c
    src/wasm.c
    src/wit.c
    src/ponypp/runtime.c src/ponypp/mpmc.c src/dwarf.c src/incremental.c src/ponypp/scheduler_ext.c src/wasm_gc.c
    src/ponypp/gc.c
    src/ponypp/wamr.c
    src/ponypp/stdlib.c
    src/ponypp/tool.c
    src/ponypp/repl.c
    src/ponypp/network.c
    src/ponypp/profiler.c
    src/ponypp/distributed.c
    src/ponypp/ffi.c
    src/ponypp/pkg.c
    src/browser.c
    src/debugger.c
    src/version_compat.c
    src/http.c
    src/crypto_openssl.c
    src/crypto_mbedtls.c
    src/crypto_stub.c
    src/regex.c
)

# ---- Crypto backend detection ----
# Priority: mbedTLS (MCU SDK) > OpenSSL (Linux/desktop) > stub (no crypto)
# mbedTLS: typically provided by ESP-IDF/STM32Cube, set MBEDTLS_ROOT to point to it
option(USE_MBEDTLS "Use mbedTLS crypto backend (MCU)" OFF)
option(USE_OPENSSL "Use OpenSSL crypto backend (Linux)" ON)

if(USE_MBEDTLS)
    find_path(MBEDTLS_INCLUDE_DIR mbedtls/sha256.h
        HINTS ${MBEDTLS_ROOT}/include /usr/include)
    find_library(MBEDTLS_CRYPTO_LIB mbedtls
        HINTS ${MBEDTLS_ROOT}/lib /usr/lib)
    if(MBEDTLS_INCLUDE_DIR AND MBEDTLS_CRYPTO_LIB)
        message(STATUS "Crypto backend: mbedTLS")
        add_compile_definitions(PONY_CRYPTO_MBEDTLS)
    else()
        message(FATAL_ERROR "mbedTLS requested but not found. Set MBEDTLS_ROOT.")
    endif()
elseif(USE_OPENSSL)
    find_package(OpenSSL QUIET)
    if(OpenSSL_FOUND)
        message(STATUS "Crypto backend: OpenSSL ${OPENSSL_VERSION}")
        add_compile_definitions(PONY_CRYPTO_OPENSSL)
    else()
        message(STATUS "Crypto backend: stub (OpenSSL not found)")
    endif()
else()
    message(STATUS "Crypto backend: stub (crypto disabled)")
endif()

# ---- Library (no driver, no main) ----
add_library(ponypp_lib STATIC ${PONYPP_SRCS})
target_include_directories(ponypp_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link crypto backend
if(USE_MBEDTLS AND MBEDTLS_CRYPTO_LIB)
    target_link_libraries(ponypp_lib PUBLIC ${MBEDTLS_CRYPTO_LIB})
    target_include_directories(ponypp_lib PUBLIC ${MBEDTLS_INCLUDE_DIR})
elseif(USE_OPENSSL AND OpenSSL_FOUND)
    target_link_libraries(ponypp_lib PUBLIC OpenSSL::Crypto)
endif()

# Link TLS support
if(USE_TLS)
    target_link_libraries(ponypp_lib PUBLIC OpenSSL::SSL OpenSSL::Crypto)
endif()

# Link Wasm GC backend
if(PONYPP_USE_WASM_GC)
    target_link_libraries(ponypp_lib PUBLIC wasmtime)
endif()

# ---- Compiler driver ----
add_executable(ponyppc src/ponyppc.c)
target_link_libraries(ponyppc PRIVATE ponypp_lib m dl)

# ---- GTest discovery ----
# Prefer locally built static libs (system GCC libstdc++, no conda mismatch)
set(GTEST_LIB_DIR /home/liunix/Ponyplusplus/third_party/gtest/lib)
set(GTEST_INCLUDE_DIR /home/liunix/nanoapi/build/_deps/googletest-src/googletest/include)

find_library(GTEST_LIB NAMES gtest PATHS "${GTEST_LIB_DIR}" NO_DEFAULT_PATH)
find_library(GTEST_MAIN_LIB NAMES gtest_main PATHS "${GTEST_LIB_DIR}" NO_DEFAULT_PATH)
find_library(GMOCK_LIB NAMES gmock PATHS "${GTEST_LIB_DIR}" NO_DEFAULT_PATH)

if(NOT GTEST_INCLUDE_DIR OR NOT GTEST_LIB OR NOT GTEST_MAIN_LIB)
    message(FATAL_ERROR "GTest not found. Install googletest or provide GTEST_ROOT.")
endif()

# ---- Tests ----
if(BUILD_TESTING)
    enable_testing()

    set(GTEST_EXECUTABLES gtest_lexer gtest_parser gtest_types gtest_typecheck
                          gtest_wasm gtest_wit gtest_codegen
                          gtest_codegen_extra gtest_codegen_cov gtest_extra gtest_coverage gtest_tc_extra gtest_more gtest_e2e gtest_util gtest_util2 gtest_runtime gtest_phase2 gtest_stdlib gtest_stdlib_phase3 gtest_stdlib_phase3_ext gtest_tool gtest_gc gtest_wamr gtest_browser gtest_debugger gtest_version_compat gtest_security gtest_http gtest_capabilities_full gtest_crypto gtest_regex gtest_gc_actor gtest_collections gtest_msgpack gtest_coverage_boost gtest_mpmc gtest_dsupervis gtest_dwarf gtest_lexer_full gtest_parser_full gtest_http_integration gtest_codegen_paths gtest_parser_edge gtest_codegen_complex gtest_caps_full gtest_codegen_advanced gtest_debugger_full gtest_codegen_extreme gtest_codegen_fullpath gtest_parser_extreme gtest_coverage_final gtest_incremental gtest_dist_runtime gtest_stdlib_utils gtest_stdlib_full gtest_codegen_coverage gtest_typecheck_full gtest_ffi_full gtest_dap_handlers gtest_stdlib_chan gtest_scheduler_ext gtest_stdlib_lang gtest_caps_coverage gtest_wasm_gc gtest_wamr_full gtest_tool_full gtest_network_full gtest_parser_cov2 gtest_dist_cov gtest_debugger_cov gtest_ffi_cov gtest_tool_cov gtest_wamr_cov gtest_wasm_cov gtest_tool_cov2 gtest_util_cov2 gtest_typecheck_cov2 gtest_codegen_cov2 gtest_wamr_cov2 gtest_debugger_cov2 gtest_dist_cov2 gtest_wasm_cov2 gtest_tool_cov3 gtest_typecheck_cov3 gtest_util_cov3 gtest_codegen_cov3 gtest_debugger_cov3 gtest_dist_cov3 gtest_wasm_cov3 gtest_tool_cov4 gtest_typecheck_cov4 gtest_util_cov4 gtest_codegen_cov4 gtest_debugger_cov4 gtest_dist_cov4 gtest_wasm_cov4 gtest_tool_cov5 gtest_typecheck_cov5 gtest_debugger_cov5 gtest_tool_cov6 gtest_wasm_cov5 gtest_debugger_cov6 gtest_typecheck_cov6 gtest_tool_cov7 gtest_dist_cov5 gtest_wamr_cov3 gtest_codegen_cov5 gtest_util_cov5 gtest_debugger_cov7 gtest_typecheck_cov7 gtest_wamr_cov4 gtest_tool_cov8)

    foreach(tname IN LISTS GTEST_EXECUTABLES)
        add_executable(${tname} tests/${tname}.cpp)
        target_include_directories(${tname} PRIVATE ${GTEST_INCLUDE_DIR})
        target_link_libraries(${tname} PRIVATE ponypp_lib ${GTEST_LIB} ${GTEST_MAIN_LIB} m pthread)
        add_test(NAME ${tname} COMMAND ${tname})
    endforeach()
endif()

# ---- Benchmarks ----
add_executable(bench_runtime benchmarks/bench_runtime.c)
target_link_libraries(bench_runtime PRIVATE ponypp_lib m pthread)

# ---- Install ----
install(TARGETS ponyppc ponypp_lib
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)
