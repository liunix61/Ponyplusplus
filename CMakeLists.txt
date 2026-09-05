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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Wno-unused-function")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

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
    src/ponypp/runtime.c
    src/ponypp/stdlib.c
    src/ponypp/tool.c
)

# ---- Library (no driver, no main) ----
add_library(ponypp_lib STATIC ${PONYPP_SRCS})
target_include_directories(ponypp_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ---- Compiler driver ----
add_executable(ponyppc src/ponyppc.c)
target_link_libraries(ponyppc PRIVATE ponypp_lib m)

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
                          gtest_capabilities gtest_wasm gtest_wit gtest_codegen
                          gtest_codegen_extra gtest_codegen_cov gtest_extra gtest_coverage gtest_tc_extra gtest_more gtest_e2e gtest_util gtest_util2 gtest_runtime gtest_phase2 gtest_stdlib gtest_tool)

    foreach(tname IN LISTS GTEST_EXECUTABLES)
        add_executable(${tname} tests/${tname}.cpp)
        target_include_directories(${tname} PRIVATE ${GTEST_INCLUDE_DIR})
        target_link_libraries(${tname} PRIVATE ponypp_lib ${GTEST_LIB} ${GTEST_MAIN_LIB} m pthread)
        add_test(NAME ${tname} COMMAND ${tname})
    endforeach()
endif()

# ---- Install ----
install(TARGETS ponyppc ponypp_lib
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)
