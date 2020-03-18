.PHONY: deps build run unifdef

deps:
	brew install cmake sdl2 sdl2_mixer

build:
	rm -rf build && mkdir build && cd build && cmake .. && make

run:
	cd data && ../build/engine

# remove code inside unused macros e.g.
# make unifdef ARGS="-U USE_SHADING -U USE_HIRES"
unifdef:
	find . -name '*.h' -o -name '*.cpp' | xargs -I {} unifdef $(ARGS) -o {} {}
