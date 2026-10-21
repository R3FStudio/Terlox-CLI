add_library(terlox_renderer STATIC Frame.h SceneItem.h SceneItem.cpp GlyphCache.h GlyphCache.cpp GraphicsBackend.h GraphicsBackend.cpp)
target_include_directories(terlox_renderer PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(terlox_renderer PUBLIC Qt6::Quick Qt6::Gui)
