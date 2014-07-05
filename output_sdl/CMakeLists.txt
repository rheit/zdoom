cmake_minimum_required( VERSION 2.4 )

make_release_only()

include_directories( ${FMOD_INCLUDE_DIR} ${SDL_INCLUDE_DIR} )

if( ZD_CMAKE_COMPILER_IS_GNUC_COMPATIBLE )
	set( CMAKE_C_FLAGS "-Wall -Wextra -Wno-unused-parameter -fomit-frame-pointer ${CMAKE_C_FLAGS}" )
endif( ZD_CMAKE_COMPILER_IS_GNUC_COMPATIBLE )

add_library( output_sdl output_sdl.c )
target_link_libraries( output_sdl )
