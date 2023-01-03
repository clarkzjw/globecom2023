import time
import subprocess

host = "34.82.168.196"

with open("rtt.txt", "w") as f:
    while True:
        ping = subprocess.Popen(
            ["ping", "-c", "1", host],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        out, error = ping.communicate()
        print(out)
        out = out.decode("utf-8").split("/")
        if len(out) < 5:
            f.write("{} {}\n".format(time.time(), -1))
        else:
            rtt = out[4]
            f.write("{} {}\n".format(time.time(), rtt))
        time.sleep(5)
