add_executable(VersionInfo
    VersionInfo.cpp
)

target_link_libraries(VersionInfo
    PRIVATE
        OpenSRX
)
