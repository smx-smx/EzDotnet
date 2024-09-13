ExternalProject_Add(ezdotnet
	SOURCE_DIR ${PARENT}
	CMAKE_ARGS 
		-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
		-DBUILD_MANAGED_SAMPLE=ON
)

test_target(
	ezdotnet
	ezdotnet
	ezdotnet
	coreclrhost
)

if(MONO_FOUND)
	test_target(
		ezdotnet
		ezdotnet
		ezdotnet
		MonoHost
	)
endif()
