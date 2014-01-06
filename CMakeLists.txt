cmake_minimum_required(VERSION 2.8)
project(simplog)

configure_file (
	"SimpLogConfig.h.in"
	"SimpLogConfig.h"
)

set( CMAKE_C_COMPILER "clang" )

find_package( BFD )
find_package( Iberty )
find_package( DL )

if( BFD_FOUND AND IBERTY_FOUND AND DL_FOUND )
	option( BETTER_BACKTRACE "" ON )
else( BFD_FOUND AND IBERTY_FOUND AND DL_FOUND )
	options( BETTER_BACKTRACE "" OFF )
endif( BFD_FOUND AND IBERTY_FOUND AND DL_FOUND )

add_library( backtrace-symbols OBJECT backtrace-symbols.c )

add_library( simplog STATIC simplog.c )

if( BFD_FOUND AND IBERTY_FOUND AND DL_FOUND )
	target_link_libraries( simplog
			backtrace-symbols
			${BFD_LIBRARY}
			${IBERTY_LIBRARY}
			${DL_LIBRARY}
	)
endif( BFD_FOUND AND IBERTY_FOUND AND DL_FOUND )