# QoE-driven Joint Decision-Making for Multipath Adaptive Video Streaming

## Steps to reproduce the results

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
sudo apt-get install wget curl build-essential git cmake unzip tree screen pkg-config libssl-dev libbrotli-dev mininet python3-full python3-virtualenv python3-pip -y
```

+ Compile

```
mkdir -p ~/clarkzjw-globecom23 && cd ~/clarkzjw-globecom23

git clone https://github.com/h2o/picotls.git
git clone --branch globecom2023 https://github.com/clarkzjw/picoquic.git
git clone --branch globecom2023 https://github.com/clarkzjw/globecom2023.git

cd picotls && git submodule init && git submodule update && cmake -DCMAKE_C_FLAGS="-fPIC" . && make
cd ../picoquic && cmake -DCMAKE_C_FLAGS="-fPIC" . && make

cd ../globecom2023 && virtualenv .venv --python=python3 && source .venv/bin/activate
pip install -r requirements.txt

cmake . && make
```

### Dataset

Download our dataset, which takes about 20GB.

```
cd ~/clarkzjw-globecom23/
wget https://globecom23.jinwei.me/mpd.zip
unzip mpd.zip
```

The `sha256sum` of the dataset file `mpd.zip` is shown below.

```
95a58132043993a6382f3bdd10fd465cea4c6f565bc243669c00f26c5b6cc0e1  mpd.zip
```

### Generate certificates for QUIC

```bash
cd ~/clarkzjw-globecom23/globecom2023

openssl req -nodes -x509 -newkey rsa:2048 -days 365 -keyout ca-key.pem -out ca-cert.pem
```

### Run the experiments

```bash
sudo bash run.sh
```

The experiments will run using `screen`. You can use `screen -ls` and `screen -r` to attach to the corresponding running session.

### Generate figures

#### Generate corresponding figures in the paper

Run `figure_individual.m` within `./figure/gcloud` and `./figure/mininet` using MATLAB to generate the figures in the paper.

#### Generate figures from custom experiments
