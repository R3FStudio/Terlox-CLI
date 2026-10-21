add_library(terlox_window STATIC
  TerminalView.h TerminalView.cpp TerminalPaint.cpp TerminalProgress.cpp TerminalInput.cpp TerminalScroll.cpp Selection.cpp
  Window.h Window.cpp WindowActions.cpp WindowInformation.cpp ProfileDialog.h ProfileDialog.cpp
  WindowFrame.cpp WindowFocus.cpp WindowCommands.cpp WindowAdmin.cpp ApplicationControls.cpp TerminalAbout.cpp AnimatedMenu.h AnimatedMenu.cpp
  ZoomControls.h ZoomControls.cpp FrameBorder.h FrameBorder.cpp)
target_include_directories(terlox_window PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(terlox_window PUBLIC Qt6::Widgets Qt6::QuickWidgets terlox_sessions terlox_settings terlox_renderer terlox_information)
