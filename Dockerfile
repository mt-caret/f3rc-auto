FROM library/ubuntu:16.04
MAINTAINER Masayuki Takeda

RUN apt-get update && apt-get install -y \
    software-properties-common \
    build-essential
RUN add-apt-repository -y ppa:team-gcc-arm-embedded/ppa && apt-get update
RUN apt-get install -y gcc-arm-embedded
CMD bash
