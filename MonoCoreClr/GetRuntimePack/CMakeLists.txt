find_program(DOTNET_EXE dotnet REQUIRED)

if(MSVC)
	get_filename_component(_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
	find_program(LIB_EXE
		NAMES lib
		HINTS ${_compiler_dir}
		REQUIRED)
	find_program(DUMPBIN_EXE
		NAMES dumpbin
		HINTS ${_compiler_dir}
		REQUIRED)
else()
	find_program(DLLTOOL_EXE dlltool REQUIRED)
endif()

# declare the top-level target
add_custom_target(gen_runtime)

if(CYGWIN)
	find_program(CYGPATH_CYGWIN cygpath REQUIRED)
endif()

function(cygpath_windows path result_var)
	if(CYGWIN)
		execute_process(
			COMMAND ${CYGPATH_CYGWIN} -w "${path}"
			OUTPUT_VARIABLE _out
		)
		string(STRIP "${_out}" _out)
		string(REPLACE "\\" "/" _out "${_out}")
		set(${result_var} "${_out}" PARENT_SCOPE)
	else()
		set(${result_var} "${path}" PARENT_SCOPE)
	endif()
endfunction()

function(dotnet_get_rid OUTPUT_VAR)
    # 1. Detect Operating System
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows"
		OR CMAKE_SYSTEM_NAME STREQUAL "CYGWIN"
		OR CMAKE_SYSTEM_NAME STREQUAL "MSYS"
	)
        set(RID_OS "win")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(RID_OS "osx")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(RID_OS "linux")
        
        # Check for Musl (Alpine Linux)
        # Standard linux-x64 binaries do not work on Alpine; it needs linux-musl-x64
        find_program(LDD_PATH ldd)
        if(LDD_PATH)
            execute_process(COMMAND ${LDD_PATH} --version
                            OUTPUT_VARIABLE LDD_OUTPUT
                            ERROR_VARIABLE LDD_OUTPUT
                            RESULT_VARIABLE LDD_RESULT)
            if(LDD_OUTPUT MATCHES "musl")
                set(RID_OS "linux-musl")
            endif()
        elseif(EXISTS "/etc/alpine-release")
            # Fallback check if ldd isn't in path but distro file exists
            set(RID_OS "linux-musl")
        endif()
    else()
        message(WARNING "Unknown system name: ${CMAKE_SYSTEM_NAME}. Defaulting to 'linux'.")
        set(RID_OS "linux")
    endif()

    # 2. Detect Architecture
    # Normalize CMAKE_SYSTEM_PROCESSOR to Dotnet standards
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" SYSTEM_PROCESSOR_LOWER)

    if(SYSTEM_PROCESSOR_LOWER MATCHES "^(aarch64|arm64)")
        set(RID_ARCH "arm64")
    elseif(SYSTEM_PROCESSOR_LOWER MATCHES "^armv[7-9]")
        set(RID_ARCH "arm") # 32-bit ARM
    else()
        # Fallback using pointer size if processor string is ambiguous
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(RID_ARCH "x64")
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set(RID_ARCH "x86")
        else()
            message(FATAL_ERROR "Could not determine architecture for Dotnet RID")
        endif()
    endif()

    # Special handling for Windows on ARM (Check pointer size to differentiate arm vs arm64 if string is vague)
    if(RID_OS STREQUAL "win" AND RID_ARCH MATCHES "arm")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(RID_ARCH "arm64")
        endif()
    endif()

    # 3. Output the result
    set(${OUTPUT_VAR} "${RID_OS}-${RID_ARCH}" PARENT_SCOPE)
endfunction()

# dotnet expects Win32 paths on Cygwin
cygpath_windows(${LOCAL_INSTALL_PREFIX} DOTNET_LOCAL_INSTALL_PREFIX)
cygpath_windows(${CMAKE_CURRENT_SOURCE_DIR}/GetRuntimePack.csproj DOTNET_CSPROJ_PATH)

dotnet_get_rid(DOTNET_RID)
message(STATUS "Dotnet RID: ${DOTNET_RID}")

## step 1: obtain the runtime pack
set(DOTNET_ARGS
	-c ${CMAKE_BUILD_TYPE}
	-r ${DOTNET_RID}
	/p:BaseIntermediateOutputPath=${DOTNET_LOCAL_INSTALL_PREFIX}/GetRuntimePack/obj/
	/p:OutputPath=${DOTNET_LOCAL_INSTALL_PREFIX}/GetRuntimePack/
)
add_custom_target(GetRuntimePack ALL
	COMMAND ${DOTNET_EXE} publish
		${DOTNET_ARGS} --self-contained
		${DOTNET_CSPROJ_PATH}
)
add_dependencies(gen_runtime GetRuntimePack)

## step 2: read the runtime pack path from the .txt (produced by the build)
## then, copy native libs to a local staging directory

set(csproj_outdir ${LOCAL_INSTALL_PREFIX}/GetRuntimePack)
set(runtime_pack_dir_file ${csproj_outdir}/runtime-pack-dir.txt)

if(MINGW OR CYGWIN)
	set(lib_prefix "")
else()
	set(lib_prefix ${CMAKE_SHARED_LIBRARY_PREFIX})
endif()

add_custom_command(
	OUTPUT ${LOCAL_INSTALL_PREFIX}/native/.touch-copylibs
	COMMAND ${CMAKE_COMMAND} 
		-D CYGWIN=${CYGWIN}
		-D CMAKE_SHARED_LIBRARY_PREFIX=${lib_prefix}
		-D CMAKE_SHARED_LIBRARY_SUFFIX=${CMAKE_SHARED_LIBRARY_SUFFIX}
		-D RUNTIME_PACK_DIR_FILE=${runtime_pack_dir_file}
		-D DESTDIR=${LOCAL_INSTALL_PREFIX}/native
		-P ${CMAKE_CURRENT_SOURCE_DIR}/copy_runtime.cmake
)
add_custom_target(gen_runtime_libs
	DEPENDS ${LOCAL_INSTALL_PREFIX}/native/.touch-copylibs)

install(CODE "
	# dynamic install script to copy coreclr.dll and dependencies next to ezdotnet.exe
    file(GLOB _file_list
        LIST_DIRECTORIES false
        \"${LOCAL_INSTALL_PREFIX}/native/*.dll\"
        \"${LOCAL_INSTALL_PREFIX}/native/*${CMAKE_SHARED_LIBRARY_SUFFIX}\"
    )

    list(REMOVE_DUPLICATES _file_list)

    foreach(f IN LISTS _file_list)
        if(f MATCHES \"\\.dll\\.a$\")
            continue()
        endif()
        file(INSTALL \${f} DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin\")
    endforeach()

	# install MonoCoreClr framework
	file(GLOB _framework_files
		LIST_DIRECTORIES false
		\"${LOCAL_INSTALL_PREFIX}/GetRuntimePack/publish/*\"
	)
	file(INSTALL \${_framework_files} DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin/publish-monocoreclr\")
")

# run after downloading the pack
add_dependencies(gen_runtime_libs GetRuntimePack)
# append to the root target
add_dependencies(gen_runtime gen_runtime_libs)

## step 3: generate coreclr import library
set(coreclr_prefix ${CMAKE_IMPORT_LIBRARY_PREFIX}coreclr)

if(WIN32 OR CYGWIN)
	if(MINGW OR CYGWIN)
		set(coreclr_dll_file ${LOCAL_INSTALL_PREFIX}/native/coreclr${CMAKE_SHARED_LIBRARY_SUFFIX})
	else()
		set(coreclr_dll_file ${LOCAL_INSTALL_PREFIX}/native/${CMAKE_SHARED_LIBRARY_PREFIX}coreclr${CMAKE_SHARED_LIBRARY_SUFFIX})
	endif()
	
	set(coreclr_def_file ${LOCAL_INSTALL_PREFIX}/native/${coreclr_prefix}.def)
	set(coreclr_lib_file ${LOCAL_INSTALL_PREFIX}/native/${coreclr_prefix}${CMAKE_IMPORT_LIBRARY_SUFFIX})

	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(LIB_MACHINE i386)
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(LIB_MACHINE i386:x86-64)
	else()
		message(FATAL_ERROR "Unknown pointer size")
	endif()

	if(MSVC)
		add_custom_command(
			OUTPUT ${coreclr_def_file} ${coreclr_lib_file}
			COMMAND ${CMAKE_COMMAND}
				-D LIB_EXE=${LIB_EXE}
				-D DUMPBIN_EXE=${DUMPBIN_EXE}
				-D IN_DLL=${coreclr_dll_file}
				-D OUT_DEF=${coreclr_def_file}
				-D OUT_LIB=${coreclr_lib_file}
				-D CMAKE_SIZEOF_VOID_P=${CMAKE_SIZEOF_VOID_P}
				-P ${CMAKE_CURRENT_SOURCE_DIR}/gen_lib_msvc.cmake
		)
	else()
		# provided by the following packages:
		## msys2:
		# - mingw-w64-i686-tools
		# - mingw-w64-x86_64-tools
		## cygwin
		# - gendef
		find_program(GENDEF_EXE gendef REQUIRED)
		find_program(DLLTOOL_EXE dlltool REQUIRED)
		add_custom_command(
			OUTPUT ${coreclr_def_file} ${coreclr_lib_file}
			COMMAND ${CMAKE_COMMAND}
				-D GENDEF_EXE=${GENDEF_EXE}
				-D DLLTOOL_EXE=${DLLTOOL_EXE}
				-D IN_DLL=${coreclr_dll_file}
				-D OUT_DEF=${coreclr_def_file}
				-D OUT_LIB=${coreclr_lib_file}
				-D CMAKE_SIZEOF_VOID_P=${CMAKE_SIZEOF_VOID_P}
				-P ${CMAKE_CURRENT_SOURCE_DIR}/gen_lib_mingw.cmake
		)
	endif()
	add_custom_target(gen_coreclr_implib DEPENDS ${coreclr_def_file} ${coreclr_lib_file})

	# run after copying the libs
	add_dependencies(gen_coreclr_implib gen_runtime_libs)
	# append to the root target
	add_dependencies(gen_runtime gen_coreclr_implib)
endif()

set(libmono_include_dirs
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/mono-headers/include/mono-2.0"
)

add_library(coreclr_libmono-2.0_imported SHARED IMPORTED)

set_target_properties(coreclr_libmono-2.0_imported PROPERTIES
        IMPORTED_LOCATION ${coreclr_lib_file}
        IMPORTED_IMPLIB ${coreclr_lib_file}
        INTERFACE_INCLUDE_DIRECTORIES "${libmono_include_dirs}"
)

add_library(coreclr_libmono-2.0 INTERFACE)

target_link_libraries(coreclr_libmono-2.0 INTERFACE coreclr_libmono-2.0_imported)
if(WIN32 OR CYGWIN)
	add_dependencies(coreclr_libmono-2.0 gen_coreclr_implib)
endif()

## Targets fixup
if(MSVC)
	# workaround for https://github.com/dotnet/NuGet.BuildTasks/issues/154
	# technically we need to set
	# - VS_GLOBAL__NuGetTargetFallbackMoniker
	# - VS_GLOBAL_RuntimeIdentifiers
	# but even then, we will still get an error during nuget restore - as if <RuntimeIdentifiers> was still not considered
	# for some reason
	#
	# however, since these .vcxproj files are just CMake generated wrappers, we can just disable Nuget package resolution
	# entirely to bypass the problematic check
    set_target_properties(GetRuntimePack PROPERTIES
		VS_GLOBAL_ResolveNuGetPackages "false"
    )
	set_target_properties(gen_runtime_libs PROPERTIES
		VS_GLOBAL_ResolveNuGetPackages "false"
    )
	set_target_properties(gen_coreclr_implib PROPERTIES
		VS_GLOBAL_ResolveNuGetPackages "false"
    )
endif()
