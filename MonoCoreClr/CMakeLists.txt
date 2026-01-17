set(LOCAL_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(GetRuntimePack)

include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz DOWNLOAD_EXTRACT_TIMESTAMP NEW)
FetchContent_MakeAvailable(json)

add_library(MonoHost_CoreClr SHARED host.cpp)
set_target_properties(MonoHost_CoreClr PROPERTIES CXX_STANDARD 20)

target_link_libraries(MonoHost_CoreClr
        coreclr_libmono-2.0
        nlohmann_json::nlohmann_json)

if(MINGW)
    target_link_options(MonoHost_CoreClr PRIVATE "LINKER:--no-undefined")
endif()

set(LIBS common)
if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    list(APPEND LIBS stdc++fs)
endif()
target_link_libraries(MonoHost_CoreClr ${LIBS})

install(TARGETS MonoHost_CoreClr)
