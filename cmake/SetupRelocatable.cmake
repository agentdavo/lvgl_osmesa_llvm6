# SetupRelocatable.cmake - Configure build for relocatable installation

# Function to configure executable for relocatable builds
function(setup_relocatable_target TARGET)
    if(BUILD_RELOCATABLE)
        # Linux/Unix: Use $ORIGIN for relative RPATH
        if(UNIX AND NOT APPLE)
            set_target_properties(${TARGET} PROPERTIES
                BUILD_RPATH "${CMAKE_BINARY_DIR}/llvm-install/lib;${CMAKE_BINARY_DIR}/mesa-install/lib/x86_64-linux-gnu"
                INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib:$ORIGIN/../../llvm-install/lib:$ORIGIN/../../mesa-install/lib/x86_64-linux-gnu"
                BUILD_WITH_INSTALL_RPATH FALSE
                INSTALL_RPATH_USE_LINK_PATH FALSE
            )
        # macOS: Use @loader_path
        elseif(APPLE)
            set_target_properties(${TARGET} PROPERTIES
                BUILD_RPATH "${CMAKE_BINARY_DIR}/llvm-install/lib;${CMAKE_BINARY_DIR}/mesa-install/lib"
                INSTALL_RPATH "@loader_path;@loader_path/../lib;@loader_path/../../llvm-install/lib;@loader_path/../../mesa-install/lib"
                BUILD_WITH_INSTALL_RPATH FALSE
                INSTALL_RPATH_USE_LINK_PATH FALSE
                MACOSX_RPATH TRUE
            )
        endif()
    endif()
endfunction()

# Configure global RPATH settings for relocatable builds
if(BUILD_RELOCATABLE)
    # Don't skip the full RPATH for the build tree
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    
    # When building, use the install RPATH already
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    
    # The RPATH to be used when installing
    if(UNIX AND NOT APPLE)
        set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/../../llvm-install/lib:$ORIGIN/../../mesa-install/lib/x86_64-linux-gnu")
    elseif(APPLE)
        set(CMAKE_INSTALL_RPATH "@loader_path/../lib:@loader_path/../../llvm-install/lib:@loader_path/../../mesa-install/lib")
    endif()
    
    # Add the automatically determined parts of the RPATH
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
endif()

# Generate a setup script for relocatable builds
if(BUILD_RELOCATABLE)
    configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/setup_env.sh.in"
        "${CMAKE_BINARY_DIR}/setup_env.sh"
        @ONLY
    )
endif()