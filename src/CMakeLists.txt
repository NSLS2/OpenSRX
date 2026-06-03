if(BUILD_STATIC_LIB)
    set(OPENSRX_LIBRARY_TYPE STATIC)
else()
    set(OPENSRX_LIBRARY_TYPE SHARED)
endif()

add_library(OpenSRX ${OPENSRX_LIBRARY_TYPE}
    AsioInterface.cpp
    Image.cpp
    ImageServer.cpp
    ParamTraits.cpp
    Scanner.cpp
    SerialInterface.cpp
    SocketInterface.cpp
    Timestamp.cpp
    Utils.cpp
    ICommInterface.cpp
)

target_link_libraries(OpenSRX
    PUBLIC
        asio::asio
        spdlog::spdlog_header_only
        fineftp::server
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

install(TARGETS OpenSRX
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/OpenSRX
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY ${OPENSRX_GENERATED_INCLUDE_DIR}/OpenSRX
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
