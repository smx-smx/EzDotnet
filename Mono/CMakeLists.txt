find_package(PkgConfig REQUIRED)
pkg_check_modules(MONO REQUIRED mono-2)

add_library(MonoHost SHARED host.cpp)

set(LIBS ${MONO_LIBRARIES} common)
if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	list(APPEND LIBS stdc++fs)
endif()

target_link_libraries(MonoHost ${LIBS})
target_link_directories(MonoHost PRIVATE ${MONO_LIBRARY_DIRS})
target_include_directories(MonoHost PRIVATE ${MONO_INCLUDE_DIRS})
install(TARGETS MonoHost)
