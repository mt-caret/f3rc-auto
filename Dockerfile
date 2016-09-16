FROM library/ubuntu:16.04
MAINTAINER Masayuki Takeda

RUN apt-get update && apt-get install -y \
    software-properties-common \
    build-essential \
    python python-pip \
    git mercurial
RUN pip install --upgrade pip
RUN add-apt-repository -y ppa:team-gcc-arm-embedded/ppa && apt-get update
RUN apt-get install -y gcc-arm-embedded
RUN pip install mbed-cli
WORKDIR /tmp/build
RUN mbed new . --mbedlib
RUN mbed add https://developer.mbed.org/users/simon/code/TextLCD/
RUN mbed add https://developer.mbed.org/users/Kemp/code/mcp3208/
RUN mbed add https://developer.mbed.org/users/okano/code/SB1602E/
RUN mbed add https://developer.mbed.org/users/aberk/code/PID/
COPY src/* /tmp/build/
CMD bash
