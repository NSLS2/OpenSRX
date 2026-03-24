include(FetchContent)

FetchContent_Declare(
    argparse
    URL https://github.com/p-ranav/argparse/archive/refs/tags/v3.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(argparse)

add_executable(VersionInfo
    VersionInfo.cpp
)

target_link_libraries(VersionInfo
    PRIVATE
        OpenSRX
        argparse::argparse
)
