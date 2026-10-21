set(CMAKE_AUTORCC ON)
add_library(terlox_settings STATIC Appearance.cpp Profiles.cpp Store.cpp Commands.cpp AdminProfile.h AdminProfile.cpp
  fonts/TerminalFont.h fonts/TerminalFont.cpp fonts/TerminalFonts.qrc)
target_include_directories(terlox_settings PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(terlox_settings PUBLIC Qt6::Gui terlox_elevation)
