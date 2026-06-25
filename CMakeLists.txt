cmake_minimum_required(VERSION 3.16)

project(OpenSRX
    VERSION 0.1.2
    DESCRIPTION "C++ library for interfacing with Keyence SR series barcode readers"
    LANGUAGES CXX
)

include(FetchContent)
include(GNUInstallDirs)

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
    URL https://github.com/eclipse-ecal/fineftp-server/archive/refs/tags/v1.6.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_Declare(
    libjpeg_turbo
    URL https://github.com/libjpeg-turbo/libjpeg-turbo/archive/refs/tags/3.1.0.tar.gz
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
set(FINEFTP_SERVER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_GetProperties(fineftp)
if(NOT fineftp_POPULATED)
    FetchContent_Populate(fineftp)
    # Build fineftp as static so it's linked into libOpenSRX.so
    set(_saved_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(${fineftp_SOURCE_DIR} ${fineftp_BINARY_DIR} EXCLUDE_FROM_ALL)
    set(BUILD_SHARED_LIBS ${_saved_BUILD_SHARED_LIBS})
endif()

# Build libjpeg-turbo as a static library.
# libjpeg-turbo blocks add_subdirectory() with a guard check; patch it out
# so we can build it inline via FetchContent.
set(ENABLE_SHARED OFF CACHE BOOL "" FORCE)
set(ENABLE_STATIC ON CACHE BOOL "" FORCE)
set(WITH_TURBOJPEG OFF CACHE BOOL "" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
FetchContent_GetProperties(libjpeg_turbo)
if(NOT libjpeg_turbo_POPULATED)
    FetchContent_Populate(libjpeg_turbo)
    # Remove the add_subdirectory() guard
    file(READ "${libjpeg_turbo_SOURCE_DIR}/CMakeLists.txt" _ljt_cmake)
    string(REPLACE
        "if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)\n  message(FATAL_ERROR"
        "if(FALSE)\n  message(FATAL_ERROR"
        _ljt_cmake "${_ljt_cmake}")
    file(WRITE "${libjpeg_turbo_SOURCE_DIR}/CMakeLists.txt" "${_ljt_cmake}")
    add_subdirectory(${libjpeg_turbo_SOURCE_DIR} ${libjpeg_turbo_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

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
