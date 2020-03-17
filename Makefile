build:
	rm -rf build && mkdir build && cd build && cmake .. && make

run:
	cd data && ../build/engine
