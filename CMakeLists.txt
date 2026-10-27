cmake_minimum_required( VERSION 3.8 )

project( iperf LANGUAGES C )

include( cmake/CMakeDebugUtils.cmake )
include( config.cmake )

set( BUILD_STATIC on CACHE STRING "" )
set( BUILD_STATIC_EXE on CACHE STRING "" )
set( BUILD_SHARED on CACHE STRING "" )
set( BUILD_SHARED_EXE on CACHE STRING "" )
set( BUILD_APPLE_FRAMEWORK off CACHE STRING "" )
set( BITCODE_ENABLED off CACHE STRING "" )

add_subdirectory( src )

configure_file( iperf3.spec.in iperf3.spec )
