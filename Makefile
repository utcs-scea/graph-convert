run-tests: run-test
run-test:
	make -C build -j `nproc` && ctest --test-dir build --verbose
