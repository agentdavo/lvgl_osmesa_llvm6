# GetLLVMLibs.cmake - Get LLVM libraries and make them relocatable

if(NOT LLVM_CONFIG)
    message(FATAL_ERROR "LLVM_CONFIG not specified")
endif()

if(NOT OUTPUT_FILE)
    message(FATAL_ERROR "OUTPUT_FILE not specified")
endif()

# Execute llvm-config to get library files
execute_process(
    COMMAND ${LLVM_CONFIG} --libfiles
    OUTPUT_VARIABLE LLVM_LIBS_RAW
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE LLVM_CONFIG_RESULT
)

if(NOT LLVM_CONFIG_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to run llvm-config")
endif()

# Convert to a list
string(REPLACE " " ";" LLVM_LIBS_LIST "${LLVM_LIBS_RAW}")

# Make paths relocatable
set(RELOCATABLE_LIBS "")
foreach(lib ${LLVM_LIBS_LIST})
    # Get just the filename
    get_filename_component(lib_name ${lib} NAME)
    # Assume it's in llvm-install/lib
    list(APPEND RELOCATABLE_LIBS "\${CMAKE_BINARY_DIR}/llvm-install/lib/${lib_name}")
endforeach()

# Write to output file
file(WRITE ${OUTPUT_FILE} "# Generated LLVM libraries list\n")
file(APPEND ${OUTPUT_FILE} "set(LLVM_LIBS\n")
foreach(lib ${RELOCATABLE_LIBS})
    file(APPEND ${OUTPUT_FILE} "    ${lib}\n")
endforeach()
file(APPEND ${OUTPUT_FILE} ")\n")