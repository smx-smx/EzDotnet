cmake_minimum_required(VERSION 3.2)

project(EzNetHost)

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE Debug)
endif(CMAKE_BUILD_TYPE STREQUAL "")

include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(common)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	add_subdirectory(CLRHost)
else()
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(MONO mono-2)
	if(MONO_FOUND)
		add_subdirectory(Mono)
	endif()
endif()

add_subdirectory(CoreCLR)
add_subdirectory(samples)