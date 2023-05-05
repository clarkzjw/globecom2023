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

But then, you have to update `bitrate_mapping` in `[bitrate.py](./bitrate.py)` with the custom bitrate ladder accordingly.

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

Run `gen_figure.m` within `[./figure/gcloud](./figure/gcloud)` and `[./figure/mininet](./figure/mininet)` using MATLAB to generate the figures in the paper.

### Run experiments on real-world network

In order to run the experiments on real-world multipath testbeds, the following variables in `[downloader.py](./downloader.py)` have to be updated accordingly.

`if_name_mapping`
`default_mpd_url`
`default_host`
`default_port`

and run `main.py` with custom options.

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

The metric results will be saved in `./results/` in json format.
