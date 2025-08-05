# RelocatableConfig.cmake - Make the build relocatable
# This file provides functions to convert absolute paths to relative paths

# Function to make paths relative to the build directory
function(make_relocatable_path ABSOLUTE_PATH OUTPUT_VAR)
    # Get the relative path from build dir to the absolute path
    file(RELATIVE_PATH REL_PATH "${CMAKE_BINARY_DIR}" "${ABSOLUTE_PATH}")
    
    # If the path is already relative or outside our tree, keep it as-is
    if(IS_ABSOLUTE "${REL_PATH}" OR REL_PATH MATCHES "^\\.\\./")
        set(${OUTPUT_VAR} "${ABSOLUTE_PATH}" PARENT_SCOPE)
    else
        # Make it relative to CMAKE_BINARY_DIR
        set(${OUTPUT_VAR} "\${CMAKE_BINARY_DIR}/${REL_PATH}" PARENT_SCOPE)
    endif()
endfunction()

# Function to process LLVM libraries and make them relocatable
function(make_llvm_libs_relocatable LLVM_LIBS_ABSOLUTE OUTPUT_VAR)
    set(RELOCATABLE_LIBS "")
    foreach(lib ${LLVM_LIBS_ABSOLUTE})
        if(EXISTS "${lib}")
            file(RELATIVE_PATH REL_LIB "${CMAKE_BINARY_DIR}" "${lib}")
            if(NOT IS_ABSOLUTE "${REL_LIB}" AND NOT REL_LIB MATCHES "^\\.\\./")
                list(APPEND RELOCATABLE_LIBS "\${CMAKE_BINARY_DIR}/${REL_LIB}")
            else
                list(APPEND RELOCATABLE_LIBS "${lib}")
            endif()
        else
            list(APPEND RELOCATABLE_LIBS "${lib}")
        endif()
    endforeach()
    set(${OUTPUT_VAR} "${RELOCATABLE_LIBS}" PARENT_SCOPE)
endfunction()

# Set up variables for relocatable builds
set(RELOCATABLE_BUILD_DIR "\${CMAKE_BINARY_DIR}")
set(RELOCATABLE_LLVM_INSTALL_DIR "\${CMAKE_BINARY_DIR}/llvm-install")
set(RELOCATABLE_MESA_INSTALL_DIR "\${CMAKE_BINARY_DIR}/mesa-install")
set(RELOCATABLE_LLVM_CONFIG "\${CMAKE_BINARY_DIR}/llvm-install/bin/llvm-config")