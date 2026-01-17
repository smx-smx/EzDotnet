get_filename_component(DLL_NAME ${IN_DLL} NAME)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(MACHINE "i386:x86-64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(MACHINE "i386")
else()
	message(FATAL_ERROR "Unknown architecture")
endif()

execute_process(
	COMMAND ${GENDEF_EXE} - ${IN_DLL}
	OUTPUT_FILE ${OUT_DEF}
	COMMAND_ERROR_IS_FATAL ANY)

execute_process(
	COMMAND ${DLLTOOL_EXE} -d ${OUT_DEF} -D ${DLL_NAME} -l ${OUT_LIB} -m ${MACHINE}
	COMMAND_ERROR_IS_FATAL ANY)