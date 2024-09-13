add_library(CLRHost SHARED AssemblyInfo.cpp CLRHost.cpp)
target_compile_options(CLRHost PRIVATE /fp:precise)
target_link_options(CLRHost PRIVATE /LTCG:OFF)
set_target_properties(CLRHost PROPERTIES
	COMMON_LANGUAGE_RUNTIME "")
string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

target_link_libraries(CLRHost common)
install(TARGETS CLRHost)
