cmake_minimum_required(VERSION 2.8)
project(simplog)

set( PACKAGE "SimpLog" )
set( PACKAGE_VERSION "0.0.1" )

set( CMAKE_C_COMPILER "clang" )

include_directories(
	\"${IBERTY_HEADER_PATH}\"
	\"${PROJECT_BINARY_DIR}\"
)

add_library( simplog STATIC simplog.c )
