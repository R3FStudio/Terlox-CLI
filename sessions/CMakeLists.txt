add_library(terlox_sessions STATIC Session.cpp Session.h)
target_include_directories(terlox_sessions PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(terlox_sessions PUBLIC terlox_terminal terlox_platform)
