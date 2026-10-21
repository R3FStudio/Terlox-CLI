# A private, ABI-compatible proxy. Do not depend on Windows Terminal's install.
find_program(TERLOX_MIDL midl REQUIRED)
set(generated "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${generated}")
foreach(interface IConsoleHandoff ITerminalHandoff)
  add_custom_command(OUTPUT "${generated}/${interface}.h" "${generated}/${interface}_i.c" "${generated}/${interface}_p.c"
    COMMAND "${TERLOX_MIDL}" /nologo /env x64 /target NT100 /out "${generated}" /h "${interface}.h" /iid "${interface}_i.c" /proxy "${interface}_p.c" /dlldata "${generated}/${interface}_dlldata.c" "${CMAKE_CURRENT_SOURCE_DIR}/${interface}.idl"
    DEPENDS "${interface}.idl" VERBATIM)
endforeach()
add_library(terlox_handoff_proxy SHARED
  "${generated}/IConsoleHandoff_i.c" "${generated}/IConsoleHandoff_p.c"
  "${generated}/ITerminalHandoff_i.c" "${generated}/ITerminalHandoff_p.c" Proxy.c Proxy.def)
set_target_properties(terlox_handoff_proxy PROPERTIES AUTOMOC OFF OUTPUT_NAME "terlox-handoff-${PROJECT_VERSION}")
target_compile_definitions(terlox_defaults PRIVATE TERLOX_HANDOFF_PROXY_FILE="terlox-handoff-${PROJECT_VERSION}.dll")
target_include_directories(terlox_handoff_proxy PRIVATE "${generated}")
target_compile_definitions(terlox_handoff_proxy PRIVATE REGISTER_PROXY_DLL _WIN32_WINNT=0x0A00)
target_link_libraries(terlox_handoff_proxy PRIVATE rpcrt4 oleaut32 uuid)
add_library(terlox_handoff STATIC ../HandoffServer.cpp ../ConptyApi.cpp "${generated}/ITerminalHandoff_i.c")
target_include_directories(terlox_handoff PUBLIC ${PROJECT_SOURCE_DIR} PRIVATE "${generated}")
target_link_libraries(terlox_handoff PUBLIC Qt6::Core PRIVATE ole32 oleaut32)
add_dependencies(terlox_handoff terlox_handoff_proxy)
