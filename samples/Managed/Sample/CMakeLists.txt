if((WIN32 AND CMAKE_GENERATOR_PLATFORM STREQUAL "x64") OR (CMAKE_SIZEOF_VOID_P EQUAL 8))
	set(platform "x64")
elseif((WIN32 AND CMAKE_GENERATOR_PLATFORM STREQUAL "Win32") OR (CMAKE_SIZEOF_VOID_P EQUAL 4))
	set(platform "x86")
else()
	set(platform "AnyCPU")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(dotnet_config "Debug")
else()
	set(dotnet_config "Release")
endif()

add_dotnet(Sample.csproj
	PLATFORM ${platform}
	CONFIG ${dotnet_config}
)
