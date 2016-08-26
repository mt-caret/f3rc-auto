TOOLCHAIN=GCC_ARM
MCU=LPC1768

container:
	docker build -t mtcaret/mbed:ver0.1 .

new: container
	docker run -w /tmp/build -it --rm mtcaret/mbed:ver0.1 mbed new . --mbedlib

build: container
	/usr/local/bin/docker run -v `pwd`/build:/tmp/build/.build/ -w /tmp/build -it --rm mtcaret/mbed:ver0.1 mbed compile -t $(TOOLCHAIN) -m $(MCU)

shell: container
	/usr/local/bin/docker run -w /tmp/build -it --rm mtcaret/mbed:ver0.1

clean:
	rm -rf build/*
