cmake_minimum_required(VERSION 3.16)

project(OpenSRX
    VERSION 0.1.0
    DESCRIPTION "C++ library for interfacing with Keyence SR series barcode readers"
    LANGUAGES CXX
)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_STATIC_LIB "Build a static library instead of a shared library" OFF)
option(BUILD_EXAMPLES "Build example binaries" ON)
option(BUILD_TESTS "Build test targets" ON)

find_package(Threads REQUIRED)

FetchContent_Declare(
    asio
    URL https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-30-2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_Declare(
    spdlog
    URL https://github.com/gabime/spdlog/archive/refs/tags/v1.15.3.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_Declare(
    fineftp
    URL https://github.com/eclipse-ecal/fineftp-server/archive/refs/tags/v1.3.4.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
    if(POLICY CMP0169)
        cmake_policy(SET CMP0169 OLD)
    endif()
    FetchContent_Populate(asio)
endif()

FetchContent_MakeAvailable(spdlog)

# Set asio_INCLUDE_DIR so fineftp's Findasio.cmake finds our already-fetched Asio headers.
set(asio_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include" CACHE PATH "" FORCE)
FetchContent_MakeAvailable(fineftp)

add_library(asio_headers INTERFACE)
add_library(asio::asio ALIAS asio_headers)

target_include_directories(asio_headers
    INTERFACE
        ${asio_SOURCE_DIR}/asio/include
)

target_compile_definitions(asio_headers
    INTERFACE
        ASIO_STANDALONE
)

target_link_libraries(asio_headers
    INTERFACE
        Threads::Threads
)

set(OPENSRX_GENERATED_INCLUDE_DIR "${PROJECT_BINARY_DIR}/generated/include")
file(MAKE_DIRECTORY "${OPENSRX_GENERATED_INCLUDE_DIR}/OpenSRX")
configure_file(
    "${PROJECT_SOURCE_DIR}/include/OpenSRX/Version.hpp.in"
    "${OPENSRX_GENERATED_INCLUDE_DIR}/OpenSRX/Version.hpp"
    @ONLY
)

add_subdirectory(src)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
