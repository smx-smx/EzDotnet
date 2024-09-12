find_package(PkgConfig REQUIRED)
pkg_check_modules(MONO REQUIRED mono-2)

include_directories(${MONO_INCLUDE_DIRS})
link_directories(${MONO_LIBRARY_DIRS})

add_library(MonoHost SHARED host.cpp)

set(LIBS ${MONO_LIBRARIES} common)
if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	list(APPEND LIBS stdc++fs)
endif()

target_link_libraries(MonoHost ${LIBS})
install(TARGETS MonoHost)
