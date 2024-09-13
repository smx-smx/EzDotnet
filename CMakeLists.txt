cmake_minimum_required(VERSION 3.10)
project(EzNetHost LANGUAGES C CXX)

enable_testing()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(Dotnet REQUIRED)
set(CMAKE_CXX_STANDARD 17)

option(BUILD_MANAGED_SAMPLE "Build managed sample" ON)

set(default_build_type Debug)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
	set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
	# Set the possible values of build type for cmake-gui
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

configure_file(config.h.in config.h)

include_directories(
	${CMAKE_BINARY_DIR}
	${CMAKE_SOURCE_DIR}
)

add_subdirectory(common)

set(IS_MSVC FALSE)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(IS_MSVC TRUE)
endif()

if(IS_MSVC)
	add_subdirectory(CLRHost)
endif()

find_package(PkgConfig)
if(PkgConfig_FOUND)
	pkg_check_modules(MONO mono-2)
endif()

if(MONO_FOUND AND NOT IS_MSVC)
	add_subdirectory(Mono)
endif()

add_subdirectory(CoreCLR)
add_subdirectory(samples)
