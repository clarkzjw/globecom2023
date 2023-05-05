# QoE-driven Joint Decision-Making for Multipath Adaptive Video Streaming

This repository contains the code for the submission "QoE-driven Joint Decision-Making for Multipath Adaptive Video Streaming" to Globecom 2023.

## Steps to reproduce the results

### Compile and install

**Prerequisites**

Only tested on a clean installation of Ubuntu server 22.04.1 LTS.

+ Install Mininet, according to the [`native installation from source`](http://mininet.org/download/#option-2-native-installation-from-source) guide.
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

+ Use our prebuilt dataset

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

+ Create custom DASH dataset

Follow the instructions in the [`dataset`](./dataset) folder to create your own DASH dataset.

But then, you have to update `bitrate_mapping` in [`bitrate.py`](./bitrate.py) with the custom bitrate ladder accordingly.

### Generate certificates for QUIC

```bash
cd ~/clarkzjw-globecom23/globecom2023

openssl req -nodes -x509 -newkey rsa:2048 -days 365 -keyout ca-key.pem -out ca-cert.pem
```

### Run Mininet emulations

```bash
sudo bash run.sh
```

The experiments will run using `screen` with root user. You can use `sudo screen -ls` and `sudo screen -r` to attach to the corresponding running session.

### Generate figures

#### Generate corresponding figures in the paper

Run `gen_figure.m` within [`./figure/gcloud`](./figure/gcloud) and [`./figure/mininet`](./figure/mininet) using MATLAB to generate the figures in the paper.

### Run experiments on real-world network

The Terraform scripts in [`./terraform`](./terraform) are used in our experiment to create a multipath testbed on GCP's `us-west-1a` zone which is geographically closest to our location.

The traceroute results below shows there are two distinct paths from our location to the VM.

```
traceroute to 34.105.96.243 (34.105.96.243), 30 hops max, 60 byte packets
1  _gateway (192.168.0.254)  0.539 ms  0.462 ms  0.426 ms
2  142.104.68.1 (142.104.68.1)  0.913 ms  0.873 ms  0.839 ms
3  142.104.124.105 (142.104.124.105)  1.080 ms  1.047 ms  1.017 ms
4  142.104.100.241 (142.104.100.241)  0.977 ms  1.065 ms  1.023 ms
5  cle-core-edge.bb.uvic.ca (142.104.100.189)  1.308 ms  1.270 ms  1.236 ms
6  207.23.244.233 (207.23.244.233)  1.199 ms  1.286 ms  1.179 ms
7  vctr3rtr2.network.canarie.ca (199.212.24.98)  1.287 ms  1.405 ms  1.669 ms
8  sttl1rtr2.canarie.ca (206.81.80.189)  3.596 ms  3.493 ms  3.597 ms
9  google-2-lo-std-707.sttlwa.pacificwave.net (207.231.242.22)  10.022 ms  11.144 ms  10.892 ms
10  243.96.105.34.bc.googleusercontent.com (34.105.96.243)  9.987 ms  9.449 ms  8.410 ms

traceroute to 34.105.96.243 (34.105.96.243), 30 hops max, 60 byte packets
 1  DD-WRT (192.168.1.1)  0.250 ms  0.248 ms  0.309 ms
 2  100.64.0.1 (100.64.0.1)  47.095 ms  68.122 ms  68.095 ms
 3  172.16.251.66 (172.16.251.66)  68.104 ms  68.076 ms  68.048 ms
 4  * * *
 5  undefined.hostname.localhost (206.224.64.13)  68.006 ms undefined.hostname.localhost (206.224.64.37)  67.978 ms undefined.hostname.localhost (206.224.64.13)  67.946 ms
 6  142.250.170.144 (142.250.170.144)  67.933 ms 142.250.163.222 (142.250.163.222)  65.939 ms 142.250.170.144 (142.250.170.144)  65.821 ms
 7  243.96.105.34.bc.googleusercontent.com (34.105.96.243)  76.793 ms  40.952 ms  70.777 ms
```

But our code can be deployed on any suitable multipath testbeds with the following requirements.

The following variables in [`downloader.py`](./downloader.py) have to be updated accordingly.

```
`if_name_mapping`
`default_mpd_url`
`default_host`
`default_port`
```

and run [`main.py`](./main.py) with custom options.

```bash
$ python3 main.py --help   

usage: main.py [-h] [--exp_id EXP_ID] [--scheduler {roundrobin,minrtt,contextual_bandit}] [--algorithm {LinUCB,LinTS,LinGreedy}] [--linucb_alpha LINUCB_ALPHA] [--lints_alpha LINTS_ALPHA]
               [--egreedy_epsilon EGREEDY_EPSILON] [--nb_segment NB_SEGMENT]

Adaptive video streaming with contextual bandits and MPQUIC

options:
  -h, --help            show this help message and exit
  --exp_id EXP_ID       experiment id
  --scheduler {roundrobin,minrtt,contextual_bandit}
                        scheduler
  --algorithm {LinUCB,LinTS,LinGreedy}
                        scheduling algorithm
  --linucb_alpha LINUCB_ALPHA
                        alpha for LinUCB
  --lints_alpha LINTS_ALPHA
                        alpha for LinTS
  --egreedy_epsilon EGREEDY_EPSILON
                        epsilon for epsilon-greedy
  --nb_segment NB_SEGMENT
                        number of segments to download
```

The metric results will be saved in [`./result/`](./result) in json format and can be used to generate figures using scripts in [`analysis`](./analysis).
