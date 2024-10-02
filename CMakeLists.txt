cmake_minimum_required(VERSION 3.26.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(graph-convert)

find_package(OpenMP REQUIRED)

#create the sanitize build type
set(CMAKE_CXX_FLAGS_ASAN
  "-O3 -fsanitize=address -DNDEBUG" CACHE STRING "Flags used by the C++ compiler during sanitizer builds"
  FORCE )
set(CMAKE_C_FLAGS_ASAN
  "-O3 -fsanitize=address -DNDEBUG" CACHE STRING "Flags used by the C compiler during sanitizer builds"
  FORCE )
set(CMAKE_EXE_LINKER_FLAGS_ASAN
  "" CACHE STRING "Flags used for linking binaries during sanitizer builds"
  FORCE )
set(CMAKE_SHARED_LINKER_FLAGS_ASAN
  "" CACHE STRING "Flags used for linking shared libraries during sanitizer builds"
  FORCE )

MARK_AS_ADVANCED(
  CMAKE_CXX_FLAGS_ASAN
  CMAKE_C_FLAGS_ASAN
  CMAKE_EXE_LINKER_FLAGS_ASAN
  CMAKE_SHARED_LINKER_FLAGS_ASAN)

set(CMAKE_CXX_FLAGS_UBSAN
  "-O3 -fsanitize=undefined -DNDEBUG" CACHE STRING "Flags used by the C++ compiler during sanitizer builds"
  FORCE )
set(CMAKE_C_FLAGS_UBSAN
  "-O3 -fsanitize=undefined -DNDEBUG" CACHE STRING "Flags used by the C compiler during sanitizer builds"
  FORCE )
set(CMAKE_EXE_LINKER_FLAGS_UBSAN
  "" CACHE STRING "Flags used for linking binaries during sanitizer builds"
  FORCE )
set(CMAKE_SHARED_LINKER_FLAGS_UBSAN
  "" CACHE STRING "Flags used for linking shared libraries during sanitizer builds"
  FORCE )

MARK_AS_ADVANCED(
  CMAKE_CXX_FLAGS_UBSAN
  CMAKE_C_FLAGS_UBSAN
  CMAKE_EXE_LINKER_FLAGS_UBSAN
  CMAKE_SHARED_LINKER_FLAGS_UBSAN)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(gconvert STATIC src/el2bel.cpp)
target_include_directories(gconvert PUBLIC include/)
set_property(TARGET gconvert PROPERTY CXX_STANDARD 23)
target_compile_options(gconvert PRIVATE -Wall -Werror ${OpenMP_CXX_FLAGS})
target_link_libraries(gconvert PRIVATE ${OpenMP_CXX_FLAGS})
target_compile_features(gconvert PUBLIC cxx_std_23)

add_executable(graph_convert src/main.cpp)
set_property(TARGET graph_convert PROPERTY CXX_STANDARD 23)
target_include_directories(graph_convert PRIVATE include/)
target_compile_options(graph_convert PRIVATE -Wall -Werror)
target_link_libraries(graph_convert PRIVATE gconvert)
target_compile_features(graph_convert PUBLIC cxx_std_23)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(BUILD_GTEST ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

function(gconvert_add_test TARGET SOURCEFILE)
  add_executable(${TARGET} ${SOURCEFILE})
  target_link_libraries(${TARGET} gtest gtest_main gconvert)
  target_include_directories(${TARGET} PRIVATE include/)
  target_compile_features(${TARGET} PUBLIC cxx_std_23)
  add_test(NAME ${TARGET}_test COMMAND ${TARGET})
endfunction()

gconvert_add_test(mmap_helper test/mmap_helper.cpp)
