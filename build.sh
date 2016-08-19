#!/bin/bash
set -e
docker build -t mtcaret/mbed:ver0.1 .
docker run -v `pwd`:/tmp/build -w /tmp/build -it --rm mtcaret/mbed:ver0.1 make
