FROM ubuntu:22.04

RUN apt-get update && apt-get install wget curl build-essential git cmake pkg-config libssl-dev libbrotli-dev -y

WORKDIR /app

# git clone branch
RUN git clone https://github.com/h2o/picotls.git && \
    git clone --branch globecom2023 https://github.com/clarkzjw/picoquic.git && \
    cd picotls && git submodule init && git submodule update && cmake -DCMAKE_C_FLAGS="-fPIC" . && make && \
    cd ../picoquic && cmake -DCMAKE_C_FLAGS="-fPIC" . && make

COPY . /app/player

RUN cd /app/player && cmake . && make
