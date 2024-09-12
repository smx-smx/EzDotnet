if(CMAKE_GENERATOR_PLATFORM STREQUAL "x64")
	set(platform "x64")
elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
	set(platform "x86")
else()
	set(platform "AnyCPU")
endif()

add_dotnet(Sample.csproj PLATFORM ${platform})
