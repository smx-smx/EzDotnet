add_library(coreclrhost SHARED dotnetcore.cpp)
target_link_libraries(coreclrhost common)

set(NETHOST_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}nethost${CMAKE_STATIC_LIBRARY_SUFFIX})

find_library(
	NETHOST_LIBRARY
	NAMES ${NETHOST_NAME}
)

if(NETHOST_LIBRARY)
	target_link_libraries(coreclrhost ${NETHOST_LIBRARY})
	target_compile_options(coreclrhost PRIVATE -DNETHOST_STATIC)
endif()

install(TARGETS coreclrhost)