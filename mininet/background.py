import sys

from iperf import generate_background_traffic, start_iperf3
import threading


def client():
    server = "10.0.1.2"
    mean1 = 30
    mean2 = 5
    iface1 = "h2-eth0"
    iface2 = "h2-eth1"

    th_link1 = threading.Thread(target=generate_background_traffic, args=(server, "5201", mean1, iface1))
    th_link2 = threading.Thread(target=generate_background_traffic, args=(server, "5202", mean2, iface2))

    # Start the thread
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
