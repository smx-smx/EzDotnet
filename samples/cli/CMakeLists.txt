add_executable(ezdotnet ezdotnet.c)
add_library(ezdotnet_shared SHARED ezdotnet.c)
add_library(ezdotnet_static STATIC ezdotnet.c)

if(UNIX)
	target_link_libraries(ezdotnet dl)
	target_link_libraries(ezdotnet_shared dl)
	target_link_libraries(ezdotnet_static dl)
endif()

target_compile_definitions(ezdotnet_static PRIVATE EZDOTNET_STATIC)

target_link_libraries(ezdotnet common)
target_link_libraries(ezdotnet_static common)
target_link_libraries(ezdotnet_shared common)

install(TARGETS
	ezdotnet
	ezdotnet_shared
	ezdotnet_static
)
