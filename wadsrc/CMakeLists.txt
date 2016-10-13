cmake_minimum_required( VERSION 2.8.7 )

add_pk3(zdoom.pk3 ${CMAKE_CURRENT_SOURCE_DIR}/static)

set( PK3_INSTALL_PATH share/games/doom CACHE PATH "Directory where zdoom.pk3 will be placed during install." )
install (FILES "${PROJECT_BINARY_DIR}/zdoom.pk3" DESTINATION share/games/doom)
