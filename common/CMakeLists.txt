set(SOURCES common.cpp)
if(WIN32 OR CYGWIN)
	list(APPEND SOURCES common_win32.cpp)
elseif(UNIX)
	list(APPEND SOURCES common_unix.cpp)
endif()

add_library(common ${SOURCES})