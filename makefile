.PHONY = cbuild, run

build-server:
	cmake --build ./build --target server

run-server:
	./build/server
