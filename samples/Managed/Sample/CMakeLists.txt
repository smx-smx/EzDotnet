if(CMAKE_GENERATOR_PLATFORM STREQUAL "x64")
	set(platform "x64")
elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
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
