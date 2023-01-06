import time

import numpy as np
import subprocess
import argparse

bw_range_low = 0
bw_range_high = 0

# mean background traffic (Mbps)
count = 100


def start_iperf3(port):
    options = ["-s", "-p", str(port)]
    cmd = ["iperf3"] + options
    subprocess.run(cmd)


def generate_background_traffic(server_ip, server_port, mean, interface):
    while True:
        # random_number = random.uniform(bw_range_low, bw_range_high)
        # print(int(random_number))

        # Generate 10 random numbers from the Poisson distribution
        poisson_samples = np.random.poisson(mean, count)

        for random_number in poisson_samples:
            bw = random_number * 1000000

            # Set the options for the iperf3 command
            options = ["-c", server_ip, "-p", server_port, "-t", "1", "-b", str(bw), "--bind-dev", interface, "-R"]
            cmd = ["iperf3"] + options

            print(cmd)
            # Call iperf3 and save the output
            _ = subprocess.run(cmd, capture_output=True)


def set_static_latency(interface, latency):
    # sudo tc qdisc replace dev h2-eth1 root netem delay 50ms
    # 100ms 20ms distribution normal, https://www.cs.unm.edu/~crandall/netsfall13/TCtutorial.pdf
    options = ["qdisc", "replace", "dev", interface, "root", "netem", "delay", latency]
    cmd = ["tc"] + options

    print(cmd)
    _ = subprocess.run(cmd, capture_output=True)


def set_latency_distribution(interface, latency, r, dist):
    # sudo tc qdisc replace dev h2-eth1 root netem delay 100ms 20ms distribution normal
    # https://www.cs.unm.edu/~crandall/netsfall13/TCtutorial.pdf
    if dist not in ["normal"]:
        print("distribution {} not supported".format(dist))
    options = ["qdisc", "replace", "dev", interface, "root", "netem", "delay", latency, r, "distribution", dist]
    cmd = ["tc"] + options

    print(cmd)
    _ = subprocess.run(cmd, capture_output=True)


def set_latency_by_traces(interface, traces):
    for latency in traces:
        options = ["qdisc", "replace", "dev", interface, "root", "netem", "delay", "{}ms".format(latency)]
        cmd = ["tc"] + options

        print("setting latency on {} to {}".format(interface, latency))
        _ = subprocess.run(cmd, capture_output=True)
        time.sleep(1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    # Add the command line arguments
    parser.add_argument("-c", "--server", required=True, help="server ip")
    parser.add_argument("-p", "--port", required=True, help="server port")
    parser.add_argument("-m", "--mean", required=True, help="mean background traffic (Mbps)")
    parser.add_argument("-i", "--interface", required=True, help="bind to interface")

    args = parser.parse_args()

    generate_background_traffic(args.server, args.port, int(args.mean), args.interface)
