find_package(Cygwin REQUIRED)
find_package(MSYS2 REQUIRED COMPONENTS mingw32 mingw64)

set(MINGW32_ROOT ${MSYS2_ROOT_DIR}/mingw32)
set(MINGW64_ROOT ${MSYS2_ROOT_DIR}/mingw64)

find_program(CMAKE_MINGW32 cmake PATHS ${MINGW32_ROOT}/bin NO_DEFAULT_PATH REQUIRED)
find_program(CMAKE_MINGW64 cmake PATHS ${MINGW32_ROOT}/bin NO_DEFAULT_PATH REQUIRED)
find_program(BASH_CYGWIN bash PATHS ${Cygwin_ROOT_DIR}/bin NO_DEFAULT_PATH REQUIRED)
find_program(CYGPATH_CYGWIN cygpath PATHS ${Cygwin_ROOT_DIR}/bin NO_DEFAULT_PATH REQUIRED)
find_program(CYGPATH_MSYS cygpath PATHS ${MSYS2_ROOT_DIR}/usr/bin NO_DEFAULT_PATH REQUIRED)
find_program(BASH_MSYS2 bash PATHS ${MSYS2_ROOT_DIR}/usr/bin NO_DEFAULT_PATH REQUIRED)

get_filename_component(DOTNET_EXE_DIR "${DOTNET_EXE}" DIRECTORY)

execute_process(
	COMMAND ${CYGPATH_MSYS} -u "${DOTNET_EXE_DIR}"
	OUTPUT_VARIABLE DOTNET_EXE_DIR_MSYS
)
string(STRIP "${DOTNET_EXE_DIR_MSYS}" DOTNET_EXE_DIR_MSYS)

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

set(_msys_prologue
	"pushd .$<SEMICOLON>"
	". /etc/profile$<SEMICOLON>"
	"popd$<SEMICOLON>"
	"export PATH=\"${DOTNET_EXE_DIR_MSYS}\":\$PATH$<SEMICOLON>"
)
set(_msys_prologue_mingw32
	"export MSYSTEM=MINGW32$<SEMICOLON>"
	${_msys_prologue}
)
set(_msys_prologue_mingw64
	"export MSYSTEM=MINGW64$<SEMICOLON>"
	${_msys_prologue}
)

ExternalProject_Add(ezdotnet_mingw32
	PREFIX ${BDIR_MINGW_X86}
	SOURCE_DIR ${PARENT}
	CONFIGURE_COMMAND
		${CMAKE_COMMAND} -E echo
		${_msys_prologue_mingw32} cmake
		-S <SOURCE_DIR> -B <BINARY_DIR>
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
		-DBUILD_MANAGED_SAMPLE=OFF | cmd /C ${BASH_MSYS2}
	BUILD_COMMAND
		${CMAKE_COMMAND} -E echo
		${_msys_prologue_mingw32} cmake --build <BINARY_DIR> --config ${CMAKE_BUILD_TYPE} | cmd /C ${BASH_MSYS2}
	INSTALL_COMMAND
		${CMAKE_COMMAND} -E echo
		${_msys_prologue_mingw32} cmake --install <BINARY_DIR> --config ${CMAKE_BUILD_TYPE} | cmd /C ${BASH_MSYS2}
)
ExternalProject_Add(ezdotnet_mingw64
	PREFIX ${BDIR_MINGW_X64}
	SOURCE_DIR ${PARENT}
	CONFIGURE_COMMAND
		${CMAKE_COMMAND} -E echo
		${_msys_prologue_mingw64} cmake
		-S <SOURCE_DIR> -B <BINARY_DIR>
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
		-DBUILD_MANAGED_SAMPLE=OFF | cmd /C ${BASH_MSYS2}
	BUILD_COMMAND
		${CMAKE_COMMAND} -E echo
		${_msys_prologue_mingw64} cmake --build <BINARY_DIR> --config ${CMAKE_BUILD_TYPE} | cmd /C ${BASH_MSYS2}
	INSTALL_COMMAND
		${CMAKE_COMMAND} -E echo
		${_msys_prologue_mingw64} cmake --install <BINARY_DIR> --config ${CMAKE_BUILD_TYPE} | cmd /C ${BASH_MSYS2}

)

set(_cygwin_prologue
	"pushd .$<SEMICOLON>"
	". /etc/profile$<SEMICOLON>"
	"popd$<SEMICOLON>"
)
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
	BUILD_COMMAND ${CMAKE_COMMAND} -E echo
		${_cygwin_prologue} cmake
		--build \"$\(cygpath -u \"<BINARY_DIR>\"\)\"
		--config ${CMAKE_BUILD_TYPE}
		-- -j${NCPU} | cmd /C ${BASH_CYGWIN}
	INSTALL_COMMAND ${CMAKE_COMMAND} -E echo
		${_cygwin_prologue} cmake
		--install \"$\(cygpath -u \"<BINARY_DIR>\"\)\"
		--config ${CMAKE_BUILD_TYPE} | cmd /C ${BASH_CYGWIN}
)

set_target_properties(ezdotnet_msvc_x86 PROPERTIES TGT_WIN32 TRUE)
set_target_properties(ezdotnet_msvc_x64 PROPERTIES TGT_WIN32 TRUE)
set_target_properties(ezdotnet_mingw32 PROPERTIES TGT_MINGW32 TRUE)
set_target_properties(ezdotnet_mingw64 PROPERTIES TGT_MINGW64 TRUE)
set_target_properties(ezdotnet_cygwin64 PROPERTIES TGT_CYGWIN TRUE)


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
	ezdotnet_mingw64
	ezdotnet_mingw64
	ezdotnet_msvc_x64
	coreclrhost
)
if(MONO_FOUND)
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
		MonoHost
	)
endif()
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
