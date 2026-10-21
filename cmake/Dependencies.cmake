# Vendored libvterm 0.3.3; see dependencies/README.md for provenance.
set(VTERM_DIR "${PROJECT_SOURCE_DIR}/dependencies/libvterm")
add_library(vterm STATIC
  ${VTERM_DIR}/engine/encoding.c ${VTERM_DIR}/engine/keyboard.c
  ${VTERM_DIR}/engine/mouse.c ${VTERM_DIR}/engine/parser.c
  ${VTERM_DIR}/engine/pen.c ${VTERM_DIR}/engine/screen.c
  ${VTERM_DIR}/engine/state.c ${VTERM_DIR}/engine/unicode.c ${VTERM_DIR}/engine/vterm.c)
target_include_directories(vterm PUBLIC ${VTERM_DIR}/include)
set_target_properties(vterm PROPERTIES AUTOMOC OFF)
if(MSVC)
  target_compile_definitions(vterm PRIVATE inline=__inline)
endif()
