import time
import numpy as np
import subprocess
import argparse

bw_range_low = 0
bw_range_high = 0

# Set the IP address of the server
server_ip = ""

# Set the port number of the server
server_port = "5201"

# mean background traffic (Mbps)
mean = 0
count = 10


def generate_background_traffic():
    while True:
        # random_number = random.uniform(bw_range_low, bw_range_high)
        # print(int(random_number))

        # Generate 10 random numbers from the Poisson distribution
        poisson_samples = np.random.poisson(mean, count)

        for random_number in poisson_samples:
            bw = random_number * 1000000

            # Set the options for the iperf3 command
            options = ["-c", server_ip, "-p", server_port, "-t", "1", "-b", str(bw)]
            cmd = ["iperf3"] + options

            print(cmd)
            # Call iperf3 and save the output
            _ = subprocess.run(cmd, capture_output=True)

            time.sleep(1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    # Add the command line arguments
    parser.add_argument("-c", "--server", required=True, help="server ip")
    parser.add_argument("-b", "--mean", required=True, help="mean background traffic (Mbps)")
    parser.add_argument("-u", "--upper", required=True, help="upper bound of link bandwidth")

    args = parser.parse_args()

    server_ip = args.server
    mean = int(args.mean)
    bw_range_high = int(args.upper)

    generate_background_traffic()
