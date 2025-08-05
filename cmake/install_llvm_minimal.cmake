# Install LLVM using the standard install target
execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${LLVM_BINARY_DIR} --target install
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "LLVM install failed")
endif()

message(STATUS "LLVM installed, now minimizing for Mesa llvmpipe...")

# Minimal LLVM libraries for Mesa llvmpipe
set(ESSENTIAL_LIBS
    # Core libraries for Mesa
    libLLVMCore.a
    libLLVMAnalysis.a
    libLLVMCodeGen.a
    libLLVMExecutionEngine.a
    libLLVMInstCombine.a
    libLLVMMC.a
    libLLVMMCDisassembler.a
    libLLVMMCJIT.a
    libLLVMObject.a
    libLLVMScalarOpts.a
    libLLVMSupport.a
    libLLVMTransformUtils.a
    libLLVMTarget.a
    libLLVMX86CodeGen.a
    libLLVMX86Desc.a
    libLLVMX86Info.a
    libLLVMX86AsmParser.a
    libLLVMX86Disassembler.a
    libLLVMBitReader.a
    libLLVMBitWriter.a
    # Additional libs needed by llvm-config and Mesa
    libLLVMCoroutines.a
    libLLVMLTO.a
    libLLVMPasses.a
    libLLVMipo.a
    libLLVMInstrumentation.a
    libLLVMVectorize.a
    libLLVMLinker.a
    libLLVMIRReader.a
    libLLVMAsmParser.a
    libLLVMFrontendOpenMP.a
    libLLVMAsmPrinter.a
    libLLVMSelectionDAG.a
    libLLVMX86AsmPrinter.a
    libLLVMX86Utils.a
    libLLVMMCParser.a
    libLLVMAggressiveInstCombine.a
    libLLVMInstCombine.a
    libLLVMScalarOpts.a
    libLLVMTransformUtils.a
    libLLVMAnalysis.a
    libLLVMProfileData.a
    libLLVMDebugInfoCodeView.a
    libLLVMDebugInfoMSF.a
    libLLVMGlobalISel.a
    libLLVMBinaryFormat.a
    libLLVMRemarks.a
    libLLVMBitstreamReader.a
    libLLVMRuntimeDyld.a
    libLLVMMCA.a
    libLLVMMCDisassembler.a
    libLLVMMC.a
    libLLVMDebugInfoDWARF.a
    libLLVMObject.a
    libLLVMTextAPI.a
    libLLVMBitReader.a
    libLLVMCore.a
    libLLVMBinaryFormat.a
    libLLVMSupport.a
    libLLVMDemangle.a
)

# Create a temporary directory for minimal libs
file(MAKE_DIRECTORY ${LLVM_INSTALL_DIR}/lib/minimal)

# Copy only essential libraries
message(STATUS "Keeping only essential libraries:")
foreach(lib ${ESSENTIAL_LIBS})
    if(EXISTS ${LLVM_INSTALL_DIR}/lib/${lib})
        message(STATUS "  - ${lib}")
        file(COPY ${LLVM_INSTALL_DIR}/lib/${lib} DESTINATION ${LLVM_INSTALL_DIR}/lib/minimal/)
        execute_process(
            COMMAND strip --strip-unneeded ${LLVM_INSTALL_DIR}/lib/minimal/${lib}
            RESULT_VARIABLE strip_result
            ERROR_QUIET
        )
    else()
        message(WARNING "  ! Missing: ${lib}")
    endif()
endforeach()

# Count libraries before removal
file(GLOB ALL_LIBS ${LLVM_INSTALL_DIR}/lib/*.a)
list(LENGTH ALL_LIBS TOTAL_LIBS)
message(STATUS "Total libraries before: ${TOTAL_LIBS}")

# Remove all shared libraries
file(GLOB SHARED_LIBS ${LLVM_INSTALL_DIR}/lib/*.so)
if(SHARED_LIBS)
    file(REMOVE ${SHARED_LIBS})
endif()

# Remove all original static libraries
file(GLOB ORIG_LIBS ${LLVM_INSTALL_DIR}/lib/*.a)
if(ORIG_LIBS)
    file(REMOVE ${ORIG_LIBS})
endif()

# Move minimal libs back
file(GLOB MINIMAL_LIBS ${LLVM_INSTALL_DIR}/lib/minimal/*.a)
foreach(lib ${MINIMAL_LIBS})
    get_filename_component(libname ${lib} NAME)
    file(RENAME ${lib} ${LLVM_INSTALL_DIR}/lib/${libname})
endforeach()

# Remove the minimal directory
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/lib/minimal)

# Count libraries after
file(GLOB FINAL_LIBS ${LLVM_INSTALL_DIR}/lib/*.a)
list(LENGTH FINAL_LIBS KEPT_LIBS)
message(STATUS "Libraries kept: ${KEPT_LIBS}")

# Keep only llvm-config in bin directory
file(GLOB BIN_FILES ${LLVM_INSTALL_DIR}/bin/*)
foreach(bin_file ${BIN_FILES})
    get_filename_component(bin_name ${bin_file} NAME)
    if(NOT bin_name STREQUAL "llvm-config")
        file(REMOVE ${bin_file})
    endif()
endforeach()

# Remove unnecessary directories
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/share)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/libexec)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/docs)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/lib/cmake)

# Clean up include directory - remove non-essential headers
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/DebugInfo)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/Testing)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/ToolDrivers)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/WindowsDriver)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/WindowsManifest)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/WindowsResource)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/XRay)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/ExecutionEngine/JITLink)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/ExecutionEngine/Orc)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/InterfaceStub)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/LTO)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/MCA)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/Remarks)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/DWARFLinker)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/FileCheck)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/FuzzMutate)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/LineEditor)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/TableGen)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/TextAPI)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/Coverage)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/DWP)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/ProfileData)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/ObjectYAML)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm/Option)
file(REMOVE_RECURSE ${LLVM_INSTALL_DIR}/include/llvm-c)

# Get size of minimized installation
execute_process(
    COMMAND du -sh ${LLVM_INSTALL_DIR}
    OUTPUT_VARIABLE INSTALL_SIZE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "LLVM minimization complete")
message(STATUS "Installation size: ${INSTALL_SIZE}")