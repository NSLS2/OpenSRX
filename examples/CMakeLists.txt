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

add_executable(ReadCode
    ReadCode.cpp
)

target_link_libraries(ReadCode
    PRIVATE
        OpenSRX
        argparse::argparse
)

add_executable(ImageReadback
    ImageReadback.cpp
)

target_link_libraries(ImageReadback
    PRIVATE
        OpenSRX
        argparse::argparse
)

add_executable(AdjustParams
    AdjustParams.cpp
)

target_link_libraries(AdjustParams
    PRIVATE
        OpenSRX
        argparse::argparse
)

install(TARGETS VersionInfo ReadCode ImageReadback AdjustParams
    RUNTIME DESTINATION bin
)
