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

from mininet.cli import CLI
from mininet.link import TCLink
from mininet.log import setLogLevel
from mininet.net import Mininet
from config import bw_high, bw_low, latency_high, latency_low

if '__main__' == __name__:
    setLogLevel('info')
    net = Mininet(link=TCLink)

    h1 = net.addHost('h1')
    h2 = net.addHost('h2')
    r1 = net.addHost('r1')

    linkopt_server = {'bw': 1000}
    # delay is one way delay
    linkopt_starlink = {'bw': bw_high, 'delay': latency_high, 'loss': 0}
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

    CLI(net)
    net.stop()
