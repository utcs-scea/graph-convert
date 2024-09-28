
BUILD_DIR ?= build

run-tests: run-test
run-test:
	make -C build -j `nproc` && ctest --test-dir ${BUILD_DIR} --verbose
