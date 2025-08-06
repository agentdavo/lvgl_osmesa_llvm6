# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/djs/lvgl_osmesa_llvm6/ext/llvm-project/llvm"
  "/home/djs/lvgl_osmesa_llvm6/build-emscripten/llvm-build"
  "/home/djs/lvgl_osmesa_llvm6/build-emscripten/llvm_external-prefix"
  "/home/djs/lvgl_osmesa_llvm6/build-emscripten/llvm_external-prefix/tmp"
  "/home/djs/lvgl_osmesa_llvm6/build-emscripten/llvm_external-prefix/src/llvm_external-stamp"
  "/home/djs/lvgl_osmesa_llvm6/build-emscripten/llvm_external-prefix/src"
  "/home/djs/lvgl_osmesa_llvm6/build-emscripten/llvm_external-prefix/src/llvm_external-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/djs/lvgl_osmesa_llvm6/build-emscripten/llvm_external-prefix/src/llvm_external-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/djs/lvgl_osmesa_llvm6/build-emscripten/llvm_external-prefix/src/llvm_external-stamp${cfgdir}") # cfgdir has leading slash
endif()
