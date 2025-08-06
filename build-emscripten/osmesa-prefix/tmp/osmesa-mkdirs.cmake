# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/djs/lvgl_osmesa_llvm6/ext/mesa"
  "/home/djs/lvgl_osmesa_llvm6/build-emscripten/mesa-build"
  "/home/djs/lvgl_osmesa_llvm6/build-emscripten/osmesa-prefix"
  "/home/djs/lvgl_osmesa_llvm6/build-emscripten/osmesa-prefix/tmp"
  "/home/djs/lvgl_osmesa_llvm6/build-emscripten/osmesa-prefix/src/osmesa-stamp"
  "/home/djs/lvgl_osmesa_llvm6/build-emscripten/osmesa-prefix/src"
  "/home/djs/lvgl_osmesa_llvm6/build-emscripten/osmesa-prefix/src/osmesa-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/djs/lvgl_osmesa_llvm6/build-emscripten/osmesa-prefix/src/osmesa-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/djs/lvgl_osmesa_llvm6/build-emscripten/osmesa-prefix/src/osmesa-stamp${cfgdir}") # cfgdir has leading slash
endif()
