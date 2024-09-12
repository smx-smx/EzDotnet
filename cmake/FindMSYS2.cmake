# Author: Stefano Moioli <smxdev4@gmail.com>

include(FindPackageHandleStandardArgs)

function(_find_msys2 base location)
	cmake_host_system_information(
		RESULT InstalledPrograms
		QUERY WINDOWS_REGISTRY ${base} SUBKEYS
	)
	foreach(itm IN LISTS InstalledPrograms)
		cmake_host_system_information(
			RESULT DisplayName
			QUERY WINDOWS_REGISTRY "${base}/${itm}"
			VALUE DisplayName
		)
		cmake_host_system_information(
			RESULT InstallLocation 
			QUERY WINDOWS_REGISTRY "${base}/${itm}"
			VALUE InstallLocation
		)
		if(DisplayName MATCHES "^MSYS2")
			set(${location} "${InstallLocation}" PARENT_SCOPE)
			return()
		endif()
	endforeach()
	set(${location} FALSE PARENT_SCOPE)
endfunction(_find_msys2)

function(_find_msys2_component base name result)
	set(_items
		${name}.ini
		${name}.exe
		${name}
	)
	foreach(itm IN LISTS _items)
		if(NOT EXISTS "${base}/${itm}")
			set(${result} FALSE PARENT_SCOPE)
			return()
		endif()
	endforeach()

	set(${result} TRUE PARENT_SCOPE)
endfunction(_find_msys2_component)


function(_find_msys2_main)
	_find_msys2("HKLM/SOFTWARE/Microsoft/Windows/CurrentVersion/Uninstall" _location)
	if(NOT _location)
		_find_msys2("HKCU/SOFTWARE/Microsoft/Windows/CurrentVersion/Uninstall" _location)
	endif()

	if(_location)
		set(MSYS2_ROOT_DIR "${_location}" PARENT_SCOPE)
	endif()

	_find_msys2_component(${_location} mingw32 _mingw32_found)
	_find_msys2_component(${_location} mingw64 _mingw64_found)
	_find_msys2_component(${_location} clang32 _clang32_found)
	_find_msys2_component(${_location} clang64 _clang64_found)
	_find_msys2_component(${_location} clangarm64 _clangarm64_found)
	_find_msys2_component(${_location} ucrt64 _ucrt64_found)
	
	set(MSYS2_mingw32_FOUND ${_mingw32_found} PARENT_SCOPE)
	set(MSYS2_mingw64_FOUND ${_mingw64_found} PARENT_SCOPE)
	set(MSYS2_clang32_FOUND ${_clang32_found} PARENT_SCOPE)
	set(MSYS2_clang64_FOUND ${_clang64_found} PARENT_SCOPE)
	set(MSYS2_clangarm64_FOUND ${_clangarm64_found} PARENT_SCOPE)
	set(MSYS2_ucrt64_FOUND ${_ucrt64_found} PARENT_SCOPE)
endfunction(_find_msys2_main)

if(NOT MSYS2_FOUND)
	_find_msys2_main()
endif()
find_package_handle_standard_args(MSYS2
	FOUND_VAR MSYS2_FOUND
	REQUIRED_VARS MSYS2_ROOT_DIR
	HANDLE_COMPONENTS
)
