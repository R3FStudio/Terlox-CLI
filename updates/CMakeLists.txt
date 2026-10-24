add_library(terlox_updates STATIC Release.h Release.cpp Client.h Client.cpp Command.h Command.cpp
  Download.h Download.cpp InstallPlan.h InstallPlan.cpp Service.h Service.cpp)
target_include_directories(terlox_updates PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(terlox_updates PUBLIC Qt6::Network terlox_information)
