sudo docker run --privileged --tmpfs /tmp --tmpfs /run -v /sys/fs/cgroup:/sys/fs/cgroup:ro -v /home/clarkzjw/Documents/dataset/mpd:/app/mpd clarkzjw/player:systemd
