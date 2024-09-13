add_executable(ezdotnet ezdotnet.c)
add_library(ezdotnet_shared SHARED ezdotnet.c)

if(UNIX)
	target_link_libraries(ezdotnet dl)
	target_link_libraries(ezdotnet_shared dl)
endif()

target_link_libraries(ezdotnet common)
target_link_libraries(ezdotnet_shared common)

install(TARGETS
	ezdotnet
	ezdotnet_shared
)
