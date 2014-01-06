cmake_minimum_required(VERSION 2.8)
project(simplog)

configure_file (
	"SimpLogConfig.h.in"
	"SimpLogConfig.h"
	@ONLY
)

set( CMAKE_C_COMPILER "clang" )

find_library( BFD_LIBRARY bfd )
find_library( IBERTY_LIBRARY iberty )

find_path(
	IBERTY_HEADER_PATH libiberty.h
	PATHS
		/usr/include/libiberty
		/usr/local/include/libiberty.h
)

include_directories(
	${IBERTY_HEADER_PATH}
	${PROJECT_BINARY_DIR}
)

set( PACKAGE "SimpLog" )
set( PACKAGE_VERSION "0.0.1" )
if( BFD_LIBRARY AND IBERTY_LIBRARY )
	option( BETTER_BACKTRACE "" ON )
else()
	option( BETTER_BACKTRACE "" OFF )
endif()

add_library( backtrace-symbols OBJECT backtrace-symbols.c )

add_library( simplog STATIC simplog.c )

if( BFD_FOUND AND IBERTY_FOUND )
	set( LIBS backtrace-symbols ${BFD_LIBRARY} ${IBERTY_LIBRARY} ${CMAKE_DL_LIBS} )
endif()

target_link_libraries( simplog ${LIBS} )