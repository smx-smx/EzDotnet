function(generate_def)
    cmake_parse_arguments(ARG "" "IN_DLL;OUT_DEF" "" ${ARGN})

    execute_process(
        COMMAND ${DUMPBIN_EXE} /exports ${ARG_IN_DLL}
        OUTPUT_VARIABLE dumpbin_out
        COMMAND_ERROR_IS_FATAL ANY
    )
    string(REPLACE "\n" ";" lines "${dumpbin_out}")

    set(FILTERED_LINES "")
    foreach(line IN LISTS lines)
        if(NOT line MATCHES "^[ ]+[0-9]+[ ]+[0-9A-F]+[ ]+[0-9A-F]+[ ]+.*$")
            continue()
        endif()
        string(REGEX REPLACE "^.*[ ]+([^ ]+)$" "\\1" symbol "${line}")
        list(APPEND FILTERED_LINES "    ${symbol}")
    endforeach()

    if(FILTERED_LINES)
        list(INSERT FILTERED_LINES 0 "EXPORTS")
        string(REPLACE ";" "\n" FILE_CONTENT "${FILTERED_LINES}")
    else()
        set(FILE_CONTENT "")
    endif()

    file(WRITE "${ARG_OUT_DEF}" "${FILE_CONTENT}\n")
endfunction()

function(generate_lib)
    cmake_parse_arguments(ARG "" "IN_DEF;MACHINE;OUT_LIB" "" ${ARGN})
    execute_process(
        COMMAND ${LIB_EXE}
            /def:${ARG_IN_DEF}
            /machine:${ARG_MACHINE}
            /out:${ARG_OUT_LIB}
        COMMAND_ERROR_IS_FATAL ANY
    )
endfunction()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(MACHINE "x64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(MACHINE "x86")
else()
	message(FATAL_ERROR "Unknown architecture")
endif()

generate_def(
    IN_DLL ${IN_DLL}
    OUT_DEF ${OUT_DEF}
)
generate_lib(
    IN_DEF ${OUT_DEF}
    MACHINE ${MACHINE}
    OUT_LIB ${OUT_LIB}
)
