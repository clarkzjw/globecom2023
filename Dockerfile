FROM ubuntu:22.04

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get install wget curl build-essential git cmake pkg-config \
    libssl-dev libbrotli-dev mininet python3-full python3-pip -y

WORKDIR /app

# git clone branch
RUN git clone https://github.com/h2o/picotls.git && \
    git clone --branch globecom2023 https://github.com/clarkzjw/picoquic.git && \
    cd picotls && git submodule init && git submodule update && cmake -DCMAKE_C_FLAGS="-fPIC" . && make && \
    cd ../picoquic && cmake -DCMAKE_C_FLAGS="-fPIC" . && make

COPY requirements.txt /app/player/

RUN pip install -r /app/player/requirements.txt

COPY . /app/player

RUN cd /app/player && cmake . && make
