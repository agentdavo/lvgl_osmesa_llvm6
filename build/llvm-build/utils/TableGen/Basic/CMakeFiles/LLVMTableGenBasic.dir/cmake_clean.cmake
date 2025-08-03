file(REMOVE_RECURSE
  "../../../lib/libLLVMTableGenBasic.a"
  "../../../lib/libLLVMTableGenBasic.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/LLVMTableGenBasic.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
