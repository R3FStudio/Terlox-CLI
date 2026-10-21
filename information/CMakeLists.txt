configure_file(Version.h.in ${CMAKE_CURRENT_BINARY_DIR}/Version.h @ONLY)
add_library(terlox_information STATIC Report.cpp Shortcuts.cpp Releases.cpp About.cpp)
target_include_directories(terlox_information PUBLIC ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/..)
target_link_libraries(terlox_information PUBLIC Qt6::Core)
