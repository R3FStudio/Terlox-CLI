add_library(terlox_shell STATIC
  parsing/Parser.cpp parsing/Expansion.cpp
  runtime/Builtins.cpp runtime/Help.h runtime/Help.cpp runtime/Executor.cpp runtime/NativeShell.cpp
  editing/History.cpp editing/Completion.cpp editing/LineEditor.cpp editing/Layout.cpp editing/InputState.h editing/InputState.cpp)
target_include_directories(terlox_shell PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(terlox_shell PUBLIC Qt6::Core terlox_information terlox_elevation terlox_defaults terlox_updates)
add_executable(terloxsh main.cpp)
target_link_libraries(terloxsh PRIVATE terlox_shell)
if(WIN32)
  target_include_directories(terloxsh PRIVATE ${PROJECT_SOURCE_DIR}/branding)
  target_sources(terloxsh PRIVATE ../branding/Terlox.rc ${CMAKE_BINARY_DIR}/branding/Version.rc)
  set_source_files_properties(../branding/Terlox.rc PROPERTIES OBJECT_DEPENDS "${PROJECT_SOURCE_DIR}/branding/icons/terlox.ico")
endif()
