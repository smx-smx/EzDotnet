# Author: Stefano Moioli <smxdev4@gmail.com>

include(FindPackageHandleStandardArgs)

function(_find_cygwin base location)
	cmake_host_system_information(
		RESULT rootdir
		QUERY WINDOWS_REGISTRY ${base}
		VALUE rootdir
	)
	if(NOT rootdir STREQUAL "")
		set(${location} ${rootdir} PARENT_SCOPE)
	else()
		set(${location} FALSE PARENT_SCOPE)
	endif()
endfunction(_find_cygwin)

function(_find_cygwin_main)
	_find_cygwin("HKLM/SOFTWARE/cygwin/setup" _location)
	if(NOT _location)
		_find_cygwin("HKCU/SOFTWARE/cygwin/setup" _location)
	endif()

	if(_location)
		set(Cygwin_ROOT_DIR "${_location}" PARENT_SCOPE)
	endif()
endfunction(_find_cygwin_main)

_find_cygwin_main()
find_package_handle_standard_args(Cygwin
	FOUND_VAR Cygwin_FOUND
	REQUIRED_VARS Cygwin_ROOT_DIR
)
