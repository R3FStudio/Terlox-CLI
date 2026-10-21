add_library(terlox_terminal STATIC Screen.cpp Scrollback.cpp Keyboard.cpp Extensions.cpp Sequences.cpp Graphics.cpp Screen.h Progress.h Progress.cpp EditableInput.h EditableInput.cpp)
target_include_directories(terlox_terminal PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(terlox_terminal PUBLIC Qt6::Gui vterm)
