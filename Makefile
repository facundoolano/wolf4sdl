.PHONY: deps build run

deps:
	brew install cmake sdl2 sdl2_mixer

build:
	rm -rf build && mkdir build && cd build && cmake .. && make

run:
	cd data && ../build/engine
