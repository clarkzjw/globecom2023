dir="/home/clarkzjw/Documents/sync/UVic/PanLab/Code/picoquic/player"
export SSLKEYLOGFILE="$dir/tmp/key"
$dir/tserver -M 2 -q qlog -c $dir/ca/ca-cert.pem -k $dir/ca/server-key.pem -p 443 -w /home/clarkzjw/Documents/dataset/mpd
