import sys

from iperf import generate_background_traffic, start_iperf3, set_latency_distribution, set_latency_by_traces
import threading
from config import bw_fast, bw_slow, bg_bw_percent


def client():
    server = "10.0.1.2"
    mean1 = bw_fast * bg_bw_percent
    mean2 = bw_slow * bg_bw_percent
    iface1 = "h2-eth0"
    iface2 = "h2-eth1"

    # bandwidth threads
    th_link1 = threading.Thread(target=generate_background_traffic, args=(server, "5201", mean1, iface1))
    th_link2 = threading.Thread(target=generate_background_traffic, args=(server, "5202", mean2, iface2))

    # latency thread
    # th_latency_link1 = threading.Thread(target=set_latency_distribution, args=(iface1, "15ms", "5ms", "normal"))

    # set latency by starlink traces
    with open("starlink/latency.txt", "r") as f:
        latency_traces = f.read().split("\n")

    th_latency_link1 = threading.Thread(target=set_latency_by_traces, args=(iface1, latency_traces[:-1]))
    th_latency_link2 = threading.Thread(target=set_latency_distribution, args=(iface2, "50ms", "20ms", "normal"))

    # Start the thread
    th_latency_link1.start()
    th_latency_link2.start()

    th_link1.start()
    th_link2.start()


def server():
    th_link1 = threading.Thread(target=start_iperf3, args=(5201,))
    th_link2 = threading.Thread(target=start_iperf3, args=(5202,))

    th_link1.start()
    th_link2.start()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: {} server/client".format(sys.argv[0]))
        sys.exit(0)
    if sys.argv[1] in ["server", "s"]:
        server()
    elif sys.argv[1] in ["client", "c"]:
        client()
