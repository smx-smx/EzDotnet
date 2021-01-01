add_library(CLRHost SHARED CLRHost.cpp)
target_compile_options(CLRHost PRIVATE /clr /fp:precise)
string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")