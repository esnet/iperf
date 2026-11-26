cmake_minimum_required( VERSION 3.8 )

cmake_policy( SET CMP0068 NEW )

set( INTERFACE_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/iperf.h
        ${CMAKE_CURRENT_SOURCE_DIR}/iperf_api.h
        ${CMAKE_CURRENT_SOURCE_DIR}/iperf_locale.h
        ${CMAKE_CURRENT_SOURCE_DIR}/queue.h
        ${CMAKE_CURRENT_SOURCE_DIR}/timer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/cjson.h
        ${CMAKE_CURRENT_BINARY_DIR}/iperf_config.h )

set( SOURCE_FILES
        cjson.c
        iperf_api.c
        iperf_error.c
        iperf_auth.c
        iperf_client_api.c
        iperf_locale.c
        iperf_server_api.c
        iperf_tcp.c
        iperf_udp.c
        iperf_sctp.c
        iperf_util.c
        dscp.c
        net.c
        tcp_info.c
        timer.c
        units.c )

configure_file( iperf_config-cmake.h.in iperf_config.h )
configure_file( version.h.in version.h @ONLY )

add_library( iperf_objects OBJECT ${INTERFACE_HEADERS} ${SOURCE_FILES} )
if( BUILD_SHARED OR BUILD_FRAMEWORK )
    string( REPLACE "-Wl,-headerpad_max_install_names" "" CMAKE_C_LINK_FLAGS ${CMAKE_C_LINK_FLAGS} )
    string( REPLACE "-Wl,-headerpad_max_install_names" "" CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS ${CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS} )
    set_target_properties( iperf_objects
        PROPERTIES
            POSITION_INDEPENDENT_CODE on )
endif()

if( APPLE AND BITCODE_ENABLED )
    target_compile_options( iperf_objects PUBLIC -fembed-bitcode )
endif()
target_include_directories( iperf_objects
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR} )

if( BUILD_STATIC )
    add_library( iperf_static_lib STATIC ${INTERFACE_HEADERS} $<TARGET_OBJECTS:iperf_objects> )
    set_target_properties( iperf_static_lib
        PROPERTIES
            OUTPUT_NAME iperf
            PUBLIC_HEADER "${INTERFACE_HEADERS}" )

    install( TARGETS iperf_static_lib
            ARCHIVE DESTINATION lib
            PUBLIC_HEADER DESTINATION include )

    add_custom_command( TARGET iperf_static_lib POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/build/lib"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/build/include"
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:iperf_static_lib> "${CMAKE_BINARY_DIR}/build/lib"
            COMMAND ${CMAKE_COMMAND} -E copy ${INTERFACE_HEADERS} "${CMAKE_BINARY_DIR}/build/include" )

    if ( BUILD_STATIC_EXE )
        add_executable( iperf_static_exe main.c )
        target_include_directories( iperf_static_exe
                PRIVATE
                    ${CMAKE_CURRENT_BINARY_DIR} )

        target_link_libraries( iperf_static_exe iperf_static_lib )
        set_target_properties( iperf_static_exe
            PROPERTIES
                OUTPUT_NAME iperf3-static )

        install( TARGETS iperf_static_exe
                RUNTIME DESTINATION sbin )

        add_custom_command( TARGET iperf_static_exe POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/build/bin"
                COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:iperf_static_exe> "${CMAKE_BINARY_DIR}/build/bin" )
    endif()
endif()

if( BUILD_SHARED )
    add_library( iperf_shared_lib SHARED ${INTERFACE_HEADERS} $<TARGET_OBJECTS:iperf_objects> )
    set_target_properties( iperf_shared_lib
        PROPERTIES
            OUTPUT_NAME iperf
            PUBLIC_HEADER "${INTERFACE_HEADERS}" )
    install( TARGETS iperf_shared_lib
            LIBRARY DESTINATION lib
            PUBLIC_HEADER DESTINATION include )

    add_custom_command( TARGET iperf_shared_lib POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/build/lib"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/build/include"
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:iperf_shared_lib> "${CMAKE_BINARY_DIR}/build/lib"
            COMMAND ${CMAKE_COMMAND} -E copy ${INTERFACE_HEADERS} "${CMAKE_BINARY_DIR}/build/include" )

    if( BUILD_SHARED_EXE )
        add_executable( iperf_shared_exe main.c )
        target_include_directories( iperf_shared_exe
                PRIVATE
                    ${CMAKE_CURRENT_BINARY_DIR} )
        if( APPLE )
            set_target_properties( iperf_shared_lib
                PROPERTIES
                    INSTALL_NAME_DIR "@rpath" )
        endif()

        target_link_libraries( iperf_shared_exe iperf_shared_lib )
        set_target_properties( iperf_shared_exe
            PROPERTIES
                OUTPUT_NAME iperf3-shared )

        install( TARGETS iperf_shared_exe
                RUNTIME DESTINATION bin )

        add_custom_command( TARGET iperf_shared_exe POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/build/bin"
                COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:iperf_shared_exe> "${CMAKE_BINARY_DIR}/build/bin" )
    endif()
endif()

if( APPLE AND BUILD_APPLE_FRAMEWORK )
    set( DUMMY_C_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/dummy.c )
    file( WRITE ${DUMMY_C_SOURCE} "void dummy() {}" )

    add_library( iperf_framework_lib SHARED ${INTERFACE_HEADERS} ${DUMMY_C_SOURCE} $<TARGET_OBJECTS:iperf_objects> )
    target_include_directories( iperf_framework_lib
            PUBLIC
                ${CMAKE_CURRENT_SOURCE_DIR}
            PRIVATE
                ${CMAKE_CURRENT_BINARY_DIR} )
    set_target_properties( iperf_framework_lib
        PROPERTIES
            FRAMEWORK on
            OUTPUT_NAME iperf
            INSTALL_NAME_DIR "@rpath"
            BUILD_WITH_INSTALL_NAME_DIR on
            PUBLIC_HEADER "${INTERFACE_HEADERS}"
            MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_SOURCE_DIR}/cmake/Info.plist.in
            MACOSX_FRAMEWORK_NAME iperf
            MACOSX_FRAMEWORK_IDENTIFIER net.es.iperf
            MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${VERSION}
            MACOSX_FRAMEWORK_BUNDLE_VERSION ${VERSION} )

    install( TARGETS iperf_framework_lib
            LIBRARY DESTINATION lib
            FRAMEWORK DESTINATION Frameworks )

    add_custom_command( TARGET iperf_framework_lib POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/build/lib"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_CURRENT_BINARY_DIR}/iperf.framework"
                    "${CMAKE_BINARY_DIR}/build/lib/iperf.framework" )
endif()
