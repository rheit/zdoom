cmake_minimum_required( VERSION 2.8.7 )

add_pk3(zdoom.pk3 ${CMAKE_CURRENT_SOURCE_DIR}/static)

set( INSTALL_PK3_PATH share/games/doom CACHE STRING "Directory where zdoom.pk3 will be placed during install." )
install ( FILES "${PROJECT_BINARY_DIR}/zdoom.pk3" DESTINATION ${INSTALL_PK3_PATH} )
