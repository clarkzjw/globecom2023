import time

import numpy as np
import subprocess
import argparse

bw_range_low = 0
bw_range_high = 0

# mean background traffic (Mbps)
count = 10


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


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    # Add the command line arguments
    parser.add_argument("-c", "--server", required=True, help="server ip")
    parser.add_argument("-p", "--port", required=True, help="server port")
    parser.add_argument("-m", "--mean", required=True, help="mean background traffic (Mbps)")
    parser.add_argument("-i", "--interface", required=True, help="bind to interface")

    args = parser.parse_args()

    generate_background_traffic(args.server, args.port, int(args.mean), args.interface)
