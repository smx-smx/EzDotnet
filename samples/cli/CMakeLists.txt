add_executable(ezdotnet ezdotnet.c)

if(UNIX)
	target_link_libraries(ezdotnet dl)
endif()
