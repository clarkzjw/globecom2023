# QoE-driven Joint Decision-Making for Multipath Adaptive Video Streaming

## Steps to reproduce the results

### Dataset

Download our dataset, which takes about 20GB.

```
wget https://globecom23.jinwei.me/mpd.zip
```

### Compile and install

**Prerequisites**

Only tested on a clean installation of Ubuntu server 22.04.1 LTS.

+ Install Mininet, according to [the `native installation from source` guide](http://mininet.org/download/#option-2-native-installation-from-source).
```bash
cd ~
git clone https://github.com/mininet/mininet.git
cd mininet && git checkout -b 2.3.1b4


./mininet/util/install.sh -s 2.3.1b4 -a
```

Our experiments are conducted with Mininet 2.3.1b4. 

+ Install dependencies

```
sudo apt-get update
sudo apt-get install wget curl build-essential git cmake pkg-config libssl-dev libbrotli-dev mininet python3-full python3-pip -y
```

+ Compile

```
mkdir -p ~/clarkzjw-globecom23 && cd ~/clarkzjw-globecom23

git clone https://github.com/h2o/picotls.git
git clone --branch globecom2023 https://github.com/clarkzjw/picoquic.git
git clone --branch globecom2023 https://github.com/clarkzjw/globecom23.git

cd picotls && git submodule init && git submodule update && cmake -DCMAKE_C_FLAGS="-fPIC" . && make
cd ../picoquic && cmake -DCMAKE_C_FLAGS="-fPIC" . && make

cd ~/clarkzjw-globecom23/globecom23
virtualenv .venv --python=python3
pip install -r requirements.txt

cmake . && make
```
