if(BUILD_STATIC_LIB)
    set(OPENSRX_LIBRARY_TYPE STATIC)
else()
    set(OPENSRX_LIBRARY_TYPE SHARED)
endif()

add_library(OpenSRX ${OPENSRX_LIBRARY_TYPE}
    app.cpp
)

target_link_libraries(OpenSRX
    PUBLIC
        asio::asio
    spdlog::spdlog_header_only
)

target_include_directories(OpenSRX
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${OPENSRX_GENERATED_INCLUDE_DIR}
)

if(NOT BUILD_STATIC_LIB)
    set_target_properties(OpenSRX PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
endif()
