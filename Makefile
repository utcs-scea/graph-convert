
BUILD_DIR ?= build

run-tests: run-test
run-test:
	make -C ${BUILD_DIR} -j `nproc` && ctest --test-dir ${BUILD_DIR} --verbose
