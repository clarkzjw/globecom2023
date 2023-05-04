"""
             ------ h2
h1 ------ r1
             ------ h2
h1-eth0: 10.0.1.2

h2-eth0: 10.0.2.2
h2-eth1: 10.0.3.2

- Scenario:
    - path 1 (eth0): high bandwidth, high latency -> starlink
    - path 2 (eth1): low bandwidth, low latency -> broadband Internet in rural areas
    - for minRTT
        - it tends to choose path 2, but will cause more buffer
    - for RR
        - for bandwidth estimation, tends to choose path 1, but higher delay
"""
import time
import argparse
import mininet.node
from mininet.link import TCLink
from mininet.log import setLogLevel
from mininet.net import Mininet
from config import bw_high, bw_low, latency_high, latency_low
from multiprocessing import Process
import numpy as np
from mininet.cli import CLI


iface1 = "h2-eth0"
iface2 = "h2-eth1"
bw_sigma = 5


def change_bw(node: mininet.node.Host, link: str, bw: int):
    for intf in node.intfList():
        if intf.link and str(intf) == link:
            intfs = [intf.link.intf1, intf.link.intf2]
            intfs[0].config(bw=bw)
            intfs[1].config(bw=bw)


def change_latency(node: mininet.node.Host, link: str, latency: int):
    for intf in node.intfList():
        if intf.link and str(intf) == link:
            intfs = [intf.link.intf1, intf.link.intf2]
            intfs[0].config(delay="{}ms".format(latency))
            intfs[1].config(delay="{}ms".format(latency))


def change_bw_normal_distribution(node: mininet.node.Host):
    while True:
        change_bw(node, iface1, int(np.random.normal(bw_high, bw_sigma)))
        change_bw(node, iface2, int(np.random.normal(bw_low, bw_sigma)))
        time.sleep(5)


def change_latency_normal_distribution(node: mininet.node.Host):
    while True:
        latency1 = int(np.random.normal(50, 5))
        latency2 = int(np.random.normal(10, 1))
        print("latency1: {}, latency2: {}".format(latency1, latency2))
        change_latency(node, iface1, latency1)
        change_latency(node, iface2, latency2)
        time.sleep(1)


def change_bw_fluctuation(node: mininet.node.Host):
    while True:
        change_bw(node, iface1, bw_high)
        time.sleep(5)
        change_bw(node, iface1, bw_low)
        time.sleep(5)


def change_path_bw(node: mininet.node.Host):
    change_bw_normal_distribution(node)
    # change_bw_fluctuation(node)


def change_path_latency(node: mininet.node.Host):
    change_latency_normal_distribution(node)


def start_server(server: mininet.node.Host):
    server.cmd("bash server.sh")


def ping(node: mininet.node.Host):
    while True:
        node.cmdPrint("ping -c 1 10.0.1.2 -I h2-eth0")
        time.sleep(1)


if '__main__' == __name__:
    parser = argparse.ArgumentParser(description="Adaptive video streaming with contextual bandits and MPQUIC")
    parser.add_argument("--exp_id", default="default", help="experiment id")
    parser.add_argument("--algorithm", choices=["LinUCB", "LinTS", "LinGreedy"], default="LinUCB", help="scheduling algorithm")
    parser.add_argument("--scheduler", choices=["roundrobin", "minrtt", "contextual_bandit"], default="", help="scheduler")
    parser.add_argument("--linucb_alpha", type=float, default=0.1, help="alpha for LinUCB")
    parser.add_argument("--lints_alpha", type=float, default=0.1, help="alpha for LinTS")
    parser.add_argument("--egreedy_epsilon", type=float, default=0.1, help="epsilon for epsilon-greedy")
    parser.add_argument("--nb_segment", type=int, default=100, help="number of segments to download")

    args = parser.parse_args()

    setLogLevel('info')
    net = Mininet(link=TCLink)

    h1 = net.addHost('h1')
    h2 = net.addHost('h2')
    r1 = net.addHost('r1')

    linkopt_server = {'bw': 1000}
    # delay is one way delay
    linkopt_starlink = {'bw': bw_high, 'delay': latency_high, 'loss': 1}
    linkopt_broadband = {'bw': bw_low, 'delay': latency_low, 'loss': 0}

    net.addLink(r1, h1, cls=TCLink, **linkopt_server)
    net.addLink(r1, h2, cls=TCLink, **linkopt_starlink)
    net.addLink(r1, h2, cls=TCLink, **linkopt_broadband)
    net.build()

    r1.cmd("ifconfig r1-eth0 0")
    r1.cmd("ifconfig r1-eth1 0")
    r1.cmd("ifconfig r1-eth2 0")

    r1.cmd("echo 1 > /proc/sys/net/ipv4/ip_forward")
    r1.cmd("echo 1 > /proc/sys/net/ipv4/conf/all/proxy_arp")

    r1.cmd("ifconfig r1-eth0 10.0.1.1 netmask 255.255.255.0")
    r1.cmd("ifconfig r1-eth1 10.0.2.1 netmask 255.255.255.0")
    r1.cmd("ifconfig r1-eth2 10.0.3.1 netmask 255.255.255.0")

    h1.cmd("ifconfig h1-eth0 0")

    h2.cmd("ifconfig h2-eth0 0")
    h2.cmd("ifconfig h2-eth1 0")

    h1.cmd("ifconfig h1-eth0 10.0.1.2 netmask 255.255.255.0")

    h2.cmd("ifconfig h2-eth0 10.0.2.2 netmask 255.255.255.0")
    h2.cmd("ifconfig h2-eth1 10.0.3.2 netmask 255.255.255.0")

    h1.cmd("ip route add default scope global nexthop via 10.0.1.1 dev h1-eth0")

    h2.cmd("ip rule add from 10.0.2.2 table 1")
    h2.cmd("ip rule add from 10.0.3.2 table 2")

    h2.cmd("ip route add 10.0.2.0/24 dev h2-eth0 scope link table 1")
    h2.cmd("ip route add 10.0.1.0/24 dev h2-eth0 scope link table 1")

    h2.cmd("ip route add 10.0.3.0/24 dev h2-eth1 scope link table 2")
    h2.cmd("ip route add 10.0.1.0/24 dev h2-eth1 scope link table 2")

    h2.cmd("ip route add default scope global nexthop via 10.0.2.1 dev h2-eth0")

    change_bw_process = Process(target=change_path_bw, args=(h2,))
    change_bw_process.start()

    change_latency_process = Process(target=change_path_latency, args=(h2,))
    change_latency_process.start()
    #
    server_process = Process(target=start_server, args=(h1,))
    server_process.start()

    h2.cmdPrint("./.venv/bin/python3 main.py "
                "--scheduler {} --exp_id {} --egreedy_epsilon {} --algorithm {} --nb_segment {} --linucb_alpha {} --lints_alpha {}".
                format(args.scheduler, args.exp_id, args.egreedy_epsilon, args.algorithm, args.nb_segment, args.linucb_alpha, args.lints_alpha))

    print("done")
    server_process.terminate()
    change_bw_process.terminate()
    change_latency_process.terminate()
