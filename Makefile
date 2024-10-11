
RELEASE_BUILD_DIR ?= build
DEBUG_BUILD_DIR ?= dbuild
ASAN_BUILD_DIR ?= abuild
UBSAN_BUILD_DIR ?= ubuild

run-test: run-tests
run-tests: run-release-test run-debug-test run-asan-test run-ubsan-test

run-debug-test:
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${DEBUG_BUILD_DIR} make -C ${DEBUG_BUILD_DIR} -j `nproc` && ctest --test-dir ${DEBUG_BUILD_DIR} --verbose

run-release-test:
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${RELEASE_BUILD_DIR} make -C ${RELEASE_BUILD_DIR} -j `nproc` && ctest --test-dir ${RELEASE_BUILD_DIR} --verbose

run-asan-test:
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${ASAN_BUILD_DIR} make -C ${ASAN_BUILD_DIR} -j `nproc` && ctest --test-dir ${ASAN_BUILD_DIR} --verbose

run-ubsan-test:
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${UBSAN_BUILD_DIR} make -C ${UBSAN_BUILD_DIR} -j `nproc` && ctest --test-dir ${UBSAN_BUILD_DIR} --verbose
