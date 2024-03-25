.PHONY : build clean rebuild run

build: hello_world

hello_world: hello_world.c
	clang \
		-I /usr/local/cuda-12.3/include \
		-L /usr/local/cuda-12.3/lib64 \
		-lOpenCL \
		-g \
		-o $@ \
		$<

clean:
	rm -f hello_world

rebuild: clean build

run: hello_world
	@echo "Running phase 1"
	./hello_world
