cmake_minimum_required(VERSION 3.24)
project(ezdotnet_test NONE)
enable_testing()

if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(MONO mono-2)
endif()


set(default_build_type Debug)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
	set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
	# Set the possible values of build type for cmake-gui
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(HERE ${CMAKE_CURRENT_LIST_DIR})
get_filename_component(PARENT ${HERE}/.. ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH "${PARENT}/cmake")
find_package(Dotnet REQUIRED)
include(ExternalProject)

set(SAMPLE_DOTNET_FRAMEWORKS
	net6.0
	net472
)

function(target_get_cli target out)
	ExternalProject_Get_property(${target} INSTALL_DIR)
	set(${out} "${INSTALL_DIR}/bin/ezdotnet${CMAKE_EXECUTABLE_SUFFIX}" PARENT_SCOPE)
endfunction()

function(target_get_mansample target fwk out)
	ExternalProject_Get_property(${target} BINARY_DIR)
	set(dotnet_outdir ${BINARY_DIR}/Sample)
	if(fwk MATCHES "^net4")
		set(_suffix ".exe")
	else()
		set(_suffix ".dll")
	endif()
	set(${out} "${dotnet_outdir}/${fwk}/Sample${_suffix}" PARENT_SCOPE)
endfunction()

function(target_get_host target host out)
	ExternalProject_Get_property(${target} INSTALL_DIR)
	get_target_property(is_mingw32 ${target} TGT_MINGW32)
	get_target_property(is_mingw64 ${target} TGT_MINGW64)

	get_target_property(is_windows ${target} TGT_WIN32)
	get_target_property(is_cygwin ${target} TGT_CYGWIN)

	if(is_cygwin)
		set(host_filename "${INSTALL_DIR}/bin/cyg${host}.dll")
	elseif(is_mingw32 OR is_mingw64)
		set(host_filename "${INSTALL_DIR}/bin/lib${host}.dll")
	elseif(is_windows)
		set(host_filename "${INSTALL_DIR}/bin/${host}.dll")
	else()
		set(host_filename "${INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}${host}${CMAKE_SHARED_LIBRARY_SUFFIX}")
	endif()
	set(${out} "${host_filename}" PARENT_SCOPE)
endfunction()

function(test_target target_cli target_host target_mansample host)
	target_get_cli(${target_cli} cli_path)
	target_get_host(${target_host} ${host} host_path)
	
	get_target_property(is_mingw32 ${target_cli} TGT_MINGW32)
	get_target_property(is_mingw64 ${target_cli} TGT_MINGW64)
	get_target_property(is_cygwin ${target_cli} TGT_CYGWIN)

	foreach(fwk IN LISTS SAMPLE_DOTNET_FRAMEWORKS)
		if(fwk MATCHES "^net4")
			if(host STREQUAL coreclrhost)
				# net4 sample can only be loaded by CLRHost or Mono
				continue()
			endif()
		else() # netcore
			if(host STREQUAL MonoHost)
				# FIXME: mono needs an ad-hoc runtime to load NetCore apps
				continue()
			endif()
		endif()

		target_get_mansample(${target_mansample} ${fwk} asm_path)
		set(test_name ${target_cli}-${target_host}-${host}-${fwk})
		add_test(
			NAME ${test_name}
			COMMAND
				${cli_path} ${host_path} ${asm_path}
				ManagedSample.EntryPoint Entry
				arg1 arg2 arg3 arg4 arg5
		)
		if(is_cygwin)
			set_property(TEST ${test_name} PROPERTY ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${Cygwin_ROOT_DIR}/bin")
		elseif(is_mingw32)
			set(_env_ops 
				"PATH=path_list_prepend:${MINGW32_ROOT}/bin"
				"MSYSTEM=set:MINGW32"
				"MINGW_PREFIX=set:/mingw32"
			)
			set_property(TEST ${test_name} PROPERTY ENVIRONMENT_MODIFICATION "${_env_ops}")
		elseif(is_mingw64)
			set(_env_ops 
				"PATH=path_list_prepend:${MINGW64_ROOT}/bin"
				"MSYSTEM=set:MINGW64"
				"MINGW_PREFIX=set:/mingw64"
			)
			set_property(TEST ${test_name} PROPERTY ENVIRONMENT_MODIFICATION "${_env_ops}")
		endif()
	endforeach()
endfunction(test_target)

define_property(TARGET PROPERTY TGT_CYGWIN BRIEF_DOCS "This project targets Cygwin")
define_property(TARGET PROPERTY TGT_WIN32 BRIEF_DOCS "This project targets Win32")
define_property(TARGET PROPERTY TGT_MINGW32 BRIEF_DOCS "This project targets Mingw32")
define_property(TARGET PROPERTY TGT_MINGW64 BRIEF_DOCS "This project targets Mingw64")

if(WIN32 OR CYGWIN)
	include(windows.cmake)
else()
	include(unix.cmake)
endif()
