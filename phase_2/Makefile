.PHONY : build clean rebuild run

build:
	$(MAKE) -C ./exercise_1 build
	$(MAKE) -C ./exercise_2 build
	$(MAKE) -C ./exercise_3 build

clean:
	$(MAKE) -C ./exercise_1 clean
	$(MAKE) -C ./exercise_2 clean
	$(MAKE) -C ./exercise_3 clean

rebuild: clean build

run:
	$(MAKE) -C ./exercise_1 run
	$(MAKE) -C ./exercise_2 run
	$(MAKE) -C ./exercise_3 run
