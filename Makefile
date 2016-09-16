TOOLCHAIN=GCC_ARM
MCU=NUCLEO_F411RE
VOLUME_NAME=NODE_F411RE

container:
	docker build -t mtcaret/mbed:ver0.1 .

new: container
	docker run -w /tmp/build -it --rm mtcaret/mbed:ver0.1 mbed new . --mbedlib

build: clean container
	/usr/local/bin/docker run -v `pwd`/build:/tmp/build/.build/ -w /tmp/build -it --rm mtcaret/mbed:ver0.1 mbed compile -t $(TOOLCHAIN) -m $(MCU) --color

shell: container
	/usr/local/bin/docker run -w /tmp/build -it --rm mtcaret/mbed:ver0.1

clean:
	rm -rf build/*

write: build
	cp build/$(MCU)/GCC_ARM/build.bin /Volumes/$(VOLUME_NAME)/
	sync
	sleep 3
	sudo umount /Volumes/$(VOLUME_NAME)
