cmake_minimum_required(VERSION 3.24)
project(ezdotnet_test NONE)
enable_testing()


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
find_package(Cygwin REQUIRED)
find_package(MSYS2 REQUIRED COMPONENTS mingw32 mingw64)
find_package(DOTNET REQUIRED)
include(ExternalProject)

set(MINGW32_ROOT ${MSYS2_ROOT_DIR}/mingw32)
set(MINGW64_ROOT ${MSYS2_ROOT_DIR}/mingw64)

define_property(TARGET PROPERTY TGT_CYGWIN BRIEF_DOCS "This project targets Cygwin")
define_property(TARGET PROPERTY TGT_WIN32 BRIEF_DOCS "This project targets Win32")
define_property(TARGET PROPERTY TGT_MINGW BRIEF_DOCS "This project targets Mingw")

find_program(CMAKE_MINGW32 cmake PATHS ${MINGW32_ROOT}/bin NO_DEFAULT_PATH REQUIRED)
find_program(CMAKE_MINGW64 cmake PATHS ${MINGW32_ROOT}/bin NO_DEFAULT_PATH REQUIRED)
find_program(BASH_CYGWIN bash PATHS ${Cygwin_ROOT_DIR}/bin NO_DEFAULT_PATH REQUIRED)
find_program(CYGPATH_CYGWIN cygpath PATHS ${Cygwin_ROOT_DIR}/bin NO_DEFAULT_PATH REQUIRED)

function(cygpath_unix path result_var)
    if(CYGWIN)
        execute_process(
            COMMAND ${CYGPATH_CYGWIN} -u "${path}"
            OUTPUT_VARIABLE _out
        )
        string(STRIP "${_out}" _out)
        set(${result_var} "${_out}" PARENT_SCOPE)
    else()
        set(${result_var} "${path}" PARENT_SCOPE)
    endif()
endfunction()

set(BDIR_MSVC_X86 ${CMAKE_BINARY_DIR}/build_msvc32)
set(BDIR_MSVC_X64 ${CMAKE_BINARY_DIR}/build_msvc64)
set(BDIR_MINGW_X86 ${CMAKE_BINARY_DIR}/build_mingw32)
set(BDIR_MINGW_X64 ${CMAKE_BINARY_DIR}/build_mingw64)
set(BDIR_CYGWIN_X64 ${CMAKE_BINARY_DIR}/build_cygwin64)

ExternalProject_Add(ezdotnet_msvc_x86
	PREFIX ${BDIR_MSVC_X86}
	SOURCE_DIR ${PARENT}
	CONFIGURE_COMMAND ${CMAKE_COMMAND}
		-S <SOURCE_DIR> -B <BINARY_DIR>
		-G "Visual Studio 17 2022" -A "Win32"
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
		-DBUILD_MANAGED_SAMPLE=ON
	BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config ${CMAKE_BUILD_TYPE}
	INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR> --config ${CMAKE_BUILD_TYPE}
)
ExternalProject_Add(ezdotnet_msvc_x64
	PREFIX ${BDIR_MSVC_X64}
	SOURCE_DIR ${PARENT}
	CONFIGURE_COMMAND ${CMAKE_COMMAND}
		-S <SOURCE_DIR> -B <BINARY_DIR>
		-G "Visual Studio 17 2022" -A "x64"
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
		-DBUILD_MANAGED_SAMPLE=ON
	BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config ${CMAKE_BUILD_TYPE}
	INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR> --config ${CMAKE_BUILD_TYPE}
)
# prevent concurrent .NET build
add_dependencies(ezdotnet_msvc_x64 ezdotnet_msvc_x86)

set(CURRENT_PATH $ENV{PATH})
# 4 escape layers!
string(REPLACE ";" "\\\\\\\;" CURRENT_PATH "${CURRENT_PATH}")

ExternalProject_Add(ezdotnet_mingw32
	PREFIX ${BDIR_MINGW_X86}
	SOURCE_DIR ${PARENT}
	CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env "PATH=${MINGW32_ROOT}/bin$<SEMICOLON>${CURRENT_PATH}" ${CMAKE_MINGW32}
		-S <SOURCE_DIR> -B <BINARY_DIR>
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
		-DBUILD_MANAGED_SAMPLE=OFF
	BUILD_COMMAND
		${CMAKE_COMMAND} -E env "PATH=${MINGW32_ROOT}/bin$<SEMICOLON>${CURRENT_PATH}" ${CMAKE_MINGW32}
		--build <BINARY_DIR> --config ${CMAKE_BUILD_TYPE}
	INSTALL_COMMAND
		${CMAKE_COMMAND} -E env "PATH=${MINGW32_ROOT}/bin$<SEMICOLON>${CURRENT_PATH}" ${CMAKE_MINGW32}
		--install <BINARY_DIR> --config ${CMAKE_BUILD_TYPE}
)
ExternalProject_Add(ezdotnet_mingw64
	PREFIX ${BDIR_MINGW_X64}
	SOURCE_DIR ${PARENT}
	CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env "PATH=${MINGW64_ROOT}/bin$<SEMICOLON>${CURRENT_PATH}" ${CMAKE_MINGW64}
		-S <SOURCE_DIR> -B <BINARY_DIR>
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
		-DBUILD_MANAGED_SAMPLE=OFF
	BUILD_COMMAND
		${CMAKE_COMMAND} -E env "PATH=${MINGW64_ROOT}/bin$<SEMICOLON>${CURRENT_PATH}" ${CMAKE_MINGW64}
		--build <BINARY_DIR> --config ${CMAKE_BUILD_TYPE}
	INSTALL_COMMAND
		${CMAKE_COMMAND} -E env "PATH=${MINGW64_ROOT}/bin$<SEMICOLON>${CURRENT_PATH}" ${CMAKE_MINGW64}
		--install <BINARY_DIR> --config ${CMAKE_BUILD_TYPE}

)

set(_cygwin_prologue "pushd .$<SEMICOLON>. /etc/profile$<SEMICOLON>popd$<SEMICOLON>")

cmake_host_system_information(RESULT NCPU
                              QUERY NUMBER_OF_PHYSICAL_CORES)

ExternalProject_Add(ezdotnet_cygwin64
	PREFIX ${BDIR_CYGWIN_X64}
	SOURCE_DIR ${PARENT}
	CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo
		${_cygwin_prologue} cmake
		-S \"$\(cygpath -u \"<SOURCE_DIR>\"\)\"
		-B \"$\(cygpath -u \"<BINARY_DIR>\"\)\"
		-G 'Unix Makefiles'
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_INSTALL_PREFIX=\"$\(cygpath -u \"<INSTALL_DIR>\"\)\"
		-DBUILD_MANAGED_SAMPLE=OFF | cmd /C ${BASH_CYGWIN}
	BUILD_COMMAND ${BASH_CYGWIN} -c "${_cygwin_prologue} cmake --build \"$(cygpath -u \"<BINARY_DIR>\")\" --config ${CMAKE_BUILD_TYPE} -- -j${NCPU}"
	INSTALL_COMMAND ${BASH_CYGWIN} -c "${_cygwin_prologue} cmake --install \"$(cygpath -u \"<BINARY_DIR>\")\" --config ${CMAKE_BUILD_TYPE}"
)

set_target_properties(ezdotnet_msvc_x86 PROPERTIES TGT_WIN32 TRUE)
set_target_properties(ezdotnet_msvc_x64 PROPERTIES TGT_WIN32 TRUE)
set_target_properties(ezdotnet_mingw32 PROPERTIES TGT_MINGW TRUE)
set_target_properties(ezdotnet_mingw64 PROPERTIES TGT_MINGW TRUE)
set_target_properties(ezdotnet_cygwin64 PROPERTIES TGT_CYGWIN TRUE)

set(all_modes win32 win64 mingw32 mingw64 cygwin)
set(all_loaders clr coreclr mono)

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
	get_target_property(is_mingw ${target} TGT_MINGW)
	get_target_property(is_windows ${target} TGT_WIN32)
	get_target_property(is_cygwin ${target} TGT_CYGWIN)

	if(is_cygwin)
		set(host_filename "${INSTALL_DIR}/bin/cyg${host}.dll")
	elseif(is_mingw)
		set(host_filename "${INSTALL_DIR}/bin/lib${host}.dll")
	elseif(is_windows)
		set(host_filename "${INSTALL_DIR}/bin/${host}.dll")
	else()
		set(host_filename "${INSTALL_DIR}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}${host}${CMAKE_SHARED_LIBRARY_SUFFIX}")
	endif()
	set(${out} "${host_filename}" PARENT_SCOPE)
endfunction()

function(test_target target_cli target_host target_mansample host)
	target_get_cli(${target_cli} cli_path)
	target_get_host(${target_host} ${host} host_path)
	
	get_target_property(is_cygwin ${target_cli} TGT_CYGWIN)

	foreach(fwk IN LISTS SAMPLE_DOTNET_FRAMEWORKS)
		# net4 sample can only be loaded by CLRHost or Mono
		if(fwk MATCHES "^net4" AND host STREQUAL coreclrhost)
			continue()
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
		endif()
	endforeach()
endfunction(test_target)

test_target(
	ezdotnet_msvc_x86
	ezdotnet_msvc_x86
	ezdotnet_msvc_x86
	CLRHost
)
test_target(
	ezdotnet_msvc_x86
	ezdotnet_msvc_x86
	ezdotnet_msvc_x86
	coreclrhost
)
test_target(
	ezdotnet_msvc_x64
	ezdotnet_msvc_x64
	ezdotnet_msvc_x64
	CLRHost
)
test_target(
	ezdotnet_msvc_x64
	ezdotnet_msvc_x64
	ezdotnet_msvc_x64
	coreclrhost
)
test_target(
	ezdotnet_mingw32
	ezdotnet_mingw32
	ezdotnet_msvc_x86
	coreclrhost
)
test_target(
	ezdotnet_mingw32
	ezdotnet_mingw32
	ezdotnet_msvc_x86
	MonoHost
)

test_target(
	ezdotnet_mingw64
	ezdotnet_mingw64
	ezdotnet_msvc_x64
	coreclrhost
)
test_target(
	ezdotnet_mingw64
	ezdotnet_mingw64
	ezdotnet_msvc_x64
	MonoHost
)
test_target(
	ezdotnet_cygwin64
	ezdotnet_cygwin64
	ezdotnet_msvc_x64
	coreclrhost
)
test_target(
	ezdotnet_cygwin64
	ezdotnet_msvc_x64
	ezdotnet_msvc_x64
	CLRHost
)
