cmake_minimum_required(VERSION 3.26.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(graph-convert)

find_package(OpenMP REQUIRED)

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

FetchContent_Declare(
  virtual-library-context
  GIT_REPOSITORY https://github.com/AdityaAtulTewari/Virtual-Library-Context.git
  GIT_TAG 06e390fb5de29231f00cdb00b59ea990654993f1
)

FetchContent_MakeAvailable(virtual-library-context)

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

add_library(gconvert_shared SHARED src/el2bel.cpp)
target_include_directories(gconvert_shared PUBLIC include/)
set_property(TARGET gconvert_shared PROPERTY CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(TARGET gconvert_shared PROPERTY CXX_STANDARD 23)
target_compile_options(gconvert_shared PRIVATE -Wall -Werror ${OpenMP_CXX_FLAGS})
target_link_libraries(gconvert_shared PRIVATE ${OpenMP_CXX_FLAGS})
target_compile_features(gconvert_shared PUBLIC cxx_std_23)

add_executable(graph_convert src/main.cpp)
set_property(TARGET graph_convert PROPERTY CXX_STANDARD 23)
target_include_directories(graph_convert PRIVATE include/)
target_compile_options(graph_convert PRIVATE -Wall -Werror)
target_link_libraries(graph_convert PRIVATE gconvert)
target_compile_features(graph_convert PUBLIC cxx_std_23)

add_executable(vlc_trivial src/vlc_trivial.cpp)
set_property(TARGET vlc_trivial PROPERTY CXX_STANDARD 23)
target_include_directories(vlc_trivial PRIVATE include/)
target_compile_options(vlc_trivial PRIVATE -Wall -Werror)
target_link_libraries(vlc_trivial PRIVATE gconvert_shared virtual-library-context)
target_compile_features(vlc_trivial PUBLIC cxx_std_23)

enable_testing()

function(gconvert_add_test TARGET SOURCEFILE)
  add_executable(${TARGET} ${SOURCEFILE})
  target_link_libraries(${TARGET} gtest gtest_main gconvert)
  target_include_directories(${TARGET} PRIVATE include/)
  target_compile_features(${TARGET} PUBLIC cxx_std_23)
  add_test(NAME ${TARGET}_test COMMAND ${TARGET})
endfunction()

function(gconvert_bin_test TARGET CONVERTTYPE INPUT ACTUAL EXPECTED)
  add_test(NAME ${TARGET}-${CONVERTTYPE}-${INPUT}-${ACTUAL}-${EXPECTED}
    COMMAND bash -c "diff -Z <($<TARGET_FILE:${TARGET}> -t ${CONVERTTYPE} -i ${INPUT} -o ${ACTUAL} \
      && ls ${ACTUAL}-* \
      | awk -F- -v OFS=- '{printf $NF; for (i = 1; i <= NF; i++) printf \"-%s\", $i; printf \"\\n\"}' \
      | sort -n -k1 | awk -F- -v OFS=- '{printf $2; for(i = 3; i <= NF; i++) printf \"-%s\", $i; printf \"\\n\"}' \
      | xargs cat) ${EXPECTED} \
    && rm -rf ${ACTUAL}-*")
endfunction()

function(gconvert_bin_incremental_el2bel_test TARGET SIZE)
  set(OUTPUT "incremental_el2bel_test-${TARGET}-${SIZE}.bel")
  set(INPUT "incremental_el2bel_test-${TARGET}-${SIZE}.el")
  set(GENSCRIPT "${CMAKE_SOURCE_DIR}/scripts/generate_incremental_output.go")
  add_test(NAME incremental_el2bel_test-${TARGET}-${SIZE}
    COMMAND bash -c "rm -rf ${INPUT} ${OUTPUT} ${OUTPUT}-expected ${OUTPUT}-* \
    && go run ${GENSCRIPT} -n ${SIZE} > ${INPUT} \
    && go run ${GENSCRIPT} -n ${SIZE} -b > expected-${OUTPUT} \
    && $<TARGET_FILE:${TARGET}> \
        -t el2bel \
        -i ${INPUT} \
        -o ${OUTPUT} \
    && ls ${OUTPUT}-* \
      | awk -F- -v OFS=- '{printf $NF; for (i = 1; i <= NF; i++) printf \"-%s\", $i; printf \"\\n\"}' \
      | sort -n -k1 | awk -F- -v OFS=- '{printf $2; for(i = 3; i <= NF; i++) printf \"-%s\", $i; printf \"\\n\"}' \
      | xargs cat > ${OUTPUT} \
    && diff \
      ${OUTPUT} \
      expected-${OUTPUT} \
    && rm -rf ${INPUT} ${OUTPUT}-* ${OUTPUT} expected-${OUTPUT}")
endfunction()

gconvert_add_test(mmap_helper test/mmap_helper.cpp)
gconvert_bin_test(graph_convert el2bel ${CMAKE_SOURCE_DIR}/input/increment-64.el increment-64.bel ${CMAKE_SOURCE_DIR}/expected/increment-64.bel)
gconvert_bin_test(graph_convert el2bel ${CMAKE_SOURCE_DIR}/input/increment-128.el increment-128.bel ${CMAKE_SOURCE_DIR}/expected/increment-128.bel)
gconvert_bin_test(graph_convert el2bel ${CMAKE_SOURCE_DIR}/input/increment-256.el increment-256.bel ${CMAKE_SOURCE_DIR}/expected/increment-256.bel)
gconvert_bin_test(graph_convert el2bel ${CMAKE_SOURCE_DIR}/input/increment-512.el increment-512.bel ${CMAKE_SOURCE_DIR}/expected/increment-512.bel)
gconvert_bin_incremental_el2bel_test(graph_convert 32)
gconvert_bin_incremental_el2bel_test(graph_convert 64)
gconvert_bin_incremental_el2bel_test(graph_convert 128)
gconvert_bin_incremental_el2bel_test(graph_convert 256)
gconvert_bin_incremental_el2bel_test(graph_convert 512)
gconvert_bin_incremental_el2bel_test(graph_convert 1024)
gconvert_bin_incremental_el2bel_test(graph_convert 2048)
gconvert_bin_incremental_el2bel_test(graph_convert 4096)
gconvert_bin_incremental_el2bel_test(graph_convert 8192)
gconvert_bin_incremental_el2bel_test(vlc_trivial 32)
gconvert_bin_incremental_el2bel_test(vlc_trivial 64)
gconvert_bin_incremental_el2bel_test(vlc_trivial 128)
gconvert_bin_incremental_el2bel_test(vlc_trivial 256)
gconvert_bin_incremental_el2bel_test(vlc_trivial 512)
gconvert_bin_incremental_el2bel_test(vlc_trivial 1024)
gconvert_bin_incremental_el2bel_test(vlc_trivial 2048)
gconvert_bin_incremental_el2bel_test(vlc_trivial 4096)
gconvert_bin_incremental_el2bel_test(vlc_trivial 8192)
gconvert_bin_incremental_el2bel_test(vlc_trivial 16384)
gconvert_bin_incremental_el2bel_test(vlc_trivial 32768)
gconvert_bin_incremental_el2bel_test(vlc_trivial 65536)
option(BIG_TEST "Run big tests" OFF)
if(BIG_TEST)
gconvert_bin_incremental_el2bel_test(graph_convert 16384)
gconvert_bin_incremental_el2bel_test(graph_convert 32768)
gconvert_bin_incremental_el2bel_test(graph_convert 65536)
gconvert_bin_incremental_el2bel_test(graph_convert 1073741824)
endif()
