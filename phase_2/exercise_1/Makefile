.PHONY : clean build rebuild run

build: mmul_main madd_main main

% : %.c
	clang \
		-I /usr/local/cuda-12.3/include \
		-L /usr/local/cuda-12.3/lib64 \
		-lOpenCL \
		-g \
		-o $@ \
		$<

clean:
	rm -f madd_main mmul_main main

rebuild: clean build

run: mmul_main madd_main main
	@echo "Running phase 2 exercise 1"
	@echo "Running \"madd_main\""
	./madd_main

	@echo "Running \"mmul_main\""
	./mmul_main
	
	@echo "Running \"main\""
	./main
